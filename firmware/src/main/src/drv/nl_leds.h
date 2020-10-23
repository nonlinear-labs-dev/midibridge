/******************************************************************************/
/** @file		nl_leds.h
    @brief	PWM LED low-level handling
*******************************************************************************/
#pragma once

#include <stdint.h>

enum BASE_COLORS
{
  COLOR_OFF    = 0,
  COLOR_BLUE   = 1,
  COLOR_GREEN  = 2,
  COLOR_ORANGE = 3,
  COLOR_RED    = 4,
};

void LED_SetState(uint8_t const ledId, uint8_t const baseColor, uint8_t const bright, uint8_t const flickering);
void LED_GetState(uint8_t const ledId, uint8_t* const baseColor, uint8_t* const bright, uint8_t* const flickering);
void LED_SetColor(uint8_t const ledId, uint8_t const color);
void LED_Process(void);
void LED_ProcessPWM(void);
void LED_Init(void);
