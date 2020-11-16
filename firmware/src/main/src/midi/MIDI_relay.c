#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"

#define A (0)
#define B (1)

typedef struct
{
  int32_t  len;
  uint32_t count;
  uint8_t *buff;
} PendingData_t;

static PendingData_t pendingData[2];

static inline void checkSends(uint8_t const port)
{
  uint8_t const incomingPort = port ^ 1;
  if (!USB_MIDI_IsConfigured(port))
  {
    USB_MIDI_SuspendReceive(incomingPort, 0);
    pendingData[port].len = 0;
    return;
  }

  if (pendingData[port].len > 0)
  {
    if (USB_MIDI_Send(port, pendingData[port].buff, pendingData[port].len))  // "start transmit" succeeded ?
    {
      pendingData[port].len = -1;  // mark transmit started
      return;
    }
  }

  if (pendingData[port].len != -1)
    return;  // no transmit running

  if (USB_MIDI_BytesToSend(port) > 0)
    return;

  if (USB_MIDI_BytesToSend(port) == -1)
    LED_SetDirectAndHalt(0b100);

  USB_MIDI_SuspendReceive(incomingPort, 0);  // resume receiver
}

void MIDI_Relay_ProcessFast(void)
{
  checkSends(0);
  checkSends(1);
  LED_DBG1 = pendingData[0].count;
  LED_DBG2 = pendingData[1].count;
}

void MIDI_Relay_Process(void)
{
}

// ------------------------------------------------------------

static void ReceiveCallback(uint8_t const port, uint8_t *buff, uint32_t len)
{
  uint8_t const outgoingPort = port ^ 1;
  if (!USB_MIDI_IsConfigured(outgoingPort))  // output side not ready ?
  {
    USB_MIDI_SuspendReceive(port, 0);
    pendingData[outgoingPort].len   = 0;
    pendingData[outgoingPort].count = 0;
    return;
  }

  if (len == 0)
  {
    LED_DBG3 = 1;
    return;
  }
  LED_DBG3 = 0;

  pendingData[outgoingPort].count++;

  USB_MIDI_SuspendReceive(port, 1);  // suspend receiver until transmit finished

  pendingData[outgoingPort].buff = buff;
  pendingData[outgoingPort].len  = len;
}

static void SendCallback(uint8_t const port)
{
  pendingData[port].count--;
}

void MIDI_Relay_Init(void)
{
  for (uint8_t port = 0; port <= 1; port++)
  {
    USB_MIDI_Config(port, ReceiveCallback, SendCallback);
    USB_MIDI_Init(port);
    USB_MIDI_Init(port);
  }
}
