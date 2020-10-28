#include "midi/nl_midi_isp.h"
#include "midi/nl_sysex.h"

// clang-format off

// "NLMBISPS"
// sysex: f0 4e 4c 4d 42 49 53 50 53 f7
static const uint8_t ISP_START[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'S',
  0x05, 0xF7,  0,  0
};

// "NLMBISPE"
// sysex: f0 4e 4c 4d 42 49 53 50 45 f7
static const uint8_t ISP_END[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'E',
  0x05, 0xF7, 0,   0
};

// "NLMBISPX"
// sysex: f0 4e 4c 4d 42 49 53 50 58 f7
static const uint8_t ISP_EXECUTE[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'X',
  0x05, 0xF7, 0,   0
};

// "NLMBISPI"
// sysex: f0 4e 4c 4d 42 49 53 50 49 f7
static const uint8_t ISP_INFO[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'I',
  0x05, 0xF7, 0,   0
};

// clang-format on

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
  return memcmp(buff, ISP_START, len, sizeof ISP_START);
}

int ISP_isIspEnd(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_END, len, sizeof ISP_END);
}

int ISP_isIspExecute(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_EXECUTE, len, sizeof ISP_EXECUTE);
}

int ISP_isIspInfo(uint8_t const* const buff, uint32_t const len)
{
  return memcmp(buff, ISP_INFO, len, sizeof ISP_INFO);
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
