#include "midi/nl_ispmarkers.h"

// clang-format off

// "NLMBISPS"
// sysex: f0 4e 4c 4d 42 49 53 50 53 f7
const uint8_t ISP_START[] = {
  0xF0, 'N', 'L', 'M', 'B', 'I', 'S', 'P', 'S', 0xF7
};
const uint8_t ISP_START_RAW[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'S',
  0x05, 0xF7,  0,  0
};

// "NLMBISPE"
// sysex: f0 4e 4c 4d 42 49 53 50 45 f7
const uint8_t ISP_END[] = {
  0xF0, 'N', 'L', 'M', 'B', 'I', 'S', 'P', 'E', 0xF7
};
const uint8_t ISP_END_RAW[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'E',
  0x05, 0xF7, 0,   0
};

// "NLMBISPX"
// sysex: f0 4e 4c 4d 42 49 53 50 58 f7
const uint8_t ISP_EXECUTE[] = {
  0xF0, 'N', 'L', 'M', 'B', 'I', 'S', 'P', 'X', 0xF7
};
const uint8_t ISP_EXECUTE_RAW[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'X',
  0x05, 0xF7, 0,   0
};

// "NLMBISPI"
// sysex: f0 4e 4c 4d 42 49 53 50 49 f7
const uint8_t ISP_INFO[] = {
  0xF0, 'N', 'L', 'M', 'B', 'I', 'S', 'P', 'I', 0xF7
};
const uint8_t ISP_INFO_RAW[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'I',
  0x05, 0xF7, 0,   0
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
  else if (marker == ISP_START_RAW)
    return sizeof ISP_START_RAW;
  else if (marker == ISP_END_RAW)
    return sizeof ISP_END_RAW;
  else if (marker == ISP_EXECUTE_RAW)
    return sizeof ISP_EXECUTE_RAW;
  else if (marker == ISP_INFO_RAW)
    return sizeof ISP_INFO_RAW;
  else
    return 0;
}
