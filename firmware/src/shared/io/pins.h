#pragma once

#include "pin_setup.h"
#include "sys/delays.h"

#ifdef EVAL_BOARD
#warning Compiling for Evaluation Board specifics (GPIOs for LEDs etc)
#endif

// clang-format off

//
// --------------- LEDs ----------------

// System RGB-LEDs (those are active-low!)
#define LED_A_init() GPIO_DIR_OUT(0, 15); SFSP(1, 20) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(2,  9); SFSP(5,  0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_Ae          GPIO_Word(0, 15)
#define LED_Ap          GPIO_Word(2,  9)

#define LED_B_init() GPIO_DIR_OUT(0, 12); SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(1,  2); SFSP(1,  9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_Be          GPIO_Word(0, 12)
#define LED_Bp          GPIO_Word(1,  2)

#define LED_C_init() GPIO_DIR_OUT(0,  2); SFSP(1, 15) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(2, 11); SFSP(5,  2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_Ce          GPIO_Word(0,  2)
#define LED_Cp          GPIO_Word(2, 11)

#define LED_D_init() GPIO_DIR_OUT(2, 13); SFSP(5,  4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(0, 14); SFSP(2, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_De          GPIO_Word(2, 13)
#define LED_Dp          GPIO_Word(0, 14)

#define LED_E_init() GPIO_DIR_OUT(1,  3); SFSP(1, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(1, 11); SFSP(2, 11) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_Ee          GPIO_Word(1,  3)
#define LED_Ep          GPIO_Word(1, 11)

#define LED_F_init() GPIO_DIR_OUT(1,  9); SFSP(1, 6) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0; \
	                 GPIO_DIR_OUT(1, 10); SFSP(2,  9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_Fe          GPIO_Word(1,  9)
#define LED_Fp          GPIO_Word(1, 10)

// LED assignments :
// these are meant for use in direct assignments only
// we simply connect both possible lines for each LED as there are *no* pin conflicts
#define LED_RED0    LED_Ap = LED_Ae
#define LED_GREEN0  LED_Bp = LED_Be
#define LED_BLUE0   LED_Cp = LED_Ce
#define LED_RED1    LED_Dp = LED_De
#define LED_GREEN1  LED_Ep = LED_Ee
#define LED_BLUE1   LED_Fp = LED_Fe

//
// Debug-LEDs (those are active-high)
#ifdef EVAL_BOARD

// conflict with VBUS-dedect pin !
//  #define LED_DBG1_init() GPIO_DIR_OUT(2, 0); SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
//  #define LED_DBG1           GPIO_Word(2, 0)
#define LED_DBG2_init() GPIO_DIR_OUT(2, 3); SFSP(4, 3) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG2           GPIO_Word(2, 3)
#define LED_DBG3_init() GPIO_DIR_OUT(2, 6); SFSP(4, 4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG3           GPIO_Word(2, 6)

#else

static uint32_t __dummy_pin__;

#define LED_DBG1_init()
#define LED_DBG1        __dummy_pin__
#define LED_DBG2_init()
#define LED_DBG2        __dummy_pin__
#define LED_DBG3_init()
#define LED_DBG3        __dummy_pin__

#endif

static inline void debugPinsInit(void)
{
  LED_A_init();
  LED_B_init();
  LED_C_init();
  LED_D_init();
  LED_E_init();
  LED_F_init();
  LED_RED0 = 0;
  LED_RED1 = 0;
  LED_GREEN0 = 0;
  LED_GREEN1 = 0;
  LED_BLUE0 = 0;
  LED_BLUE1 = 0;
  LED_DBG2_init();
  LED_DBG3_init();
  LED_DBG2 = 0;
  LED_DBG3 = 0;
}

// --------------- USB VBUS-Monitors ----------------

static int pinUSB0_VBUS(void)
{
#ifdef EVAL_BOARD
   return GPIO_Word(3, 0);
#else
   return GPIO_Word(2, 0);
#endif
}

static int pinUSB1_VBUS(void)
{
#ifdef EVAL_BOARD
  return GPIO_Word(3, 1);
#else
  return GPIO_Word(3, 0);
#endif
}


static inline void USBPinsInit(void)
{
#ifdef EVAL_BOARD
  GPIO_DIR_IN(3, 0); SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  GPIO_DIR_IN(3, 1); SFSP(6, 2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
#else
  GPIO_DIR_IN(2, 0); SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  GPIO_DIR_IN(3, 0); SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#endif
}

// ------- Init all the pins -------
static inline void PINS_Init(void)
{
  debugPinsInit();
  USBPinsInit();
}

// clang-format on
