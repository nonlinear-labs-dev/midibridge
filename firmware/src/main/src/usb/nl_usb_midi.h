/******************************************************************************/
/** @file		nl_usb_midi.h
    @date		2014-12-11
    @brief    	Definitions for the USB-MIDI driver
    @example
    @ingroup  	nl_drv_modules
    @author		Nemanja Nikodijevic 2014-12-11
*******************************************************************************/
#pragma once

#include "nl_usbd.h"

/** USB MIDI driver configuration block
 * @{
 */
/** MIDI buffer size */
#define USB_MIDI_BUFFER_SIZE 1024
/** @} */

/* Definition for Midi Callback functions */
typedef void (*MidiReceiveComplete_Callback)(uint8_t const port, uint8_t* buff, uint32_t len);
typedef void (*MidiSendComplete_Callback)(uint8_t const port);

/* USB MIDI functions */
void USB_MIDI_Init(uint8_t const port);
void USB_MIDI_DeInit(uint8_t const port);
void USB_MIDI_Config(uint8_t const port, MidiReceiveComplete_Callback midircv, MidiSendComplete_Callback midisend);
void USB_MIDI_Poll(uint8_t const port);

uint32_t USB_MIDI_IsConfigured(uint8_t const port);
void     USB_MIDI_SuspendReceive(uint8_t const port, uint8_t const suspend);
int      USB_MIDI_SuspendReceiveGet(uint8_t const port);
uint32_t USB_MIDI_Send(uint8_t const port, uint8_t const* const buff, uint32_t const cnt);
int32_t  USB_MIDI_BytesToSend(uint8_t const port);
void     USB_MIDI_DropMessages(uint8_t const port, uint8_t const drop);
