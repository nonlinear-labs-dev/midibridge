#include "midi/nl_midi_isp.h"
#include "midi/nl_sysex.h"
#include "midi/nl_ispmarkers.h"

#define CODE_START (0x10000000)
static uint8_t* code = (uint8_t*) CODE_START;

static inline int memcmp(uint8_t const* const p, uint8_t const* const q, uint32_t const lenP, uint32_t const lenQ)
{
  if (lenP != lenQ)
    return 0;
  for (int i = 0; i < lenP; i++)
    if (p[i] != q[i])
      return 0;
  return 1;
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

int ISP_isIspStart(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_START_RAW, len, sizeof ISP_START_RAW);
}

int ISP_isIspEnd(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_END_RAW, len, sizeof ISP_END_RAW);
}

int ISP_isIspExecute(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_EXECUTE_RAW, len, sizeof ISP_EXECUTE_RAW);
}

int ISP_isIspInfo(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_INFO_RAW, len, sizeof ISP_INFO_RAW);
}

static int error;
static int data;

int ISP_FillData(uint8_t const* const buff, uint32_t const len)
{
  uint16_t decodedLen;
  if (error || !(decodedLen = MIDI_decodeRawSysex(buff, len, code)))
  {
    error = 1;
    return 0;
  }
  code += decodedLen;
  data = 1;
  return 1;
}

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

void ISP_Init(void)
{
}
