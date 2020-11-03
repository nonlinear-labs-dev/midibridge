#include <stdint.h>
#include "midi/nl_ispmarkers.h"
#include "io/pins.h"

extern unsigned int _startOfAttachment;

int a;
int b = 3;

#define NOP asm volatile("nop")

__attribute__((section(".codeentry"))) int main(void)
{
  uint16_t len =  sizeof ISP_INFO_RAW;
  uint8_t  buffer[len];
  for (int i; i < len; i++)
    buffer[i] = ISP_INFO_RAW[i];
  *pLED_BLUE0 = buffer[0] + a + b;
  NOP;
  NOP;
  *pLED_BLUE1 = *((uint32_t *) _startOfAttachment);
  NOP;
  NOP;

#if 01
  static int MAX = 1000000;
  int toggle = 1;

  // __disable_irq();
  while (1)
  {
    NOP;
    uint32_t cntr = MAX;
    NOP;
    cntr--;
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
#endif
}
