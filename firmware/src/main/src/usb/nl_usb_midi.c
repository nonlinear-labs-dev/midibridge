/******************************************************************************/
/** @file		nl_usb_midi.c
    @date		2014-12-11
    @brief    	Functions for the USB-MIDI driver
	@ingroup	nl_drv_modules
    @author		Nemanja Nikodijevic [2014-12-11]

*******************************************************************************/
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "usb/nl_usb_core.h"
#include "sys/globals.h"
#include "sys/nl_stdlib.h"
#include "io/pins.h"

#define RX_BUFFERSIZE (16384)  // 16k is the max the USB hardware can handle

__attribute__((section(".noinit.$RamAHB32"))) static uint8_t rxBuffer[2][RX_BUFFERSIZE];

typedef struct
{
  uint32_t                     endOfBuffer;
  MidiReceiveComplete_Callback ReceiveCallback;
  MidiSendComplete_Callback    SendCallback;
  uint8_t                      suspendReceive;
  uint8_t                      dropMessages;
} UsbMidi_t;

static UsbMidi_t usbMidi[2];

/******************************************************************************/
/** @brief		Endpoint 1 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void EndPoint1_ReadFromHost(uint8_t const port, uint32_t const event)
{
  uint32_t length;

  switch (event)
  {
    case USB_EVT_OUT:
      length = USB_ReadEP(port, 0x01, rxBuffer[port]);
      if (usbMidi[port].ReceiveCallback)
        usbMidi[port].ReceiveCallback(port, rxBuffer[port], length);
    case USB_EVT_OUT_NAK:
      if (!usbMidi[port].suspendReceive)
        USB_ReadReqEP(port, 0x01, rxBuffer[port], RX_BUFFERSIZE);
      break;
  }
}

/******************************************************************************/
/** @brief		Endpoint 2 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void EndPoint2_WriteToHost(uint8_t const port, uint32_t const event)
{

  switch (event)
  {
    case USB_EVT_IN_NAK:
      break;
    case USB_EVT_IN:  // end of write out
      if (usbMidi[port].SendCallback)
        usbMidi[port].SendCallback(port);
      break;
  }
}

/******************************************************************************/
/** @brief    Function that initializes USB MIDI driver for USB0 controller
*******************************************************************************/
void USB_MIDI_Init(uint8_t const port)
{
  memset(&(rxBuffer[0][0]), 0, RX_BUFFERSIZE);
  memset(&(rxBuffer[1][0]), 0, RX_BUFFERSIZE);

  /** assign descriptors */
  USB_Core_Device_Descriptor_Set(port, (const uint8_t *) USB_MIDI_DeviceDescriptor);
  USB_Core_Device_FS_Descriptor_Set(port, (const uint8_t *) USB_MIDI_FSConfigDescriptor);
  USB_Core_Device_HS_Descriptor_Set(port, (const uint8_t *) USB_MIDI_HSConfigDescriptor);
  USB_Core_Device_String_Descriptor_Set(port, (const uint8_t *) (port == 0) ? USB0_MIDI_StringDescriptor : USB1_MIDI_StringDescriptor);
  USB_Core_Device_Device_Quali_Descriptor_Set(port, (const uint8_t *) USB_MIDI_DeviceQualifier);
  /** assign callbacks */
  USB_Core_Endpoint_Callback_Set(port, 1, EndPoint1_ReadFromHost);
  USB_Core_Endpoint_Callback_Set(port, 2, EndPoint2_WriteToHost);
  USB_Core_Init(port);
}

void USB_MIDI_DeInit(uint8_t const port)
{
  usbMidi[port].suspendReceive = 0;
  USB_Core_DeInit(port);
  USB_Core_DeInit(port);
}

/******************************************************************************/
/** @brief    Function that configures the USB MIDI driver
 *  @param[in]	midircv		Pointer to the callback function for
 *  						the MIDI received data
*******************************************************************************/
void USB_MIDI_Config(uint8_t const port, MidiReceiveComplete_Callback midircv, MidiSendComplete_Callback midisend)
{
  usbMidi[port].ReceiveCallback = midircv;
  usbMidi[port].SendCallback    = midisend;
}

/******************************************************************************/
/** @brief    Function that polls USB MIDI driver
*******************************************************************************/
void USB_MIDI_Poll(uint8_t const port)
{
  if (port == 0)
    USB0_IRQHandler();
  else
    USB1_IRQHandler();
}

/******************************************************************************/
/** @brief		Checks whether the USB-MIDI is connected and configured
    @return		1 - Success ; 0 - Failure
*******************************************************************************/
uint32_t USB_MIDI_IsConfigured(uint8_t const port)
{
  return USB_Core_IsConfigured(port);
}

/******************************************************************************/
/** @brief		Send MIDI buffer
    @param[in]	buff	Pointer to data buffer
    @param[in]	cnt		Amount of bytes to send
    @param[in]	imm		Immediate only
    @return		Number of bytes written - Success ; 0 - Failure
*******************************************************************************/
uint32_t USB_MIDI_Send(uint8_t const port, uint8_t const *const buff, uint32_t const cnt)
{

  if (usbMidi[port].dropMessages)
    return 0;

  if (USB_Core_ReadyToWrite(port, 0x82))
  {
    USB_WriteEP(port, 0x82, (uint8_t *) buff, (uint32_t) cnt);
    usbMidi[port].endOfBuffer = (uint32_t) buff + cnt;
    return cnt;
  }
  return 0;
}

/******************************************************************************/
/** @brief		Get the amount of bytes left to be sent
    @return		Amount of bytes to be sent from the buffer
*******************************************************************************/
int32_t USB_MIDI_BytesToSend(uint8_t const port)
{
  return USB_Core_BytesToSend(port, usbMidi[port].endOfBuffer, 0x82);
}

/******************************************************************************/
/** @brief		Drop all messages written to the interface
    @param[in]	drop	1 - drop future messages; 0 - do not drop future msgs
*******************************************************************************/
void USB_MIDI_DropMessages(uint8_t const port, uint8_t const drop)
{
  usbMidi[port].dropMessages = drop;
}

/******************************************************************************/
/** @brief		Suspend further receives
    @param[in]	suspend	!= 0--> suspend, == 0, normal
*******************************************************************************/

void USB_MIDI_SuspendReceive(uint8_t const port, uint8_t const suspend)
{
  usbMidi[port].suspendReceive = suspend;
}

int USB_MIDI_SuspendReceiveGet(uint8_t const port)
{
  return usbMidi[port].suspendReceive;
}
// EOF
