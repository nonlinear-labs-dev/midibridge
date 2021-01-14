#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>

#include "midi/nl_devctl_defs.h"

static void error(char const *const format, ...)
{
  va_list ap;
  fflush(stdout);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  putc('\n', stderr);
  fflush(stderr);
}

static void usage(void)
{
  fflush(stderr);
  puts("Usage: mk-sysex <in-file> <out-file>");
  puts(" Generate a sysex (.syx) binary file from a binary input file,");
  puts(" to be used to upload the binary to the NLL Midi Bridge.");
  puts(" Payload data is encoded with the NLL 8-to-7 bit scheme.");
}

#pragma pack(push, 1)
static struct
{
  uint8_t topBits;
  uint8_t lsbytes[7];
} encodeData;
#pragma pack(pop)

uint8_t topBitsMask;
int     fillCount;

static void flushAnyRemaingBuffer(FILE *outfile)
{
  if (fillCount != 0)
  {
    if (1 != fwrite(&encodeData, fillCount + 1, 1, outfile))
    {
      error("could not write to output file!");
      exit(3);
    }
  }
}

static void encodeAndWrite(uint8_t *buffer, size_t len, FILE *outfile)
{
  while (len--)
  {
    if (topBitsMask == 0)  // need to start insert a new top bits byte ?
    {
      topBitsMask        = 0x40;  // reset top bit mask to first bit (bit6)
      encodeData.topBits = 0;     // ...and clear top bits
      fillCount          = 0;
    }
    uint8_t byte = *(buffer++);
    if (byte & 0x80)
      encodeData.topBits |= topBitsMask;  // add in top bit for this byte
    topBitsMask >>= 1;

    byte &= 0x7F;
    encodeData.lsbytes[fillCount] = byte;
    fillCount++;
    if (fillCount == 7)
    {
      if (1 != fwrite(&encodeData, sizeof(encodeData), 1, outfile))
      {
        error("could not write to output file!");
        exit(3);
      }
      fillCount = 0;
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    error("Wrong number of arguments!");
    usage();
    return 3;
  }

  FILE *inFile;
  FILE *outFile;
  if (NULL == (inFile = fopen(argv[1], "rb")))
  {
    error("Could not open input file %s", argv[1]);
    return 3;
  }
  if (NULL == (outFile = fopen(argv[2], "wb")))
  {
    error("Could not open output file %s", argv[2]);
    return 3;
  }

#define BLOCKSIZE (4096)
  uint8_t inBuffer[BLOCKSIZE];  // read blocks of data from file

  puts("writing device control header...");
  if (1 != fwrite(NLMB_DevCtlSignature, sizeof(NLMB_DevCtlSignature), 1, outFile))
  {
    error("could not write to output file!");
    return 3;
  }

  puts("encoding payload...");
  size_t bytes;
  size_t total = 0;
  while (BLOCKSIZE == (bytes = fread(inBuffer, 1, BLOCKSIZE, inFile)))
  {
    encodeAndWrite(inBuffer, BLOCKSIZE, outFile);
    total += BLOCKSIZE;
  }
  encodeAndWrite(inBuffer, bytes, outFile);
  flushAnyRemaingBuffer(outFile);
  total += bytes;
  printf("encoded %lu bytes of payload data.\n", total);
  fclose(inFile);

  uint8_t byte = 0xF7;
  if (1 != fwrite(&byte, sizeof(byte), 1, outFile))
  {
    error("could not write to output file!");
    return 3;
  }
  fclose(outFile);
  puts("done");
  return 0;
}
