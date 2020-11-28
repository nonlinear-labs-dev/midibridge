//#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"

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
  ABORTING,
} PacketState_t;

typedef enum
{
  REALTIME = 0,
  LATE,
  STALE,
} PacketLatency_t;

typedef struct
{
  uint32_t realtimeCntr;
  uint32_t lateCntr;
  uint32_t staleCntr;
} PortStatus_t;

static PortStatus_t portStatus[2];

static inline void portStatusReset(uint8_t const port)
{
  PortStatus_t *p = &(portStatus[port]);
  p->realtimeCntr = 0;
  p->lateCntr     = 0;
  p->staleCntr    = 0;
}

typedef struct
{
  PacketState_t   state;
  PacketLatency_t latency;
  uint8_t *       pData;
  int32_t         len;
  int             remaining;
  int             toTransfer;
  int             index;
  uint32_t        timeoutReload;
  uint32_t        timeoutCntr;
  int             timeoutFlag;
  uint32_t        realtimeCntr;
  int             dropped;
} PacketTransfer_t;

static PacketTransfer_t packetTransfer[2];  // port number is for outgoing port

static inline void packetTransferReset(uint8_t const port)
{
  PacketTransfer_t *p = &(packetTransfer[port]);
  p->state            = IDLE;
  USB_MIDI_SuspendReceive(port ^ 1, 0);  // keep receiver enabled
  p->pData         = NULL;
  p->len           = 0;
  p->remaining     = 0;
  p->index         = 0;
  p->timeoutCntr   = 0;
  p->timeoutFlag   = 0;
  p->latency       = REALTIME;
  p->dropped       = 0;
  p->timeoutReload = (!p->dropped) ? PACKET_TIMEOUT : PACKET_TIMEOUT_SHORT;
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

  uint8_t           sendPort  = port ^ 1;
  PortStatus_t *    s         = &(portStatus[port]);          // display is for this receiving port
  PacketTransfer_t *p         = &(packetTransfer[sendPort]);  // but packet state comes from the sending port
  int               connected = USB_MIDI_IsConfigured(port);

  if (connected)
    reload[port] = DIM_PWM;
  else
    reload[port] = BLINK_TIME;

  do  // do {} while (0) loop is a helper so we can use break statements
  {
    if (!connected)
    {  // not up
      int powered = (port == 0) ? pinUSB0_VBUS : pinUSB1_VBUS;
      if (powered)
        color = 0b101;  // dim magenta
      else
        color = 0b100;  // dim blue
      break;
    }

    if (p->state == IDLE)
    {  // no outgoing packet on the run
      if (s->staleCntr)
        color = 0b001;  // dim red
      else if (s->lateCntr)
        color = 0b011;  // dim orange
      else
        color = 0b010;  // dim green

      if (s->realtimeCntr)
      {
        output = 1;  // force output while monoflop is running
        switch (p->latency)
        {
          case REALTIME:
            color = 0b010;  // GREEN
            break;
          case LATE:
            color = 0b011;  // YELLOW
            break;
          case STALE:
            color = 0b001;  // RED
            break;
        }
      }
      break;
    }
  } while (0);

  if (!output)
    color = 0;
  LED_SetDirect(port, color);
}

static inline void checkSends(uint8_t const port)
{
  __disable_irq();
  if (!USB_MIDI_IsConfigured(port))
  {
    packetTransferReset(port);
    packetTransfer[port].latency = STALE;
    __enable_irq();
    return;
  }

  PacketTransfer_t *p           = &(packetTransfer[port]);
  uint8_t           receivePort = port ^ 1;
  PortStatus_t *    s           = &(portStatus[receivePort]);  // display is for other, receiving port

  if (p->state >= TRANSMITTING)
  {
    if (p->timeoutFlag)
    {
      p->timeoutFlag = 0;
      p->state       = ABORTING;
    }
    s->realtimeCntr = REALTIME_INDICATOR_TIMEOUT;
    if (p->timeoutCntr + LATE_TIME < p->timeoutReload)
    {
      p->latency  = LATE;
      s->lateCntr = LATE_INDICATOR_TIMEOUT;
    }
    if (p->timeoutCntr + STALE_TIME < p->timeoutReload)
    {
      p->latency   = STALE;
      s->staleCntr = STALE_INDICATOR_TIMEOUT;
    }

    int color = 0;
    switch (p->latency)
    {
      case REALTIME:
        if (s->staleCntr)
          color = 0b101;  // MAGENTA
        else if (s->lateCntr)
          color = 0b011;  // YELLOW
        else
          color = 0b110;  // CYAN
        break;
      case LATE:
        color = 0b011;  // YELOW
        break;
      case STALE:
        color = 0b001;  // RED
        break;
    }
    if (color)
      LED_SetDirect(receivePort, color);
  }

  switch (p->state)
  {
    default:
      break;

    case RECEIVED:
      p->remaining     = p->len;
      p->index         = 0;
      p->state         = TRANSMITTING;
      p->timeoutReload = (!p->dropped) ? PACKET_TIMEOUT : PACKET_TIMEOUT_SHORT;
      p->dropped       = 0;
      p->timeoutCntr   = p->timeoutReload;
      p->timeoutFlag   = 0;
      p->latency       = REALTIME;

      // fall-through is on purpose

    case TRANSMITTING:
      if (p->remaining == 0)
      {
        p->state = IDLE;
        USB_MIDI_SuspendReceive(port ^ 1, 0);  // re-enable receiver
        break;
      }
      p->toTransfer = p->remaining;
      if (p->toTransfer > USB_FS_BULK_SIZE)  // we're handling the chunks ourselves,
        p->toTransfer = USB_FS_BULK_SIZE;    // so limit to packet size for Full-Speed
      p->state = WAIT_FOR_XMIT_READY;
      // fall-through is on purpose

    case WAIT_FOR_XMIT_READY:
      if (USB_MIDI_Send(port, &(p->pData[p->index]), p->toTransfer) < 0)
        break;  /// could not start transfer now, try later
      p->state = WAIT_FOR_XMIT_DONE;
      break;

    case WAIT_FOR_XMIT_DONE:
      if (USB_MIDI_BytesToSend(port) > 0)
        break;  // still sending...
      p->remaining -= p->toTransfer;
      p->index += p->toTransfer;
      p->state = TRANSMITTING;
      break;
    case ABORTING:
      packetTransferReset(port);
      p->dropped = 1;
      __enable_irq();
      USB_MIDI_KillTransmit(port);
      break;
  }
  __enable_irq();
}

void MIDI_Relay_ProcessFast(void)
{
  checkSends(0);
  checkSends(1);
}

void MIDI_Relay_Process(void)
{
  displayStatus(0);
  displayStatus(1);
}

// ------------------------------------------------------------

static void Receive_IRQ_Callback(uint8_t const port, uint8_t *buff, uint32_t len)
{
  if (len > 512)
  {    // we should never receive a packet longer than the 512 FS bulk size max
    ;  //LED_SetDirectAndHalt(0b111);
  }

  if (len == 0)
    return;

  uint8_t           sendPort = port ^ 1;
  PortStatus_t *    s        = &(portStatus[port]);          // display is for this receiving port
  PacketTransfer_t *p        = &(packetTransfer[sendPort]);  // but packet state comes from the sending port

  if (!USB_MIDI_IsConfigured(sendPort))  // output side not ready ?
  {
    packetTransferReset(sendPort);
    s->lateCntr     = LATE_INDICATOR_TIMEOUT;
    s->staleCntr    = STALE_INDICATOR_TIMEOUT;
    s->realtimeCntr = REALTIME_INDICATOR_TIMEOUT;
    return;
  }

  if (p->state != IDLE)
  {  // we should never receive a packet when not IDLE
    LED_SetDirectAndHalt(0b001);
  }

  // setup packet transfer data ...
  p->pData = buff;
  p->len   = len;
  p->state = RECEIVED;
  // ... and block receiver until transmit finished/failed
  USB_MIDI_SuspendReceive(port, 1);
}

static void Send_IRQ_Callback(uint8_t const port)
{
}

void MIDI_Relay_TickerInterrupt(void)
{
  for (int i = 0; i < 2; i++)
  {
    if (packetTransfer[i].timeoutCntr)
    {
      if (--packetTransfer[i].timeoutCntr == 0)
        packetTransfer[i].timeoutFlag = 1;
    }
    if (portStatus[i].realtimeCntr)
      portStatus[i].realtimeCntr--;
    if (portStatus[i].lateCntr)
      portStatus[i].lateCntr--;
    if (portStatus[i].staleCntr)
      portStatus[i].staleCntr--;
  }
}

void MIDI_Relay_Init(void)
{
  USB_MIDI_Config(0, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Config(1, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Init(0);
  USB_MIDI_Init(1);
}
