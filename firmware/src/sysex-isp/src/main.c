#include <stdint.h>
#include "sys/nl_stdlib.h"
#include "io/pins.h"
#include "sys/flash.h"
#include "CPU_clock.h"
#include "sys/incbin.h"

static inline void LedA(uint8_t const rgb)
{
  LED_RED0   = !(rgb & 0b001);
  LED_GREEN0 = !(rgb & 0b010);
  LED_BLUE0  = !(rgb & 0b100);
}

static inline void LedB(uint8_t const rgb)
{
  LED_RED1   = !(rgb & 0b001);
  LED_GREEN1 = !(rgb & 0b010);
  LED_BLUE1  = !(rgb & 0b100);
}

// the code actually to be flashed is included as an object
// at compile time from  final output file of dependent project "main".
#warning FIXME: make this work with linux automated build system
#ifdef LPCXPRESSO
INCBIN(Code, "../../main/Release/main.bin");
#else
INCBIN(Code, <some auto - generated path>);
#endif

__attribute__((section(".codeentry"))) int main(void)
{
  __disable_irq();
  LedA(0b001);
  LedB(0b000);
  CPU_ConfigureClocks();

  LedA(0b010);
  FLASH_Init();
  int fail = !flashMemory((uint32_t *) gCodeData, gCodeSize, 0);  // 36k to flash A

#define MAX (3000000ul);
  int toggle = 1;
  while (1)
  {
    uint32_t cntr = MAX;
    while (cntr--)
      asm volatile("nop");
    toggle = !toggle;
    if (toggle)
    {
      LedA(fail ? 0b001 : 0b010);
      LedB(fail ? 0b001 : 0b010);
    }
    else
    {
      LedA(0b000);
      LedB(0b000);
    }
  }
}
