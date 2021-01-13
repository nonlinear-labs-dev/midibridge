#include "sys/nl_version.h"
#include "drv/nl_leds.h"

#define MS250  (250000ul / 125)
#define MS500  (500000ul / 125)
#define MS1000 (1000000ul / 125)

int showFirmwareVersion(void)
{
  static int step  = 1;
  static int major = SW_VERSION_MAJOR;
  static int minor = SW_VERSION_MINOR;
  static int wait  = 0;

  if (step == 0)
    return 0;

  if (wait)
  {
    wait--;
    return 1;
  }

  switch (step)
  {
    case 1:
      LED_SetDirect(0, 0);
      LED_SetDirect(1, 0);
      wait = MS1000;
      step++;
      break;

    case 2:
      LED_SetDirect(0, 0b011);
      wait = MS500;
      step++;
      break;

    case 3:
      LED_SetDirect(0, 0b000);
      if (--major)
      {
        step = 2;
        wait = MS250;
      }
      else
      {
        step++;
        wait = MS1000;
      }
      break;

    case 4:
      LED_SetDirect(1, 0b110);
      wait = MS500;
      step++;
      break;

    case 5:
      LED_SetDirect(1, 0b000);
      if (--minor)
      {
        step = 4;
        wait = MS250;
      }
      else
      {
        step = 0;
        wait = MS1000;
      }
      break;
  }
  return 1;  // still displaying
}
