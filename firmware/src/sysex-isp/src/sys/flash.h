#pragma once

#include <stdint.h>

void FLASH_Init(void);
int  flashMemory(uint32_t const* const buf, uint16_t const len, uint8_t const bank);
