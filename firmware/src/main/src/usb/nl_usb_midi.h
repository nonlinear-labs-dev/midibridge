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

/* Definition for Midi Receive Callback function */
typedef void (*MidiRcvCallback)(uint8_t const port, uint8_t* buff, uint32_t len);

/* USB MIDI functions */
void USB_MIDI_Init(uint8_t const port);
void USB_MIDI_DeInit(uint8_t const port);
void USB_MIDI_Config(uint8_t const port, MidiRcvCallback midircv);
void USB_MIDI_Poll(uint8_t const port);

uint32_t USB_MIDI_IsConfigured(uint8_t const port);
uint32_t USB_MIDI_Send(uint8_t const port, uint8_t const* const buff, uint32_t const cnt);
int32_t  USB_MIDI_BytesToSend(uint8_t const port);
void     USB_MIDI_DropMessages(uint8_t const port, uint8_t drop);
