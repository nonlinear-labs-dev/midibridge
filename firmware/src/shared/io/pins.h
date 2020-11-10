#pragma once

#include "pin_setup.h"
#include "sys/globals.h"

// clang-format off

//
// --------------- LEDs ----------------
//
// System RGB-LEDs (those are active-low!)
#define LED_A_init() GPIO_DIR_OUT(0, 15); SFSP(1, 20) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_A          pGPIO_Word(0, 15)
#define LED_B_init() GPIO_DIR_OUT(0, 12); SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_B          pGPIO_Word(0, 12)
#define LED_C_init() GPIO_DIR_OUT(0,  2); SFSP(1, 15) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_C          pGPIO_Word(0,  2)

#define LED_D_init() GPIO_DIR_OUT(2, 13); SFSP(5,  4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_D          pGPIO_Word(2, 13)
#define LED_E_init() GPIO_DIR_OUT(1,  3); SFSP(1, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_E          pGPIO_Word(1,  3)
#define LED_F_init() GPIO_DIR_OUT(1,  9); SFSP(1, 6) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_F          pGPIO_Word(1,  9)

// LED assignments :
#define pLED_RED0	 LED_A
#define pLED_GREEN0  LED_B
#define pLED_BLUE0   LED_C
#define pLED_RED1	 LED_D
#define pLED_GREEN1  LED_E
#define pLED_BLUE1   LED_F

//
// Debug-LEDs (those are active-high)
#define LED_DBG1_init() GPIO_DIR_OUT(2, 0); SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG1           GPIO_Word(2, 0)
#define LED_DBG2_init() GPIO_DIR_OUT(2, 3); SFSP(4, 3) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG2           GPIO_Word(2, 3)
#define LED_DBG3_init() GPIO_DIR_OUT(2, 6); SFSP(4, 4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG3           GPIO_Word(2, 6)


static inline void debugPinsInit(void)
{
  LED_A_init();
  LED_B_init();
  LED_C_init();
  LED_D_init();
  LED_E_init();
  LED_F_init();
  *LED_A = 0;
  *LED_B = 0;
  *LED_C = 0;
  *LED_D = 0;
  *LED_E = 0;
  *LED_F = 0;

  LED_DBG1_init();
  LED_DBG2_init();
  LED_DBG3_init();
  LED_DBG1 = 0;
  LED_DBG2 = 0;
  LED_DBG3 = 0;
}


// --------------- USB VBUS-Monitors ----------------
#define pinUSB0_VBUS_init() GPIO_DIR_IN(3, 0); SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB0_VBUS          GPIO_Word(3, 0)
#define pinUSB1_VBUS_init() GPIO_DIR_IN(3, 1); SFSP(6, 2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB1_VBUS          GPIO_Word(3, 1)

static inline void USBPinsInit(void)
{
  pinUSB0_VBUS_init();
  pinUSB1_VBUS_init();
}

// ------- Init all the pins -------
// since M4 starts first, it's best to init the pins needed by M0 also right there
static inline void PINS_Init(void)
{
  debugPinsInit();
  USBPinsInit();
}

// clang-format on
