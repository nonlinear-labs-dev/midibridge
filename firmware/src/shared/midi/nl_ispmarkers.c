#include "midi/nl_ispmarkers.h"

// clang-format off


// Note: we are using the non-official MIDI Manufacturer ID 0x00 0x4E 0x4C (ascii 'NL')
// The ID is mandatory after each 0xF0 sysex start so that other devices will
// ignore the sysex properly in case it actually reaches the device. This can happen
// for example in the fw-uploader which issues an INFO request before it continue
// (if something is returned


// "\0NLMBISPS"
// sysex: f0 00 4e 4c 4d 42 49 53 50 53 f7
const uint8_t ISP_START[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'S', 0xF7
};
const uint8_t ISP_START_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'S', 0xF7,  0
};

// "NLMBISPE"
// sysex: f0 00 4e 4c 4d 42 49 53 50 45 f7
const uint8_t ISP_END[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'E', 0xF7
};
const uint8_t ISP_END_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'E', 0xF7,  0
};

// "NLMBISPX"
// sysex: f0 00 4e 4c 4d 42 49 53 50 58 f7
const uint8_t ISP_EXECUTE[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'X', 0xF7
};
const uint8_t ISP_EXECUTE_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'X', 0xF7,  0
};

// "NLMBISPI"
// sysex: f0 00 4e 4c 4d 42 49 53 50 49 f7
const uint8_t ISP_INFO[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'I', 0xF7
};
const uint8_t ISP_INFO_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'I', 0xF7,  0
};

// "NLMBISPD"
// sysex: f0 00 4e 4c 4d 42 49 53 50 44 f7
const uint8_t ISP_DATA[] = {
  0xF0, MMID0, MMID1, MMID2, 'M', 'B', 'I', 'S', 'P', 'D', 0xF7
};
const uint8_t ISP_DATA_RAW[] = {
  0x04, 0xF0, MMID0, MMID1,
  0x04, MMID2, 'M', 'B',
  0x04, 'I', 'S', 'P',
  0x06, 'D', 0xF7,  0
};

// clang-format on

uint16_t ISP_getMarkerSize(uint8_t const* const marker)
{
  if (marker == ISP_START)
    return sizeof ISP_START;
  else if (marker == ISP_END)
    return sizeof ISP_END;
  else if (marker == ISP_EXECUTE)
    return sizeof ISP_EXECUTE;
  else if (marker == ISP_INFO)
    return sizeof ISP_INFO;
  else if (marker == ISP_DATA)
    return sizeof ISP_DATA;
  else if (marker == ISP_START_RAW)
    return sizeof ISP_START_RAW;
  else if (marker == ISP_END_RAW)
    return sizeof ISP_END_RAW;
  else if (marker == ISP_EXECUTE_RAW)
    return sizeof ISP_EXECUTE_RAW;
  else if (marker == ISP_INFO_RAW)
    return sizeof ISP_INFO_RAW;
  else if (marker == ISP_DATA_RAW)
    return sizeof ISP_DATA_RAW;
  else
    return 0;
}
