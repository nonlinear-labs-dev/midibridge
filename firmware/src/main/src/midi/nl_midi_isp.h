#pragma once

#include <stdint.h>

int ISP_isIspStart(uint8_t const* const buff, uint32_t const len);
int ISP_isIspEnd(uint8_t const* const buff, uint32_t const len);
int ISP_isIspExecute(uint8_t const* const buff, uint32_t const len);
int ISP_isIspInfo(uint8_t const* const buff, uint32_t const len);

int ISP_FillData(uint8_t const* const buff, uint32_t const len);

int ISP_Execute(void);
