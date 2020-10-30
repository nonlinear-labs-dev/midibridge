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

uint16_t ISP_getMarkerSize(uint8_t const * const marker);
