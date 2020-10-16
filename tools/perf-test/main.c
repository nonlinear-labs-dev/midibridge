#include <stdio.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

void error(char const *const s)
{
  puts(s);
  exit(3);
}

void send(snd_rawmidi_t *handle, const uint8_t *data, size_t const numBytes)
{
  if(handle)
  {
    ssize_t res;
    if(res = snd_rawmidi_write(handle, data, numBytes))
    {
      if((size_t) res != numBytes)
      {
        error("Could not write message into midi output device");
      }
    }
    snd_rawmidi_drain(handle);
  }
}

static void LogClock(char const *const s)
{
  int result;
  struct timespec tp;
  clockid_t clk_id;

  clk_id = CLOCK_MONOTONIC;

  result = clock_gettime(clk_id, &tp);
  printf("MIDI %s TimeStamp %lld:%ld\n", s, tp.tv_sec, tp.tv_nsec);
}

int main(int argc, char *argv[])
{
  snd_rawmidi_t *midi1;
  snd_rawmidi_t *midi2;

  if(snd_rawmidi_open(NULL, &midi1, argv[1], SND_RAWMIDI_NONBLOCK))
    return 3;
  if(snd_rawmidi_open(&midi2, NULL, argv[2], SND_RAWMIDI_NONBLOCK))
    return 3;

  uint8_t dataOut[] = { 0x80, 0x30, 0x30, 0x90, 0x30, 0x30 };
  uint8_t dataIn[sizeof dataOut];

  while(1)
  {
    usleep(30);
    send(midi1, dataOut, sizeof dataOut);
    // LogClock("write");
    snd_rawmidi_read(midi2, dataIn, sizeof dataIn);
  }
#if 0
  LogClock("read ");
  for(int i = 0; i < sizeof dataIn; i++)
    printf("%02X ", dataOut[i]);
  printf("\n");
#endif
  snd_rawmidi_close(midi1);
  snd_rawmidi_close(midi2);
  return 0;
}
