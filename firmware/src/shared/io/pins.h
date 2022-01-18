#pragma once

#include <stdint.h>

enum pbcaVersion_t
{
  EVAL_BOARD = 0,
  V1a        = 1,
  V2a        = 2,
};

void    PINS_Init(void);
uint8_t getPbcaVersion(void);

#define LED_RED0   (*LEDs[0])
#define LED_GREEN0 (*LEDs[1])
#define LED_BLUE0  (*LEDs[2])
#define LED_RED1   (*LEDs[3])
#define LED_GREEN1 (*LEDs[4])
#define LED_BLUE1  (*LEDs[5])
#define USB0_VBUS  (*USBVBUS[0])
#define USB1_VBUS  (*USBVBUS[1])
#define LED_DBG0   (*DEBUG[0])
#define LED_DBG1   (*DEBUG[1])

#ifndef PINS_LOCAL
extern volatile uint32_t* const LEDs[6];        // LED0 RGB and LED1 RGB
extern volatile uint32_t* const USBVBUS[2];     // USB Vbus 0 and 1
extern volatile uint32_t* const DEBUG[2];       // DebugPins/LEDs 0 and 1
extern uint32_t                 __dummy_pin__;  // dummy for unavailable pins
#endif
