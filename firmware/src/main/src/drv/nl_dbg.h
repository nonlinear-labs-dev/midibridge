/******************************************************************************/
/** @file		nl_dbg.h
    @date		2013-03-08
    @version	0.01
    @author		Daniel Tzschentke [2012--]
    @brief		This module is a debug interface
 	 	 	 	- LEDs
 	 	 	 	- buttons
 	 	 	 	- logic-analyzer IOs
    @ingroup	e2_mainboard
*******************************************************************************/
#pragma once

#include <stdint.h>

void DBG_Process(void);
void DBG_Led(uint8_t const ledId, uint8_t const on);
void DBG_Led_TimedOn(uint8_t const ledId, int16_t time);
