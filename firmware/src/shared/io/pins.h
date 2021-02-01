#pragma once

#include "pin_setup.h"

#ifdef EVAL_BOARD
#warning Compiling for Evaluation Board specifics (GPIOs for LEDs etc)
#endif

// clang-format off

//
// --------------- LEDs ----------------

// System RGB-LEDs (those are active-low!)
#ifdef EVAL_BOARD

#define LED_A_init() GPIO_DIR_OUT(0, 15); SFSP(1, 20) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_A           GPIO_Word(0, 15)
#define LED_B_init() GPIO_DIR_OUT(0, 12); SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_B           GPIO_Word(0, 12)
#define LED_C_init() GPIO_DIR_OUT(0,  2); SFSP(1, 15) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_C           GPIO_Word(0,  2)

#define LED_D_init() GPIO_DIR_OUT(2, 13); SFSP(5,  4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_D           GPIO_Word(2, 13)
#define LED_E_init() GPIO_DIR_OUT(1,  3); SFSP(1, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_E           GPIO_Word(1,  3)
#define LED_F_init() GPIO_DIR_OUT(1,  9); SFSP(1, 6) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_F           GPIO_Word(1,  9)

#else

#define LED_A_init() GPIO_DIR_OUT(2,  9); SFSP(5,  0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_A           GPIO_Word(2,  9)
#define LED_B_init() GPIO_DIR_OUT(1,  2); SFSP(1,  9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_B           GPIO_Word(1,  2)
#define LED_C_init() GPIO_DIR_OUT(2, 11); SFSP(5,  2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_C           GPIO_Word(2, 11)

#define LED_D_init() GPIO_DIR_OUT(0, 14); SFSP(2, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_D           GPIO_Word(0, 14)
#define LED_E_init() GPIO_DIR_OUT(1, 11); SFSP(2, 11) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_E           GPIO_Word(1, 11)
#define LED_F_init() GPIO_DIR_OUT(1, 10); SFSP(2,  9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_F           GPIO_Word(1, 10)

#endif

// LED assignments :
#define LED_RED0    LED_A
#define LED_GREEN0  LED_B
#define LED_BLUE0   LED_C
#define LED_RED1    LED_D
#define LED_GREEN1  LED_E
#define LED_BLUE1   LED_F

//
// Debug-LEDs (those are active-high)
#ifdef EVAL_BOARD

#define LED_DBG1_init() GPIO_DIR_OUT(2, 0); SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define LED_DBG1           GPIO_Word(2, 0)
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
  LED_A = 0;
  LED_B = 0;
  LED_C = 0;
  LED_D = 0;
  LED_E = 0;
  LED_F = 0;

  LED_DBG1_init();
  LED_DBG2_init();
  LED_DBG3_init();
  LED_DBG1 = 0;
  LED_DBG2 = 0;
  LED_DBG3 = 0;
}


// --------------- USB VBUS-Monitors ----------------
#ifdef EVAL_BOARD

#define pinUSB0_VBUS_init() GPIO_DIR_IN(3, 0); SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB0_VBUS          GPIO_Word(3, 0)
#define pinUSB1_VBUS_init() GPIO_DIR_IN(3, 1); SFSP(6, 2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB1_VBUS          GPIO_Word(3, 1)

#else

#define pinUSB0_VBUS_init() GPIO_DIR_IN(2, 0); SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB0_VBUS          GPIO_Word(2, 0)
#define pinUSB1_VBUS_init() GPIO_DIR_IN(3, 0); SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0
#define pinUSB1_VBUS          GPIO_Word(3, 0)

#endif

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
