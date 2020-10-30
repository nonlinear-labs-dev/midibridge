#include <stdint.h>

#include "io/pins.h"

void ResetISR(void)
{
  int toggle = 1;

  // __disable_irq();
  while (1)
  {
    uint32_t cntr = 1000000;
    while (cntr--)
      asm volatile("nop");
    toggle       = !toggle;
    *pLED_RED0   = toggle;
    *pLED_GREEN0 = toggle;
    *pLED_BLUE0  = toggle;
    *pLED_RED1   = toggle;
    *pLED_GREEN1 = toggle;
    *pLED_BLUE1  = toggle;
  }
}
