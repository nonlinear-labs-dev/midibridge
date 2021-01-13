#include "sys/nl_version.h"
#include "drv/nl_leds.h"

#define SHORT  (250000ul / 125)
#define MEDIUM (500000ul / 125)
#define LONG   (1000000ul / 125)

int showFirmwareVersion(void)
{
  static int step  = 1;
  static int major = SW_VERSION_MAJOR;
  static int minor = SW_VERSION_MINOR_H * 10 + SW_VERSION_MINOR_L;
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
      wait = LONG;
      step++;
      break;

    case 2:
      LED_SetDirect(0, 0b011);
      wait = MEDIUM;
      step++;
      break;

    case 3:
      LED_SetDirect(0, 0b000);
      if (--major)
      {
        step = 2;
        wait = MEDIUM;
      }
      else
      {
        step++;
        wait = LONG;
      }
      break;

    case 4:
      LED_SetDirect(1, 0b110);
      wait = MEDIUM;
      step++;
      break;

    case 5:
      LED_SetDirect(1, 0b000);
      if (--minor)
      {
        step = 4;
        wait = MEDIUM;
      }
      else
      {
        step = 6;
        wait = LONG;
      }
      break;

    case 6:
      step = 0;  // done
      break;
  }
  return 1;  // still displaying
}
