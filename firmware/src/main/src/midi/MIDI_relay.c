//#include "MIDI_relay.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"

#define A (0)
#define B (1)

typedef enum
{
  IDLE = 0,
  RECEIVED,
  TRANSMITTING,
  WAIT_FOR_XMIT_READY,
  WAIT_FOR_XMIT_DONE
} PacketState_t;

typedef struct
{
  PacketState_t state;
  uint8_t *     pData;
  int32_t       len;
  int           remaining;
  int           toTransfer;
  int           index;
} PacketTransfer_t;

static PacketTransfer_t packetTransfer[2];  // port number is for outgoing port

static inline void packetTransferReset(uint8_t const port)
{
  packetTransfer[port].state = IDLE;
  USB_MIDI_SuspendReceive(port ^ 1, 0);  // keep receiver enabled
  packetTransfer[port].pData     = NULL;
  packetTransfer[port].len       = 0;
  packetTransfer[port].remaining = 0;
  packetTransfer[port].index     = 0;
}

static inline void checkSends(uint8_t const port)
{
  __disable_irq();
  if (!USB_MIDI_IsConfigured(port))
  {
    packetTransferReset(port);
    __enable_irq();
    return;
  }

  // display
  switch (packetTransfer[port].state)
  {
    case IDLE:
      LED_SetDirect(port, 0b000);  // OFF
      break;
    case RECEIVED:
      LED_SetDirect(port, 0b010);  // GREEN
      break;
    case TRANSMITTING:
    case WAIT_FOR_XMIT_READY:
    case WAIT_FOR_XMIT_DONE:
      LED_SetDirect(port, 0b011);  // YELLOW
      break;
  }

  switch (packetTransfer[port].state)
  {
    default:
      break;

    case RECEIVED:
      packetTransfer[port].remaining = packetTransfer[port].len;
      packetTransfer[port].index     = 0;
      packetTransfer[port].state     = TRANSMITTING;
      // fall-through is on purpose

    case TRANSMITTING:
      if (packetTransfer[port].remaining == 0)
      {
        packetTransfer[port].state = IDLE;
        USB_MIDI_SuspendReceive(port ^ 1, 0);  // re-enable receiver
        break;
      }
      packetTransfer[port].toTransfer = packetTransfer[port].remaining;
      if (packetTransfer[port].toTransfer > USB_FS_BULK_SIZE)
        packetTransfer[port].toTransfer = USB_FS_BULK_SIZE;  // limit to packet size for Full-Speed
      packetTransfer[port].state = WAIT_FOR_XMIT_READY;
      // fall-through is on purpose

    case WAIT_FOR_XMIT_READY:
      if (USB_MIDI_Send(port, &(packetTransfer[port].pData[packetTransfer[port].index]), packetTransfer[port].toTransfer) < 0)
        break;  /// could not start transfer now, try later
      packetTransfer[port].state = WAIT_FOR_XMIT_DONE;
      break;

    case WAIT_FOR_XMIT_DONE:
      if (USB_MIDI_BytesToSend(port) > 0)
        break;  // still sending...
      packetTransfer[port].remaining -= packetTransfer[port].toTransfer;
      packetTransfer[port].index += packetTransfer[port].toTransfer;
      packetTransfer[port].state = TRANSMITTING;
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

void MIDI_Relay_Init(void)
{
  USB_MIDI_Config(0, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Config(1, Receive_IRQ_Callback, Send_IRQ_Callback);
  USB_MIDI_Init(0);
  USB_MIDI_Init(1);
}
