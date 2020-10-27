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

typedef void (*downloadCode_t)(void);

static downloadCode_t* execStart = (downloadCode_t*) 0x10000014;  // Pointer to RamLoc32 at 0x1000014, code entry
static uint8_t*        code      = (uint8_t*) 0x10000000;         // Pointer to RamLoc32 at 0x1000000

// we assume a sysex stream that is organized in blocks like this:
// top-bits byte0 [byte1 byte2 byte3 byte4 byte5 byte6]
// top-bits bit6..0 contains the top bits for (up to) 6 following bytes
// So, 8 bytes, all with bit7==0 as required by midi-protocol, contain 7 net bytes
//
// Note that the input buffer is raw USB-MIDI data which is always 4-byte mutliples,
// the first byte of each block containing a cable-number and data-type ID which has
// to be discarded
static int decodeRawSysexIntoCodeBuffer(uint8_t const* const buff, uint32_t const len)
{
  if ((len < 4) || (buff[1] != 0xF0))  // too short or not a sysex ?
    return 0;
  uint16_t i           = 2;  // we will be beyond the F0 header already
  uint8_t  topBitsMask = 0;
  uint8_t  topBits;
  uint8_t  byte;
  while ((byte = buff[i]) != 0xF7)  // wait for sysex tail
  {
    if (topBitsMask == 0)
    {
      topBitsMask = 0x40;     // reset top bit mask to first bit (bit6)
      topBits     = buff[i];  // save top bits
    }
    else
    {
      if (topBits & topBitsMask)
        byte |= 0x80;  // set top bit when required
      topBitsMask >>= 1;
      *(code++) = byte;
    }
    i++;
    if (i >= len)  // buffer ends before sysex tail was found ?
      return 0;
    if ((i & 0b11) == 0)  // on USB raw frame header byte ?
      i++;                // then skip it
  }
  while ((i & 0b11) != 0)  // advance to next packet ...
    i++;                   // .. boundary it required
  return (i == len);       // when there is more data than one single sysex this is an error, too
}

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
  if (error || !decodeRawSysexIntoCodeBuffer(buff, len))
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
  (*execStart)();
  return 1;
}
