/******************************************************************************/
/** @file		nl_leds.h
    @brief	PWM LED low-level handling
*******************************************************************************/
#pragma once

#include <stdint.h>

enum BASE_COLORS
{
  COLOR_OFF     = 0b000,
  COLOR_RED     = 0b001,
  COLOR_GREEN   = 0b010,
  COLOR_BLUE    = 0b100,
  COLOR_YELLOW  = 0b011,
  COLOR_CYAN    = 0b110,
  COLOR_MAGENTA = 0b101,
  COLOR_WHITE   = 0b111,
};

void LED_SetDirect(uint8_t const ledId, uint8_t const rgb);
void LED_SetDirectAndHalt(uint8_t const rgb);
void LED_SetState(uint8_t const ledId, uint8_t const baseColor, uint8_t const bright, uint8_t const flickering);
void LED_GetState(uint8_t const ledId, uint8_t* const baseColor, uint8_t* const bright, uint8_t* const flickering);
void LED_SetColor(uint8_t const ledId, uint8_t const color);
void LED_Process(void);
void LED_ProcessPWM(void);
void LED_Init(void);
