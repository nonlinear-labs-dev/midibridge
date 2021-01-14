#pragma once

#include <stdint.h>

int  DEVCTL_isDeviceControlMsg(uint8_t** const pBuff, uint32_t* const pLen);
void DEVCTL_init(void);
void DEVCTL_processMsg(uint8_t* buff, uint32_t len);
