#pragma once

#include <stdint.h>

enum ISP_COMMANDS
{
  ISP_COMMAND_GET_FIRMWARE_ID  = 0x0101,  // data : none, "f0 00 4e 4c 01 01 f7"
  ISP_COMMAND_START_ISP_DATA   = 0x0102,  // data : none, "f0 00 4e 4c 02 01 f7"
  ISP_COMMAND_END_ISP_DATA     = 0x0103,  // data : none, "f0 00 4e 4c 03 01 f7"
  ISP_COMMAND_EXECUTE_ISP_DATA = 0x0104,  // data : none, "f0 00 4e 4c 04 01 f7"
  ISP_COMMAND_END_COMMAND_MODE = 0x0105,  // data : none, "f0 00 4e 4c 05 01 f7"
  ISP_COMMAND_ISP_DATA_BLOCK   = 0x0106,  // data : any number > 0 of NLL sysex encoded 8bit data "f0 00 4e 4c 06 01 [...] f7"
};

enum ISP_NOTIFICATIONS
{
  ISP_NOTIFICATION_FIRMWARE_ID = 0x0201,  // data : none, "f0 00 4e 4c 01 02 [NLL sysex encoded 8bit data] f7"
};

// three-byte MIDI Manufacturer ID (inofficial, selected to avoid conflicts with taken IDs
// see https://www.midi.org/specifications-old/item/manufacturer-id-numbers
#define MMID0 (0)
#define MMID1 (0x4E)  // 'N', will become 0x20 or 0x21 with an official ID (0x20 = european group)
#define MMID2 (0x4C)  // 'L', will be come the next free number assigned for us by midi.org

// Note: we are using the non-official MIDI Manufacturer ID 0x00 0x4E 0x4C (ascii 'NL')
// The ID is mandatory after each 0xF0 sysex start so that other devices will
// ignore the sysex properly in case it actually reaches the device. This can happen
// for example in the fw-uploader which issues an INFO request before it continue
// (if something is returned)

// command structure is always :
// F0 00 4E 4C cmdID-L cmdID-H [(possibly encoded) data bytes] F7
// 16bit command ID cmdID is comprised of low and high bytes, both of
// which always have bit7==0 to be legal
// depending on the command optional data bytes might follow, which may
// or may not be encoded 8-bit source data, the command parser is in charge for that

// "\0NL"
// sysex: f0 00 4e 4c
static const uint8_t NLMB_ManuID[] = {
  0xF0, MMID0, MMID1, MMID2
};
static const uint8_t NLMB_ManuID_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2
};
