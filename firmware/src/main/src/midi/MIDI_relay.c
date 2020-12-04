//#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"
#include "sys/ticker.h"

#define PACKET_TIMEOUT             (8000ul)   // in 125us units, 8000 *0.125ms = 1000ms  until a packet is aborted
#define PACKET_TIMEOUT_SHORT       (800ul)    // in 125us units, 800  *0.125ms = 100ms    until the next packet is aborted
#define LATE_TIME                  (10ul)     // in 125us units, 10   *0.125ms = 1.25ms  until packet is marked as LATE
#define STALE_TIME                 (240ul)    // in 125us units, 240  *0.125ms = 30ms    until packet is marked as STALE
#define REALTIME_INDICATOR_TIMEOUT (160ul)    // in 125us units, 20ms display of  any real-time packets
#define LATE_INDICATOR_TIMEOUT     (16000ul)  // in 125us units, 2s display of any late packets
#define STALE_INDICATOR_TIMEOUT    (48000ul)  // in 125us units, 6s display of any stale packets
#define BLINK_TIME                 (8000ul)   // blink time for offline status display

typedef enum
{
  IDLE = 0,
  RECEIVED,
  TRANSMITTING,
  WAIT_FOR_XMIT_READY,
  WAIT_FOR_XMIT_DONE,
} PacketState_t;

struct PacketTransfer;

struct PacketTransfer
{
  int                    online;
  uint8_t                portNo;
  uint8_t                outgoingPortNo;
  struct PacketTransfer *outgoingTransfer;
  PacketState_t          state;

  uint8_t *pData;
  int32_t  len;
  int      remaining;
  int      toTransfer;
  int      index;

  uint64_t packetTimeout;
  uint64_t packetTime;
  int      dropped;
};

typedef struct PacketTransfer PacketTransfer_t;

static PacketTransfer_t packetTransfer[2] =  // port number is referring to incoming packet !
    {
      { .portNo = 0, .outgoingPortNo = 1, .outgoingTransfer = &packetTransfer[1] },
      { .portNo = 1, .outgoingPortNo = 0, .outgoingTransfer = &packetTransfer[0] },
    };

#define OP         PacketTransfer_t *const t
#define mkOP(name) PacketTransfer_t *const name

static inline void packetTransferReset(OP)
{
  t->state = IDLE;
  USB_MIDI_SuspendReceive(t->portNo, 0);  // keep receiver enabled
  t->pData         = NULL;
  t->len           = 0;
  t->dropped       = 0;
  t->packetTimeout = PACKET_TIMEOUT;
}

#define DIM_PWM (15)
static inline void displayStatus(uint8_t const port)
{  // display traffic/port state from an incoming(receiving) viewpoint
  static int cntr[2]   = { DIM_PWM, DIM_PWM };
  static int reload[2] = { DIM_PWM, DIM_PWM };
  int        output    = 0;
  int        color     = 0;

  if (!--cntr[port])
  {
    cntr[port] = reload[port];
    output     = 1;
  }
  if ((cntr[port] <= 3) && (reload[port] == BLINK_TIME))
    output = 1;
}

static inline void checkSends(OP)
{
  if (!t->outgoingTransfer->online)
    return;

  if (t->state == IDLE)
    return;

  uint64_t const now = ticker;

  switch (t->state)
  {                 // main sending state machine
    case RECEIVED:  // packet was received, so start transmit process
      t->state         = TRANSMITTING;
      t->packetTimeout = (!t->dropped) ? PACKET_TIMEOUT : PACKET_TIMEOUT_SHORT;
      t->dropped       = 0;
      t->remaining     = t->len;
      t->index         = 0;
      // fall-through is on purpose

    case TRANSMITTING:
    NextChunk:
      if (t->remaining == 0)
      {
        t->state = IDLE;
        USB_MIDI_SuspendReceive(t->portNo, 0);  // re-enable receiver
        break;
      }
      t->toTransfer = t->remaining;
      //if (t->toTransfer > USB_FS_BULK_SIZE)  // we're handling the chunks ourselves,
      //t->toTransfer = USB_FS_BULK_SIZE;    // so limit to packet size to Full-Speed size
      t->packetTime = now;
      t->state      = WAIT_FOR_XMIT_READY;
      // fall-through is on purpose

    case WAIT_FOR_XMIT_READY:
      if (USB_MIDI_Send(t->outgoingPortNo, &(t->pData[t->index]), t->toTransfer) < 0)
        break;  /// could not start transfer now, try later
      t->state = WAIT_FOR_XMIT_DONE;
      break;

    case WAIT_FOR_XMIT_DONE:
      if (USB_MIDI_BytesToSend(t->outgoingPortNo) > 0)
        break;  // still sending...
      t->remaining -= t->toTransfer;
      t->index += t->toTransfer;
      t->state = TRANSMITTING;
      goto NextChunk;
      break;
  }

  if (t->state >= WAIT_FOR_XMIT_READY)
  {
    if (now - t->packetTime > t->packetTimeout)  // packet could not be submitted
    {
      t->dropped = 1;
      USB_MIDI_KillTransmit(t->outgoingPortNo);
      t->state = IDLE;
      USB_MIDI_SuspendReceive(t->portNo, 0);  // re-enable receiver
      return;
    }
  }
}

static inline void checkPortStatus(OP)
{
  uint32_t tmp = USB_MIDI_IsConfigured(t->portNo);
  if (tmp != t->online)
  {
    t->online = tmp;
    if (!tmp)
      packetTransferReset(t);
  }
}

void MIDI_Relay_ProcessFast(void)
{
  checkPortStatus(&packetTransfer[0]);
  checkPortStatus(&packetTransfer[1]);

  checkSends(&packetTransfer[0]);
  checkSends(&packetTransfer[1]);
}

void MIDI_Relay_Process(void)
{
  displayStatus(0);
  displayStatus(1);
}

// ------------------------------------------------------------

static inline void onReceive(OP, uint8_t *buff, uint32_t len)
{
  if (len == 0)
    return;  // empty packet will not cause any action and is simply ignored

  if (!t->online)
    return;  // just in case incoming port went offline and we still got an interrupt

  if (len > 512)  // we should never ever receive a packet longer than the fixed(!) 512Bytes HS bulk size max
    LED_SetDirectAndHalt(COLOR_WHITE);

  if (!t->outgoingTransfer->online)  // outgoing port is offline, mark packet as dismissed
  {
    // ...
    return;
  }

  if (t->state != IDLE)  // we should never receive a packet when not IDLE
    LED_SetDirectAndHalt(COLOR_WHITE);

  // setup packet transfer data ...
  t->pData = buff;
  t->len   = len;
  t->state = RECEIVED;
  // ... and block receiver until transmit finished/failed
  USB_MIDI_SuspendReceive(t->portNo, 1);
}

static void Receive_IRQ_Callback(uint8_t const port, uint8_t *buff, uint32_t len)
{  // we use calls to the handler with port# as literal constant, to make inlining effective
  if (port == 0)
    onReceive(&packetTransfer[0], buff, len);
  else
    onReceive(&packetTransfer[1], buff, len);
}

void MIDI_Relay_Init(void)
{
  packetTransferReset(&packetTransfer[0]);
  packetTransferReset(&packetTransfer[1]);
  USB_MIDI_Config(0, Receive_IRQ_Callback);
  USB_MIDI_Config(1, Receive_IRQ_Callback);
  USB_MIDI_Init(0);
  USB_MIDI_Init(1);
}
