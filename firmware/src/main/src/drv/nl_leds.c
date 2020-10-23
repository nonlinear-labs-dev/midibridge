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

static uint16_t       pwmBufferIndex;
static uint8_t const *pwmBuffer[2];

#define PWM_LENGTH (5)
// clang-format off
static uint8_t const pwmColorTable[27][PWM_LENGTH] = {  // NOTE: drive is inverted
  { ~0b000, ~0b000, ~0b000, ~0b000, ~0b000 },  // 0 : R0 G0 B0
  { ~0b001, ~0b000, ~0b000, ~0b000, ~0b000 },  // 1 : R1 G0 B0
  { ~0b001, ~0b001, ~0b001, ~0b001, ~0b001 },  // 2 : R2 G0 B0

  { ~0b010, ~0b000, ~0b000, ~0b000, ~0b000 },  // 3 : R0 G1 B0
  { ~0b001, ~0b010, ~0b000, ~0b000, ~0b000 },  // 4 : R1 G1 B0
  { ~0b001, ~0b001, ~0b011, ~0b001, ~0b001 },  // 5 : R2 G1 B0

  { ~0b010, ~0b010, ~0b010, ~0b010, ~0b010 },  // 6 : R0 G2 B0
  { ~0b011, ~0b010, ~0b010, ~0b010, ~0b010 },  // 7 : R1 G2 B0
  { ~0b011, ~0b011, ~0b011, ~0b011, ~0b011 },  // 8 : R2 G2 B0

  { ~0b000, ~0b000, ~0b100, ~0b000, ~0b000 },  // 9 : R0 G0 B1
  { ~0b001, ~0b000, ~0b100, ~0b000, ~0b000 },  // 10: R1 G0 B1
  { ~0b001, ~0b001, ~0b101, ~0b001, ~0b001 },  // 11: R2 G0 B1

  { ~0b010, ~0b000, ~0b100, ~0b000, ~0b000 },  // 12: R0 G1 B1
  { ~0b001, ~0b010, ~0b100, ~0b000, ~0b000 },  // 13: R1 G1 B1
  { ~0b001, ~0b001, ~0b011, ~0b101, ~0b001 },  // 14: R2 G1 B1

  { ~0b110, ~0b010, ~0b010, ~0b010, ~0b010 },  // 15: R0 G2 B1
  { ~0b011, ~0b010, ~0b110, ~0b010, ~0b010 },  // 16: R1 G2 B1
  { ~0b011, ~0b011, ~0b011, ~0b011, ~0b111 },  // 17: R2 G2 B1

  { ~0b100, ~0b100, ~0b100, ~0b100, ~0b100 },  // 18: R0 G0 B2
  { ~0b101, ~0b100, ~0b100, ~0b100, ~0b100 },  // 19: R1 G0 B2
  { ~0b101, ~0b101, ~0b101, ~0b101, ~0b101 },  // 20: R2 G0 B2

  { ~0b110, ~0b100, ~0b100, ~0b100, ~0b100 },  // 21: R0 G1 B2
  { ~0b101, ~0b110, ~0b100, ~0b100, ~0b100 },  // 22: R1 G1 B2
  { ~0b101, ~0b101, ~0b011, ~0b101, ~0b101 },  // 23: R2 G1 B2

  { ~0b110, ~0b110, ~0b110, ~0b110, ~0b110 },  // 24: R0 G2 B2
  { ~0b111, ~0b110, ~0b110, ~0b110, ~0b110 },  // 25: R1 G2 B2
  { ~0b111, ~0b111, ~0b111, ~0b111, ~0b111 },  // 26: R2 G2 B2
};
// clang-format on

void LED_SetColor(uint8_t const ledId, uint8_t const color)
{
  pwmBuffer[ledId & 1] = &(pwmColorTable[color & 0b111111][0]);
}

// clang-format off
static uint8_t colorTable[2][5] = {
  {
    0,   // dim OFF
	9,   // dim BLUE
	3,   // dim GREEN
	4,   // dim ORANGE
	1,   // dim RED
  },
  {
    0,   // bright OFF
	18,  // bright BLUE
	6,   // bright GREEN
	8,   // bright ORANGE
	2,   // bright RED
  },
};
// clang-format on

void LED_SetColorByParam(uint8_t const ledId, uint8_t const baseColor, uint8_t const intensity)
{
  if (baseColor == 0 || intensity == 0)
    LED_SetColor(ledId, 0);
  else
    LED_SetColor(ledId, colorTable[(intensity - 1) & 1][baseColor & 7]);
}

/******************************************************************************/
/** @brief    	PWM-Control for LEDs
*******************************************************************************/
void LED_ProcessPWM(void)  // call-rate > 1kHz (<1ms) for low flicker
{
  uint8_t pattern;

  pattern      = pwmBuffer[0][pwmBufferIndex];
  *pLED_REDa   = pattern & 0x01;
  *pLED_GREENa = pattern & 0x02;
  *pLED_BLUEa  = pattern & 0x04;

  pattern      = pwmBuffer[1][pwmBufferIndex];
  *pLED_REDb   = pattern & 0x01;
  *pLED_GREENb = pattern & 0x02;
  *pLED_BLUEb  = pattern & 0x04;

  if (++pwmBufferIndex >= PWM_LENGTH)
    pwmBufferIndex = 0;
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
    uint8_t on = 1;
    if (ledState[ledId].flickering)
      on = toggle;

    uint8_t intensity = 0;
    if (ledState[ledId].bright)
    {
      if (on)
        intensity = 2;
      else
        intensity = 1;
    }
    else
    {
      if (on)
        intensity = 1;
      else
        intensity = 0;
    }
    LED_SetColorByParam(ledId, ledState[ledId].baseColor, intensity);
  }
}

void LED_Init(void)
{
  LED_SetColor(0, 0);
  LED_SetColor(1, 0);
}
