#include "midi/nl_midi_isp.h"
#include "midi/nl_sysex.h"
#include "midi/nl_ispmarkers.h"
#include "drv/nl_leds.h"

#define CODE_START (0x10000000)
static uint8_t* code = (uint8_t*) CODE_START;

#define A (0)
#define B (1)

static inline int memcmp(uint8_t const* const p, uint8_t const* const q, uint32_t const lenP, uint32_t const lenQ)
{
  if (lenP != lenQ)
    return 0;
  for (int i = 0; i < lenP; i++)
    if (p[i] != q[i])
      return 0;
  return 1;
}
static void showIspStart(void)
{
  LED_SetState(A, COLOR_BLUE, 0, 1);
  LED_SetState(B, COLOR_BLUE, 0, 1);
}

static void showIspEnd(void)
{
  LED_SetState(A, COLOR_GREEN, 0, 1);
  LED_SetState(B, COLOR_GREEN, 0, 1);
}

static void showIspFill(void)
{
  LED_SetState(A, COLOR_ORANGE, 0, 1);
  LED_SetState(B, COLOR_ORANGE, 0, 1);
}

static void showIspExecute(void)
{
  LED_SetState(A, COLOR_RED, 0, 1);
  LED_SetState(B, COLOR_RED, 0, 1);
}

static void showIspError(void)
{
  LED_SetState(A, COLOR_RED, 1, 1);
  LED_SetState(B, COLOR_RED, 1, 1);
}

// return number of bytes of encoding
static uint16_t encode8bitData(uint8_t const* src, uint32_t len, uint8_t* const dest)
{
  uint8_t* dst         = (uint8_t*) dest;
  uint8_t  topBitsMask = 0;
  uint8_t* topBits;

  while (len)
  {
    if (topBitsMask == 0)  // need to start insert a new top bits byte ?
    {
      topBitsMask = 0x40;   // reset top bit mask to first bit (bit6)
      topBits     = dst++;  // save top bits position...
      *topBits    = 0;      // ...and clear top bits
    }
    else  // regular byte
    {
      uint8_t byte = *(src++);
      *(dst++)     = byte & 0x7F;
      if (byte & 0x80)
        *topBits |= topBitsMask;  // add in top bit for this byte
      topBitsMask >>= 1;
      len--;
    }
  }

  return dst - dest;  // total bytes written to encoded data buffer;
}

// ----------- the main command scheduler
// return == 0 only with illegal/unknown command or when an executed command wants to terminate command mode
static int commandCallback(uint16_t const commandID, uint8_t const* const data, uint32_t const len)
{
  switch (commandID)
  {
    case ISP_COMMAND_GET_FIRMWARE_ID:
      return 1;
    case ISP_COMMAND_START_ISP_DATA:
      showIspStart();
      return 1;
    case ISP_COMMAND_ISP_DATA_BLOCK:
      showIspFill();
      return 1;
    case ISP_COMMAND_END_ISP_DATA:
      showIspEnd();
      return 1;
    case ISP_COMMAND_EXECUTE_ISP_DATA:
      showIspExecute();
      return 1;
    case ISP_COMMAND_END_COMMAND_MODE:
      return 0;
    default:
      showIspError();
      return 1;
  }
}

// return == 0 only with illegal command or when an executed command wants to terminate command mode
static int sysexCommandParser(uint8_t const* const buff, uint32_t const len)
{
  // expected buffer contents :
  // F0 00 4E 4C cmdID-L cmdID-H [(possibly encoded) data bytes] F7

  if ((len < 7)
      || (buff[0] != 0xF0)
      || (buff[1] != MMID0)
      || (buff[2] != MMID1)
      || (buff[3] != MMID2))
    return 0;

  uint16_t const commandID = (uint16_t) buff[4] + (((uint16_t) buff[5]) << 8);
  return commandCallback(commandID, &buff[6], len - 7);
}

// return != 0 when a sysex header is found in the buffer that is for us
int ISP_isOurCommand(uint8_t const* const buff, uint32_t const len)
{
  uint32_t const ID_SIZE = sizeof NLMB_ManuID_RAW;
  if ((len >= ID_SIZE) && memcmp(buff, NLMB_ManuID_RAW, ID_SIZE, ID_SIZE))
    return 1;
  return 0;
}

// return == 0 only when an executed command wants to terminate command mode
int ISP_collectAndExecuteCommand(uint8_t const* buff, uint32_t len)
{
  static uint8_t dearmed;
  static uint8_t sysexStart, sysexStop;

  static uint8_t  buffer[1024];
  static uint8_t* sysexBuffer      = buffer;
  static uint8_t* sysexBufferStart = buffer;

  if (dearmed)
    return 0;

  int result = 1;

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
      default:  // non-sysex related will terminate us
        dearmed = 1;
        return 0;
    }

    // determine payload size of packet
    uint8_t payload = 0;
    switch (*buff & 0x0F)  // mask out channel number
    {
      case 0x05:
      case 0x0F:  // single byte packets
        payload = 1;
        break;
      case 0x02:
      case 0x06:
      case 0x0C:
      case 0x0D:  // two-byte packets
        payload = 2;
        break;
      default:  // three-byte packets
        payload = 3;
        break;
    }

    // add payload into packed sysex buffer
    uint16_t index = 1;
    while (payload--)
    {
      uint8_t const byte = buff[index++];

      if (byte == 0xF0)
      {
        sysexStart  = 1;
        sysexBuffer = sysexBufferStart;  // reset sysex buffer
      }
      else if (byte == 0xF7)
        sysexStop = 1;

      if (sysexStart)
        *(sysexBuffer++) = byte;  // collect byte

      if (sysexStart && sysexStop)  // sysex end, let callback analyse it
      {
        result &= sysexCommandParser(sysexBufferStart, sysexBuffer - sysexBufferStart);
        sysexStart = sysexStop = 0;
      }
    }

    buff += 4;  // next raw USB packet
  }
  return result;
}

static inline void unpackUsbRawMidi(uint8_t* src, uint8_t* dest, uint32_t len)
{
  while (len >= 4)
  {
    src++;                 // skip USB packet ID
    *(dest++) = *(src++);  // copy 3 payload bytes
    *(dest++) = *(src++);
    *(dest++) = *(src++);
    len -= 4;
  }
}

#if 0
int ISP_Execute(void)
{
  typedef void (*downloadCode_t)(void);
  downloadCode_t execStart;

#warning uploaded application must have its code entry point at 0x10000000 !
  execStart = (downloadCode_t)(0x10000000 + 1);  // Pointer to RamLoc32 at 0x1000000, this is code entry adr + 1 (!!!!)

  if (error || !data)
    return 0;
  (*execStart)();
  return 1;
}
#endif

void ISP_Init(void)
{
}
