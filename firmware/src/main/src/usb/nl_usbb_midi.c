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
#include "usb/nl_usbb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "usb/nl_usbb_core.h"
#include "sys/globals.h"

static uint32_t endOfBuffer = 0;

static uint8_t  midiBuffer[2][USB_MIDI_BUFFER_SIZE];
static uint8_t  activeBuffer = 0;
static uint32_t midiBuffPosition[2];

static MidiRcvCallback USB_MIDI_RcvCallback = 0;

#ifndef __NO_USB_MIDI__
static uint8_t midiDropMessages = 0;
#else
static uint8_t midiDropMessages = 1;
#endif

/******************************************************************************/
/** @brief		Endpoint 1 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void USBB_EndPoint1(uint32_t event)
{
  static uint8_t outbuff[512];
  uint32_t       length;

  switch (event)
  {
    case USB_EVT_OUT:
      length = USBB_ReadEP(0x01, outbuff);
      if (USB_MIDI_RcvCallback)
        USB_MIDI_RcvCallback(outbuff, length);
    case USB_EVT_OUT_NAK:
      USBB_ReadReqEP(0x01, outbuff, 512);
      break;
  }
}

/******************************************************************************/
/** @brief		Endpoint 2 Callback
    @param[in]	event	Event that triggered the interrupt
*******************************************************************************/
static void USBB_EndPoint2(uint32_t event)
{

  switch (event)
  {
    case USB_EVT_IN_NAK:
    case USB_EVT_IN:
      USBB_MIDI_CheckBuffer();
      break;
  }
}

/******************************************************************************/
/** @brief    Function that initializes USB MIDI driver for USB0 controller
*******************************************************************************/
void USBB_MIDI_Init(void)
{
  /** assign descriptors */
  USBB_Core_Device_Descriptor_Set((const uint8_t *) USB_MIDI_DeviceDescriptor);
  USBB_Core_Device_FS_Descriptor_Set((const uint8_t *) USB_MIDI_FSConfigDescriptor);
  USBB_Core_Device_HS_Descriptor_Set((const uint8_t *) USB_MIDI_HSConfigDescriptor);
  USBB_Core_Device_String_Descriptor_Set((const uint8_t *) USBB_MIDI_StringDescriptor);
  USBB_Core_Device_Device_Quali_Descriptor_Set((const uint8_t *) USB_MIDI_DeviceQualifier);
  /** assign callbacks */
  USBB_Core_Endpoint_Callback_Set(1, USBB_EndPoint1);
  USBB_Core_Endpoint_Callback_Set(2, USBB_EndPoint2);
  USBB_Core_Init();
}

void USBB_MIDI_DeInit(void)
{
  USBB_Core_DeInit();
}

/******************************************************************************/
/** @brief    Function that configures the USB MIDI driver
 *  @param[in]	midircv		Pointer to the callback function for
 *  						the MIDI received data
*******************************************************************************/
void USBB_MIDI_Config(MidiRcvCallback midircv)
{
  USB_MIDI_RcvCallback = midircv;
}

/******************************************************************************/
/** @brief    Function that polls USB MIDI driver
*******************************************************************************/
void USBB_MIDI_Poll(void)
{
#if USBB_PORT_FOR_MIDI == 0
  USB0_IRQHandler();
#else
  USB1_IRQHandler();
#endif
}

/******************************************************************************/
/** @brief		Checks whether the USB-MIDI is connected and configured
    @return		1 - Success ; 0 - Failure
*******************************************************************************/
uint32_t USBB_MIDI_IsConfigured(void)
{
  return USBB_Core_IsConfigured();
}

/******************************************************************************/
/** @brief		Send MIDI buffer
    @param[in]	buff	Pointer to data buffer
    @param[in]	cnt		Amount of bytes to send
    @param[in]	imm		Immediate only
    @return		Number of bytes written - Success ; 0 - Failure
*******************************************************************************/
uint32_t USBB_MIDI_Send(uint8_t const *const buff, uint32_t const cnt, uint8_t const imm)
{

  if (midiDropMessages)
  {
    return 0;
  }

  if (USBB_Core_ReadyToWrite(0x82))
  {
    USBB_WriteEP(0x82, (uint8_t *) buff, (uint32_t) cnt);
    endOfBuffer = (uint32_t) buff + cnt;
    return cnt;
  }
  else if (!imm)
  {
    return USBB_MIDI_SendDelayed((uint8_t *) buff, (uint32_t) cnt);
  }
  return 0;
}

/******************************************************************************/
/** @brief		Write the data into buffer for delayed transfer
    @param[in]	buff	Pointer to data buffer
    @param[in]	cnt		Amount of bytes to send
    @return		Number of bytes written - Success ; 0 - Failure
*******************************************************************************/
uint32_t USBB_MIDI_SendDelayed(uint8_t *buff, uint32_t cnt)
{
  uint32_t i = 0;
  if (midiDropMessages)
  {
    return 0;
  }
  while ((midiBuffPosition[activeBuffer] < USB_MIDI_BUFFER_SIZE) && (i < cnt))
  {
    midiBuffer[activeBuffer][midiBuffPosition[activeBuffer]] = buff[i];
    i++;
    midiBuffPosition[activeBuffer]++;
  }
  return i;
}

/******************************************************************************/
/** @brief		Send out the current buffer
 	@return		1 - Success; 0 - Failure
*******************************************************************************/
uint32_t USBB_MIDI_CheckBuffer(void)
{
  if (midiBuffPosition[activeBuffer] && USBB_Core_ReadyToWrite(0x82))
  {
    USBB_WriteEP(0x82, midiBuffer[activeBuffer], midiBuffPosition[activeBuffer]);
    endOfBuffer                    = (uint32_t) midiBuffer[activeBuffer] + midiBuffPosition[activeBuffer];
    midiBuffPosition[activeBuffer] = 0;
    activeBuffer                   = activeBuffer ? 0 : 1;
    return 1;
  }
  else
    return 0;
}

/******************************************************************************/
/** @brief		Get the amount of bytes left to be sent
    @return		Amount of bytes to be sent from the buffer
*******************************************************************************/
uint32_t USBB_MIDI_BytesToSend(void)
{
  return USBB_Core_BytesToSend(endOfBuffer, 0x82);
}

/******************************************************************************/
/** @brief		Drop all messages written to the interface
    @param[in]	drop	1 - drop future messages; 0 - do not drop future msgs
*******************************************************************************/
void USBB_MIDI_DropMessages(uint8_t drop)
{
#ifndef __NO_USB_MIDI__
  midiDropMessages = drop;
#endif
}

// EOF
