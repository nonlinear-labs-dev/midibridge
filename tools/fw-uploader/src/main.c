#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include "midi/nl_sysex.h"
#include "midi/nl_ispmarkers.h"

static void usage(char *message, int const quit)
{
  if (message)
    puts(message);
  puts("Usage: nlmb-fwupload <midi-device> <firmware-image>");
  puts("Upload and program a new firmware into the NLL MIDI Bridge via MIDI SysEx messages.");
  puts("  <midi-device> typically is hw:1,0,0 when there is only one midi device attached");
  puts("    (Use amidi -l to check connected devices select the NLL MIDI Bridge)");
  puts("  <firmware-image> is the path to a firmware image. Not sanity checks are performed, so only");
  puts("    use a file which you a sure that it contains a valid firmware for the NLL MIDI Bridge");
  puts("  Note that the firmware upload will work only if it is the first MIDI transaction for the device");
  puts("  after it was attached, otherwise the SysEx data will just be transmitted to the other side of the Bridge.");
  if (quit)
    exit(quit);
}

static size_t midiReceiveSysex(snd_rawmidi_t *const handle, uint8_t *const data, size_t maxBytes)
{
  if (data == NULL)
    usage("midiReceiveSysex : NULL pointer!", 3);
  if (handle == NULL)
    usage("midiReceiveSysex : NULL handle!", 3);

  uint8_t byte;
  ssize_t status;
  size_t  total = 0;

  // wait for sysex header
  do
  {
    status = snd_rawmidi_read(handle, &byte, 1);
    if (status <= 0)
      usage("midiReceiveSysex: read from MIDI device failed!", 3);
  } while (byte != 0xF0);
  data[total++] = byte;

  // copy data wait for sysex tail
  while (1)
  {
    status = snd_rawmidi_read(handle, &byte, 1);
    if (status <= 0)
      usage("midiReceiveSysex: read from MIDI device failed!", 3);
    if (byte == 0xF7)
      break;
    data[total++] = byte;
  };
  data[total++] = byte;
  return total;
}

static void midiSend(snd_rawmidi_t *const handle, uint8_t const *const data, size_t const numBytes)
{
  if (numBytes == 0)
    return;
  if (handle == NULL)
    usage("midiSend : NULL handle!", 3);
  ssize_t res;
  if (res = snd_rawmidi_write(handle, data, numBytes))
  {
    if ((size_t) res != numBytes)
      usage("midiSend: could not write message into MIDI output device!", 3);
  }
}

static size_t receiveAndDecode(snd_rawmidi_t *const handle, uint8_t *const data, size_t const maxBytes)
{
  uint8_t  sysexBuffer[8 * maxBytes / 6];
  uint16_t sysexSize = midiReceiveSysex(handle, sysexBuffer, 8 * maxBytes / 6);
  uint16_t dataSize  = MIDI_decodeSysex(sysexBuffer, sysexSize, data);
  return dataSize;
}

static int encodeAndSend(snd_rawmidi_t *const handle, uint8_t *const data, size_t const numBytes, uint8_t *const sysexBuffer)
{
  if (numBytes == 0)
    return 0;
  uint16_t sysexBytes;
  midiSend(handle, sysexBuffer, sysexBytes = MIDI_encodeSysex(data, numBytes, sysexBuffer));
  return 1;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
    usage("Wrong number of arguments!", 3);

  snd_rawmidi_t *midiOut;
  snd_rawmidi_t *midiIn;
  if (snd_rawmidi_open(&midiIn, &midiOut, argv[1], SND_RAWMIDI_SYNC))
    usage("Could not open MIDI device!", 3);

  FILE *imageFile;
  if (NULL == (imageFile = fopen(argv[2], "rb")))
    usage("Could not open image file", 3);

#define BLOCKSIZE (750)           // will assure the encoded sysex fits into the 1k sysex buffer
  uint8_t fileBuffer[BLOCKSIZE];  // read blocks of data from file
  uint8_t sysexBuffer[1000];      // write blocks of encoded sysex to midi device

  //
  // write ISP_INFO message to device
  midiSend(midiOut, ISP_INFO, ISP_getMarkerSize(ISP_INFO));
  // read response
  puts("reading firmware version string... (press CTRL-C if response does not show immediately)");
  char     versionString[200];
  uint16_t versionStringSize = receiveAndDecode(midiIn, versionString, sizeof versionString);
  if (versionStringSize > 1)
    printf(" current firmware is '%s'\n", versionString);

  printf("program new firmware? (y/n): ");
  char c = getchar();
  if (c != 'y' && c != 'Y')
  {
    puts("programming aborted!");
    exit(0);
  }

  //
  // write ISP_START message to device
  midiSend(midiOut, ISP_START, ISP_getMarkerSize(ISP_START));

  //
  // write ISP_END message to device
  puts("uploading new firmware...");
  size_t bytes;
  size_t total = 0;
  while (BLOCKSIZE == (bytes = fread(fileBuffer, 1, BLOCKSIZE, imageFile)))
  {
    if (encodeAndSend(midiOut, fileBuffer, BLOCKSIZE, sysexBuffer))
      total += BLOCKSIZE;
  }
  if (encodeAndSend(midiOut, fileBuffer, bytes, sysexBuffer))
    total += bytes;

  //
  // write ISP_END message to device
  midiSend(midiOut, ISP_END, ISP_getMarkerSize(ISP_END));
  printf(" %lu total bytes transferred to MIDI device.\n", total);

  // write ISP_EXECUTE message to device
  puts("flashing new firmware...");
  midiSend(midiOut, ISP_EXECUTE, ISP_getMarkerSize(ISP_EXECUTE));

  puts("done");
  fclose(imageFile);
  snd_rawmidi_close(midiOut);
  return 0;
}
