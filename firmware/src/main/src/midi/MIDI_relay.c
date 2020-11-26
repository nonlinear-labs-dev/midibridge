//#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"

#define PACKET_TIMEOUT   (8000ul)  // in 125us units, 8000*0.125ms = 1s
#define ACTIVITY_TIMEOUT (800ul)   // in 125us units, 800*0.125ms = 0.1s
typedef enum
{
  IDLE = 0,
  RECEIVED,
  TRANSMITTING,
  WAIT_FOR_XMIT_READY,
  WAIT_FOR_XMIT_DONE,
  ABORTING,
} PacketState_t;

typedef struct
{
  PacketState_t state;
  uint8_t *     pData;
  int32_t       len;
  int           remaining;
  int           toTransfer;
  int           index;
  uint32_t      timeoutCntr;
  int           timeoutFlag;
} PacketTransfer_t;

static PacketTransfer_t packetTransfer[2];  // port number is for outgoing port

static inline void packetTransferReset(uint8_t const port)
{
  PacketTransfer_t *p = &(packetTransfer[port]);
  p->state            = IDLE;
  USB_MIDI_SuspendReceive(port ^ 1, 0);  // keep receiver enabled
  p->pData       = NULL;
  p->len         = 0;
  p->remaining   = 0;
  p->index       = 0;
  p->timeoutCntr = 0;
  p->timeoutFlag = 0;
}

static inline void displayTransfer(uint8_t const port)
{
  PacketTransfer_t *p = &(packetTransfer[port]);
  switch (p->state)
  {
    case IDLE:
      LED_SetDirect(port, 0b000);  // OFF
      break;
    case RECEIVED:
    case TRANSMITTING:
    case WAIT_FOR_XMIT_READY:
      LED_SetDirect(port, 0b111);  // WHITE
      break;
    case WAIT_FOR_XMIT_DONE:
    case ABORTING:
      LED_SetDirect(port, 0b011);  // YELLOW
      break;
  }
}

static inline void checkSends(uint8_t const port)
{
  __disable_irq();
  if (!USB_MIDI_IsConfigured(port))
  {
    LED_SetDirect(port, 0b100);  // BLUE
    packetTransferReset(port);
    __enable_irq();
    return;
  }

  PacketTransfer_t *p = &(packetTransfer[port]);

  if (p->state >= TRANSMITTING)
  {
    if (p->timeoutFlag)
    {
      p->timeoutFlag = 0;
      p->state       = ABORTING;
    }
  }

  switch (p->state)
  {
    default:
      break;

    case RECEIVED:
      p->remaining   = p->len;
      p->index       = 0;
      p->state       = TRANSMITTING;
      p->timeoutCntr = PACKET_TIMEOUT;
      p->timeoutFlag = 0;
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
      __enable_irq();
      USB_MIDI_KillTransmit(port);
      break;
  }
  __enable_irq();

  displayTransfer(port);
}

void MIDI_Relay_ProcessFast(void)
{
  checkSends(0);
  checkSends(1);
}

void MIDI_Relay_Process(void)
{
}

// ------------------------------------------------------------

static void Receive_IRQ_Callback(uint8_t const port, uint8_t *buff, uint32_t len)
{
  if (len > 512)
  {  // we should never receive a packet when not IDLE
    LED_SetDirectAndHalt(0b101);
  }

  if (len == 0)
    return;

  uint8_t const outgoingPort = port ^ 1;
  if (!USB_MIDI_IsConfigured(outgoingPort))  // output side not ready ?
  {
    packetTransferReset(outgoingPort);
    return;
  }

  if (packetTransfer[outgoingPort].state != IDLE)
  {  // we should never receive a packet when not IDLE
    LED_SetDirectAndHalt(0b001);
  }

  // setup packet transfer data ...
  packetTransfer[outgoingPort].pData = buff;
  packetTransfer[outgoingPort].len   = len;
  packetTransfer[outgoingPort].state = RECEIVED;
  // ... and block receiver until transmit finished/failed
  USB_MIDI_SuspendReceive(port, 1);
}

static void Send_IRQ_Callback(uint8_t const port)
{
}

void MIDI_Relay_TickerInterrupt(void)
{
  if (packetTransfer[0].timeoutCntr)
  {
    if (--packetTransfer[0].timeoutCntr == 0)
      packetTransfer[0].timeoutFlag = 1;
  }
  if (packetTransfer[1].timeoutCntr)
  {
    if (--packetTransfer[1].timeoutCntr == 0)
      packetTransfer[1].timeoutFlag = 1;
  }
}

void MIDI_Relay_Init(void)
{
  USB_MIDI_Config(0, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Config(1, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Init(0);
  USB_MIDI_Init(1);
}
