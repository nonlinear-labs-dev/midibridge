#include <stdint.h>

#include "io/pins.h"

uint32_t max = 1000000;

void ResetISR(void)
{
  while (1)
  {
    int32_t cntr = max;
    while (cntr--)
      ;
    *pLED_RED0   = !*pLED_RED0;
    *pLED_GREEN0 = !*pLED_GREEN0;
    *pLED_BLUE0  = !*pLED_BLUE0;
  }
}
