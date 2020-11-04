#pragma once

#include <stdint.h>

void ISP_Init(void);

int ISP_isOurCommand(uint8_t const* const buff, uint32_t const len);
int ISP_collectAndExecuteCommand(uint8_t const* buff, uint32_t len);
