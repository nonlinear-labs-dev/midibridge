#pragma once

#include <stdint.h>

extern const uint8_t ISP_START[];
extern const uint8_t ISP_START_RAW[];
extern const uint8_t ISP_END[];
extern const uint8_t ISP_END_RAW[];
extern const uint8_t ISP_EXECUTE[];
extern const uint8_t ISP_EXECUTE_RAW[];
extern const uint8_t ISP_INFO[];
extern const uint8_t ISP_INFO_RAW[];
extern const uint8_t ISP_DATA[];
extern const uint8_t ISP_DATA_RAW[];

// three-byte MIDI Manufacturer ID (inofficial, selected to avoid conflicts with taken IDs
// see https://www.midi.org/specifications-old/item/manufacturer-id-numbers
#define MMID0 (0)
#define MMID1 (0x4E)  // 'N', will become 0x20 or 0x21 with an official ID (0x20 = european group)
#define MMID2 (0x4C)  // 'L', will be come the next free number assigned for us by midi.org

uint16_t ISP_getMarkerSize(uint8_t const* const marker);
