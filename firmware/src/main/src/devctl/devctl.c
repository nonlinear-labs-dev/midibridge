#include "midi/nl_devctl_defs.h"
#include "devctl/devctl.h"
#include "drv/nl_leds.h"
#include "sys/nl_stdlib.h"

static int memcmp(uint8_t const* const p, uint8_t const* const q, uint32_t const lenP, uint32_t const lenQ)
{
  if (lenP != lenQ)
    return 0;
  for (int i = 0; i < lenP; i++)
    if (p[i] != q[i])
      return 0;
  return 1;
}

static inline void error(LedColor_t const color)
{
  int toggle = 0;
  __disable_irq();
  LED_SetDirect(0, COLOR_WHITE);
  while (1)
  {
    toggle = !toggle;
    LED_SetDirect(1, toggle ? color : COLOR_OFF);

    uint32_t cntr = 1000000ul;
    while (cntr--)
      asm volatile("nop");
  }
}

#define CODE_START (0x10080000)  // Pointer to RamLoc40 at 0x10080000,
#define CODE_SIZE  (0xA000)      // RamLoc40 is 40kB
static uint8_t* code = (uint8_t*) CODE_START;
#warning uploaded application must have its code entry point at 0x10080000 (==RamLoc40) !

static void execute(void)
{
  typedef void (*downloadCode_t)(void);
  downloadCode_t execStart;

  execStart = (downloadCode_t)(CODE_START + 1);  // this must be code entry adr + 1 (ARM weirdness)
  (*execStart)();
  error(COLOR_GREEN);
}

// ----------------------------------------------
// return 1 when a sysex header is found in the buffer that is for us
// Buff and Len are updated to point/count to the payload after the signature
int DEVCTL_isDeviceControlMsg(uint8_t** const pBuff, uint32_t* const pLen)
{
  uint32_t const ID_SIZE = sizeof NLMB_DevCtlSignature_RAW;
  if ((*pLen >= ID_SIZE) && memcmp(*pBuff, NLMB_DevCtlSignature_RAW, ID_SIZE, ID_SIZE))
  {
    *pBuff += ID_SIZE;
    *pLen -= ID_SIZE;
    return 1;
  }
  return 0;
}

static inline void parseEncodedByte(uint8_t byte)
{
  if (byte >= 0x80 && byte != 0xF7)  // illegal content will kill us
    error(COLOR_CYAN);

  if (byte == 0xF7)  // end of SysEx ?
    execute();       // will NOT return !

  static uint8_t topBitsMask;
  static uint8_t topBits;

  if (topBitsMask == 0)
  {
    topBitsMask = 0x40;  // reset top bit mask to first bit (bit6)
    topBits     = byte;  // save top bits
  }
  else
  {
    if (topBits & topBitsMask)
      byte |= 0x80;  // set top bit when required
    topBitsMask >>= 1;

    if ((code - (uint8_t*) CODE_START) + 1 > CODE_SIZE)
      error(COLOR_MAGENTA);  // buffer would overrun
    *(code++) = byte;
  }
}

void DEVCTL_init(void)
{  // clear code buffer to have all data-segment uninitialized variables ("bss") zeroed
  memset(code, 0, CODE_SIZE);
}

void DEVCTL_processMsg(uint8_t* buff, uint32_t len)
{
  while (len >= 4)  // more raw USB packets ?
  {
    len -= 4;

    // check for a sysex related packet
    switch (*buff & 0x0F)  // mask out channel number
    {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:  // sysex type
        break;
      default:  // non-sysex will kill us
        error(COLOR_CYAN);
    }

    // determine payload size of packet
    uint8_t payload = 0;
    switch (*buff & 0x0F)  // mask out channel number
    {
      case 0x05:  // single byte packets
        payload = 1;
        break;
      case 0x06:  // two-byte packets
        payload = 2;
        break;
      default:  // three-byte packets
        payload = 3;
        break;
    }

    for (int i = 1; i <= payload; i++)
      parseEncodedByte(buff[i]);

    buff += 4;  // next raw USB packet
  }
}
