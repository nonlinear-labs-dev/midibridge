#include "midi/nl_midi_isp.h"
#include "midi/nl_sysex.h"
#include "midi/nl_ispmarkers.h"

typedef void (*downloadCode_t)(void);

static downloadCode_t* execStart = (downloadCode_t*) 0x10000014;  // Pointer to RamLoc32 at 0x1000014, code entry
static uint8_t*        code      = (uint8_t*) 0x10000000;         // Pointer to RamLoc32 at 0x1000000

static inline int memcmp(uint8_t const* const p, uint8_t const* const q, uint32_t const lenP, uint32_t const lenQ)
{
  if (lenP != lenQ)
    return 0;
  for (int i = 0; i < lenP; i++)
    if (p[i] != q[i])
      return 0;
  return 1;
}

int ISP_isIspStart(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_START_RAW, len, ISP_getMarkerSize(ISP_START_RAW));
}

int ISP_isIspEnd(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_END_RAW, len, ISP_getMarkerSize(ISP_END_RAW));
}

int ISP_isIspExecute(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_EXECUTE_RAW, len, ISP_getMarkerSize(ISP_EXECUTE_RAW));
}

int ISP_isIspInfo(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_INFO_RAW, len, ISP_getMarkerSize(ISP_INFO_RAW));
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
  if (error || !data)
    return 0;
  (*execStart)();
  return 1;
}
