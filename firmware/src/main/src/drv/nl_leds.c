/******************************************************************************/
/** @file		nl_leds.c
    @brief	PWM LED low-level handling
*******************************************************************************/
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"

#if USBA_PORT_FOR_MIDI == 0
#define pLED_REDa   (pLED_RED0)
#define pLED_GREENa (pLED_GREEN0)
#define pLED_BLUEa  (pLED_BLUE0)
#else
#define pLED_REDa   (pLED_RED1)
#define pLED_GREENa (pLED_GREEN1)
#define pLED_BLUEa  (pLED_BLUE1)
#endif

#if USBB_PORT_FOR_MIDI == 0
#define pLED_REDb   (pLED_RED0)
#define pLED_GREENb (pLED_GREEN0)
#define pLED_BLUEb  (pLED_BLUE0)
#else
#define pLED_REDb   (pLED_RED1)
#define pLED_GREENb (pLED_GREEN1)
#define pLED_BLUEb  (pLED_BLUE1)
#endif

void LED_SetDirectAndHalt(uint8_t const rgb)
{
  *pLED_RED0 = *pLED_RED1 = !(rgb & 0b001);
  *pLED_GREEN0 = *pLED_GREEN1 = !(rgb & 0b010);
  *pLED_BLUE0 = *pLED_BLUE1 = !(rgb & 0b100);

  __disable_irq();
  while (1)
    ;
}

typedef struct __attribute__((__packed__))
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGB_t;

static uint16_t     pwmBufferBit;
static RGB_t const *pwmBuffer[2];

// clang-format off
static RGB_t const pwmColorTable[] = {
  {  // 0  : all OFF
    .r = 0b00000000,
    .g = 0b00000000,
    .b = 0b00000000 },
  {  // 1  : very dim red (1/8)
	.r = 0b10000000,
	.g = 0b00000000,
	.b = 0b00000000 },
  {  // 2  : dim red (2/8)
	.r = 0b10001000,
	.g = 0b00000000,
	.b = 0b00000000 },
  {  // 3  : bright red (5/8)
	.r = 0b11101010,
	.g = 0b00000000,
	.b = 0b00000000 },
  {  // 4  : very bright red (8/8)
	.r = 0b11111111,
	.g = 0b00000000,
	.b = 0b00000000 },
  {  // 5  : very dim green (1/8)
	.r = 0b00000000,
	.g = 0b01000000,
	.b = 0b00000000 },
  {  // 6  : dim green (2/8)
	.r = 0b00000000,
	.g = 0b01000100,
	.b = 0b00000000 },
  {  // 7  : bright green 5/8)
	.r = 0b00000000,
	.g = 0b01110101,
	.b = 0b00000000 },
  {  // 8  : very bright green (8/8)
	.r = 0b00000000,
	.g = 0b11111111,
	.b = 0b00000000 },
  {  // 9  : very dim blue (1/8)
	.r = 0b00000000,
	.g = 0b00000000,
	.b = 0b00100000 },
  {  // 10 : dim blue (2/8)
	.r = 0b00000000,
	.g = 0b00000000,
	.b = 0b00100010 },
  {  // 11 : bright blue 5/8)
	.r = 0b00000000,
	.g = 0b00000000,
	.b = 0b01011011 },
  {  // 12 : very bright blue (8/8)
	.r = 0b00000000,
	.g = 0b00000000,
	.b = 0b11111111 },
  {  // 13 : very dim orange (1/8)
	.r = 0b10000000,
	.g = 0b00001000,
	.b = 0b00000000 },
  {  // 14 : dim orange (2/8)
	.r = 0b10001000,
	.g = 0b00100010,
	.b = 0b00000000 },
  {  // 15 : bright orange 5/8)
	.r = 0b11101010,
	.g = 0b01011101,
	.b = 0b00000000 },
  {  // 16 : very bright orange (8/8)
	.r = 0b11111111,
	.g = 0b11111111,
	.b = 0b00000000 },
};
// clang-format on

void LED_SetColor(uint8_t const ledId, uint8_t const color)
{
  pwmBuffer[ledId] = &(pwmColorTable[color]);
}

// clang-format off
static uint8_t colorTable[4][5] = {
  {
	9,   // very dim BLUE
	5,   // very dim GREEN
	13,  // very dim ORANGE
	1,   // very dim RED
  },
  {
	10,  // dim BLUE
	6,   // dim GREEN
	14,  // dim ORANGE
	2,   // dim RED
  },
  {
	11,  // bright BLUE
	7,   // bright GREEN
	15,  // bright ORANGE
	3,   // bright RED
  },
  {
	12,  // very bright BLUE
	8,   // very bright GREEN
	16,  // very bright ORANGE
	4,   // very bright RED
  },
};
// clang-format on

void LED_SetColorByParam(uint8_t const ledId, uint8_t const baseColor, uint8_t const intensity)
{
  if (baseColor == 0 || intensity == 0)
    LED_SetColor(ledId, 0);
  else
    LED_SetColor(ledId, colorTable[intensity - 1][baseColor - 1]);
}

/******************************************************************************/
/** @brief    	PWM-Control for LEDs
*******************************************************************************/
void LED_ProcessPWM(void)  // call-rate > 1kHz (<1ms) for low flicker
{
  pwmBufferBit >>= 1;
  if (pwmBufferBit == 0)
    pwmBufferBit = 0x80;

  *pLED_REDa   = (0 == (pwmBuffer[0]->r & pwmBufferBit));
  *pLED_GREENa = (0 == (pwmBuffer[0]->g & pwmBufferBit));
  *pLED_BLUEa  = (0 == (pwmBuffer[0]->b & pwmBufferBit));

  *pLED_REDb   = (0 == (pwmBuffer[1]->r & pwmBufferBit));
  *pLED_GREENb = (0 == (pwmBuffer[1]->g & pwmBufferBit));
  *pLED_BLUEb  = (0 == (pwmBuffer[1]->b & pwmBufferBit));
}

typedef struct
{
  uint8_t flickering;  // flickering / solid
  uint8_t bright;      // bright / dimmed
  uint8_t baseColor;   // 3bits RGB
} LedState_t;
static LedState_t ledState[2];

void LED_SetState(uint8_t const ledId, uint8_t const baseColor, uint8_t const bright, uint8_t const flickering)
{
  ledState[ledId].baseColor  = baseColor;
  ledState[ledId].bright     = bright;
  ledState[ledId].flickering = flickering;
}

void LED_GetState(uint8_t const ledId, uint8_t *const baseColor, uint8_t *const bright, uint8_t *const flickering)
{
  *baseColor  = ledState[ledId].baseColor;
  *bright     = ledState[ledId].bright;
  *flickering = ledState[ledId].flickering;
}

static uint32_t rand32 = 0x12345678;
static uint32_t xorshift_u32(void)
{  // 32 bit xor-shift generator, period 2^32-1, non-zero seed required, range 1...2^32-1
   // source : https://www.jstatsoft.org/index.php/jss/article/view/v008i14/xorshift.pdf
  rand32 ^= rand32 << 13;
  rand32 ^= rand32 >> 17;
  rand32 ^= rand32 << 5;
  return rand32;
}

static inline uint8_t randomBit(void)
{
  static uint8_t bit;
  bit <<= 1;
  do
  {
    if (xorshift_u32() & 1)
      bit |= 1;
    else
      bit &= ~1;
  } while ((bit & 7) == 0 || (bit & 7) == 7);
  return bit & 1;
}

/******************************************************************************/
/** @brief    	Handling the LED state machines, call every 25ms or so
*******************************************************************************/
void LED_Process(void)
{
  static uint8_t toggle;

  toggle ^= 1;
  for (int ledId = 0; ledId < 2; ledId++)
  {
    uint8_t flicker = 1;
    if (ledState[ledId].flickering)
      flicker = toggle;

    uint8_t intensity = 0;
    if (ledState[ledId].bright)
    {
      if (flicker)
        intensity = 4;
      else
        intensity = 0;
    }
    else
    {
      if (flicker)
        intensity = 1;
      else
        intensity = 2;
    }
    LED_SetColorByParam(ledId, ledState[ledId].baseColor, intensity);
  }
}

void LED_Init(void)
{
  LED_SetColor(0, 0);
  LED_SetColor(1, 0);
}
