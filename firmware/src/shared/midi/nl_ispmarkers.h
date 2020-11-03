#pragma once

#include <stdint.h>

// three-byte MIDI Manufacturer ID (inofficial, selected to avoid conflicts with taken IDs
// see https://www.midi.org/specifications-old/item/manufacturer-id-numbers
#define MMID0 (0)
#define MMID1 (0x4E)  // 'N', will become 0x20 or 0x21 with an official ID (0x20 = european group)
#define MMID2 (0x4C)  // 'L', will be come the next free number assigned for us by midi.org

// Note: we are using the non-official MIDI Manufacturer ID 0x00 0x4E 0x4C (ascii 'NL')
// The ID is mandatory after each 0xF0 sysex start so that other devices will
// ignore the sysex properly in case it actually reaches the device. This can happen
// for example in the fw-uploader which issues an INFO request before it continue
// (if something is returned

// "\0NLMBISPS"
// sysex: f0 00 4e 4c 4d 42 49 53 50 53 f7
static const uint8_t ISP_START[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'S', 0xF7
};
static const uint8_t ISP_START_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'S', 0xF7, 0
};

// "NLMBISPE"
// sysex: f0 00 4e 4c 4d 42 49 53 50 45 f7
static const uint8_t ISP_END[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'E', 0xF7
};
static const uint8_t ISP_END_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'X', 0xF7, 0
};

// "NLMBISPX"
// sysex: f0 00 4e 4c 4d 42 49 53 50 58 f7
static const uint8_t ISP_EXECUTE[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'X', 0xF7
};
static const uint8_t ISP_EXECUTE_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'X', 0xF7, 0
};

// "NLMBISPI"
// sysex: f0 00 4e 4c 4d 42 49 53 50 49 f7
static const uint8_t ISP_INFO[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'I', 0xF7
};
static const uint8_t ISP_INFO_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'X', 0xF7, 0
};
