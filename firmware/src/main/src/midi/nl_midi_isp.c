#include "midi/nl_midi_isp.h"

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
static const uint8_t ISP_EXEXCUTE[] = {
  0x04, 0xF0, 'N', 'L',
  0x04,  'M', 'B', 'I',
  0x04,  'S', 'P', 'X',
  0x05, 0xF7, 0,   0
};
// clang-format on

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
  return memcmp(buff, ISP_EXEXCUTE, len, sizeof ISP_EXEXCUTE);
}

static int error;
static int data;

int ISP_FillData(uint8_t const* const buff, uint32_t const len)
{
  if (error || len < 4 || buff[1] != 0xF0)
  {
    error = 1;
    return 0;
  }
  data = 1;
  return 1;
}

int ISP_Execute(void)
{
  if (error || !data)
    return 0;
  return 1;
}
