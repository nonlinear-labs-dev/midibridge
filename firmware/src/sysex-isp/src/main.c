#include <stdint.h>
#include "io/pins.h"

extern unsigned int _startOfAttachment;

__attribute__((section(".codeentry"))) int main(void)
{
#define MAX (1000000ul);
  int toggle = 1;
  __disable_irq();
  while (1)
  {
    uint32_t cntr = MAX;
    while (cntr--)
      asm volatile("nop");
    toggle     = !toggle;
    LED_RED0   = toggle;
    LED_GREEN0 = toggle;
    LED_BLUE0  = toggle;
    LED_RED1   = toggle;
    LED_GREEN1 = toggle;
    LED_BLUE1  = toggle;
  }
}
