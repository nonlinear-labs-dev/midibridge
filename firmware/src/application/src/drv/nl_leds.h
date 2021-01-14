/******************************************************************************/
/** @file		nl_leds.h
    @brief	PWM LED low-level handling
*******************************************************************************/
#pragma once

#include <stdint.h>
#include "io/pins.h"

typedef enum
{
  COLOR_OFF     = 0b000,
  COLOR_RED     = 0b001,
  COLOR_GREEN   = 0b010,
  COLOR_BLUE    = 0b100,
  COLOR_YELLOW  = 0b011,
  COLOR_CYAN    = 0b110,
  COLOR_MAGENTA = 0b101,
  COLOR_WHITE   = 0b111,
} LedColor_t;

static inline void LED_SetDirect(uint8_t const ledId, LedColor_t const rgb)
{
  if (ledId)
  {
    LED_RED1   = !(rgb & 0b001);
    LED_GREEN1 = !(rgb & 0b010);
    LED_BLUE1  = !(rgb & 0b100);
  }
  else
  {
    LED_RED0   = !(rgb & 0b001);
    LED_GREEN0 = !(rgb & 0b010);
    LED_BLUE0  = !(rgb & 0b100);
  }
}

static inline void LED_SetDirectAndHalt(uint8_t const rgb)
{
  LED_SetDirect(0, rgb);
  LED_SetDirect(1, rgb);
  __disable_irq();
  while (1)
    ;
}
