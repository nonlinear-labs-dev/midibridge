/******************************************************************************/
/** @file		nl_usb_midi.c
    @date		2014-12-11
    @brief    	Functions for the USB-MIDI driver
    @example	Interrupt mode:
    			main() {
    				...
    				USB_MIDI_Init();
    				...
    				while(1) {
    					USB_MIDI_Send(buffer, length, 1);
    				}
    			}

    			Polling mode:
    			main () {
    				...
    				USB_MIDI_Init();
    				...
    				while(1) {
    					...
    					if(systick_triggered){
    						USB_MIDI_Poll();
    					}
    					...
    					USB_MIDI_Send(buffer, length, 0);
    				}
    			}
	@ingroup	nl_drv_modules
    @author		Nemanja Nikodijevic [2014-12-11]

*******************************************************************************/
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "usb/nl_usb_core.h"
#include "sys/globals.h"
#include "io/pins.h"

typedef struct
{
  uint32_t        endOfBuffer;
  MidiRcvCallback MIDI_RcvCallback;
  uint8_t         dropMessages;
} UsbMidi_t;

static UsbMidi_t usbMidi[2];

/******************************************************************************/
/** @brief		Endpoint 1 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void EndPoint1(uint8_t const port, uint32_t const event)
{
  static uint8_t outbuff[512];
  uint32_t       length;

  switch (event)
  {
    case USB_EVT_OUT:
      length = USB_ReadEP(port, 0x01, outbuff);
      if (usbMidi[port].MIDI_RcvCallback)
        usbMidi[port].MIDI_RcvCallback(port, outbuff, length);
    case USB_EVT_OUT_NAK:
      USB_ReadReqEP(port, 0x01, outbuff, 512);
      break;
  }
}

/******************************************************************************/
/** @brief		Endpoint 2 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void EndPoint2(uint8_t const port, uint32_t const event)
{

  switch (event)
  {
    case USB_EVT_IN_NAK:
    case USB_EVT_IN:  // end of write out
      if (port == 0)
        LED_DBG1 = 0;
      else
        LED_DBG2 = 0;
      break;
  }
}

/******************************************************************************/
/** @brief    Function that initializes USB MIDI driver for USB0 controller
*******************************************************************************/
void USB_MIDI_Init(uint8_t const port)
{
  /** assign descriptors */
  USB_Core_Device_Descriptor_Set(port, (const uint8_t *) USB_MIDI_DeviceDescriptor);
  USB_Core_Device_FS_Descriptor_Set(port, (const uint8_t *) USB_MIDI_FSConfigDescriptor);
  USB_Core_Device_HS_Descriptor_Set(port, (const uint8_t *) USB_MIDI_HSConfigDescriptor);
  USB_Core_Device_String_Descriptor_Set(port, (const uint8_t *) (port == 0) ? USB0_MIDI_StringDescriptor : USB1_MIDI_StringDescriptor);
  USB_Core_Device_Device_Quali_Descriptor_Set(port, (const uint8_t *) USB_MIDI_DeviceQualifier);
  /** assign callbacks */
  USB_Core_Endpoint_Callback_Set(port, 1, EndPoint1);
  USB_Core_Endpoint_Callback_Set(port, 2, EndPoint2);
  USB_Core_Init(port);
}

void USB_MIDI_DeInit(uint8_t const port)
{
  USB_Core_DeInit(port);
}

/******************************************************************************/
/** @brief    Function that configures the USB MIDI driver
 *  @param[in]	midircv		Pointer to the callback function for
 *  						the MIDI received data
*******************************************************************************/
void USB_MIDI_Config(uint8_t const port, MidiRcvCallback midircv)
{
  usbMidi[port].MIDI_RcvCallback = midircv;
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
    if (port == 0)
      LED_DBG1 = 1;
    else
      LED_DBG2 = 1;
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
void USB_MIDI_DropMessages(uint8_t const port, uint8_t drop)
{
  usbMidi[port].dropMessages = drop;
}

// EOF