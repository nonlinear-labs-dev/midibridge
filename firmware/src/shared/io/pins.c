#include "pin_setup.h"

#ifdef EVAL_BOARD
#warning "Forcing Compile for Evaluation Board specifics (GPIOs for LEDs etc)"
#endif

#define PINS_LOCAL
#include "pins.h"
#undef PINS_LOCAL

volatile uint32_t* LEDs[6];        // LED0 RGB and LED1 RGB
volatile uint32_t* USBVBUS[2];     // USB Vbus 0 and 1
volatile uint32_t* DEBUG[2];       // DebugPins/LEDs 0 and 1
uint32_t           __dummy_pin__;  // dummy for unavailable pins

static uint8_t pbcaVersion = 0xFF;

static void setupPCBAtype(void);

uint8_t getPbcaVersion(void)
{
  if (pbcaVersion == 0xFF)
    setupPCBAtype();
  return pbcaVersion;
}

void PINS_Init(void)
{
  setupPCBAtype();

  // system LEDs are active LOW
  LED_RED0   = 0;
  LED_GREEN0 = 0;
  LED_BLUE0  = 0;
  LED_RED1   = 0;
  LED_GREEN1 = 0;
  LED_BLUE1  = 0;

  // Debug-LEDs/Pins are active low
  LED_DBG0 = 0;
  LED_DBG1 = 0;
}

// ------------- locals -----------------------

static void detectPCBAtype(void);
static void initEvalBoardPins(void);
static void initV1aPins(void);
static void initV2aPins(void);
static void setupPCBAtype(void)
{
  pbcaVersion = EVAL_BOARD;
#ifndef EVAL_BOARD
  detectPCBAtype();
#endif
  switch (pbcaVersion)
  {
    case EVAL_BOARD:
      initEvalBoardPins();
      break;
    case V1a:
      initV1aPins();
      break;
    case V2a:
      initV2aPins();
      break;
  }
}

static void Delay300us(void);

static void detectPCBAtype(void)
{
  /*
   Board ID Resistors:
   (1 means fitted
	ID_1 ID_0 Rev
	0    0    1a (or Eval when P4_0 is floating)
	0    1    2a
	1    0    tbd
	1    1    tbd
  */

  // P2_0 --> GPIO5[0] is ID_0 resistor input, use pullup
  GPIO_DIR_IN(5, 0);
  SFSP(2, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_DPD + SFS_EPU + 0;

  // P1_17 --> GPIO0[12] is ID_1 resistor input, use pullup
  GPIO_DIR_IN(0, 12);
  SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_DPD + SFS_EPU + 0;

  Delay300us();

  switch (((GPIO_Word(0, 12) == 0) << 1) | (GPIO_Word(5, 0) == 0))  // generate "fitted" bit pattern
  {
    case 0b00:  // V1a or eval
    {
      // check if pin P4_0 is floating --> Eval
      // Step 1 : pull it up
      GPIO_DIR_IN(2, 0);
      SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_DPD + SFS_EPU + 0;
      Delay300us();
      uint32_t P4_0_PU = GPIO_Word(2, 0);

      // Step, 2 pull it down
      SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_DPU + 0;
      Delay300us();
      uint32_t P4_0_PD = GPIO_Word(2, 0);

      if ((P4_0_PU != 0) && (P4_0_PD == 0))
      {  // pin followed pulls, so open circuit --> eval board
        pbcaVersion = EVAL_BOARD;
        break;
      }
      pbcaVersion = V1a;
      break;
    }

    case 0b01:  // V2a or larger
    case 0b10:
    case 0b11:
      pbcaVersion = V2a;
      break;
  }

  // free ID resistor pins for later use as test outputs in V2a++
  SFSP(2, 0)  = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_DPU + 0;
  SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_DPU + 0;
}

static void initEvalBoardPins(void)
{
  // RED0
  GPIO_DIR_OUT(0, 15);
  SFSP(1, 20) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[0]     = pGPIO_Word(0, 15);
  // GREEN0
  GPIO_DIR_OUT(0, 12);
  SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[1]     = pGPIO_Word(0, 12);
  // BLUE0
  GPIO_DIR_OUT(0, 2);
  SFSP(1, 15) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[2]     = pGPIO_Word(0, 2);
  // RED1
  GPIO_DIR_OUT(2, 13);
  SFSP(5, 4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[3]    = pGPIO_Word(2, 13);
  // GREEN1
  GPIO_DIR_OUT(1, 3);
  SFSP(1, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[4]     = pGPIO_Word(1, 3);
  // BLUE1
  GPIO_DIR_OUT(1, 9);
  SFSP(1, 6) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[5]    = pGPIO_Word(1, 9);

  // VBUS_0
  GPIO_DIR_IN(3, 0);
  SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  USBVBUS[0] = pGPIO_Word(3, 0);
  // VBUS_1
  GPIO_DIR_IN(3, 1);
  SFSP(6, 2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  USBVBUS[1] = pGPIO_Word(3, 1);

  // LED_DBG0
  GPIO_DIR_OUT(2, 3);
  SFSP(4, 3) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  DEBUG[0]   = pGPIO_Word(2, 3);
  // LED_DBG1
  GPIO_DIR_OUT(2, 6);
  SFSP(4, 4) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  DEBUG[1]   = pGPIO_Word(2, 6);
}

static void initV1aPins(void)
{
  // RED0
  GPIO_DIR_OUT(2, 9);
  SFSP(5, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[0]    = pGPIO_Word(2, 9);
  // GREEN0
  GPIO_DIR_OUT(1, 2);
  SFSP(1, 9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[1]    = pGPIO_Word(1, 2);
  // BLUE0
  GPIO_DIR_OUT(2, 11);
  SFSP(5, 2) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[2]    = pGPIO_Word(2, 11);
  // RED1
  GPIO_DIR_OUT(0, 14);
  SFSP(2, 10) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[3]     = pGPIO_Word(0, 14);
  // GREEN1
  GPIO_DIR_OUT(1, 11);
  SFSP(2, 11) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[4]     = pGPIO_Word(1, 11);
  // BLUE1
  GPIO_DIR_OUT(1, 10);
  SFSP(2, 9) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[5]    = pGPIO_Word(1, 10);

  // VBUS_0
  GPIO_DIR_IN(2, 0);
  SFSP(4, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  USBVBUS[0] = pGPIO_Word(2, 0);
  // VBUS_1
  GPIO_DIR_IN(3, 0);
  SFSP(6, 1) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  USBVBUS[1] = pGPIO_Word(3, 0);

  // LED_DBG0
  DEBUG[0] = &__dummy_pin__;
  // LED_DBG1
  DEBUG[1] = &__dummy_pin__;
}

static void initV2aPins(void)
{
  // RED0
  GPIO_DIR_OUT(1, 5);
  SFSP(1, 12) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[0]     = pGPIO_Word(1, 5);
  // GREEN0
  GPIO_DIR_OUT(0, 2);
  SFSP(1, 15) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[1]     = pGPIO_Word(0, 2);
  // BLUE0
  GPIO_DIR_OUT(1, 7);
  SFSP(1, 14) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[2]     = pGPIO_Word(1, 7);
  // RED1
  GPIO_DIR_OUT(1, 11);
  SFSP(2, 11) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[3]     = pGPIO_Word(1, 11);
  // GREEN1
  GPIO_DIR_OUT(1, 13);
  SFSP(2, 13) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  LEDs[4]     = pGPIO_Word(1, 13);
  // BLUE1
  GPIO_DIR_OUT(5, 5);
  SFSP(2, 5) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 4;
  LEDs[5]    = pGPIO_Word(5, 5);

  // VBUS_0
  GPIO_DIR_IN(5, 7);
  SFSP(2, 8) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 4;
  USBVBUS[0] = pGPIO_Word(5, 7);
  // VBUS_1
  GPIO_DIR_IN(0, 12);
  SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_EPD + SFS_EPU + 0;
  USBVBUS[1]  = pGPIO_Word(0, 12);

  // LED_DBG0
  GPIO_DIR_OUT(5, 0);
  SFSP(2, 0) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_DPD + SFS_EPU + 0;
  DEBUG[0]   = pGPIO_Word(5, 5);
  // LED_DBG1
  GPIO_DIR_IN(0, 12);
  SFSP(1, 17) = SFS_EIF + SFS_EIB + SFS_DHS + SFS_DPD + SFS_EPU + 0;
  DEBUG[1]    = pGPIO_Word(0, 12);
}

static void Delay300us(void)
{
  register uint32_t cnt = 100000;  // 100'000 * 10ns = 1ms @ 96kHz or 0.5ms @ 204MHz
  while (--cnt)
    asm volatile("nop");
}
