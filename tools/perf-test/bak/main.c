#include <stdio.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

void error(char const *const s)
{
  puts(s);
  exit(3);
}

void send(snd_rawmidi_t *handle, const uint8_t *data, size_t const numBytes)
{
  if (handle)
  {
    ssize_t res;
    if (res = snd_rawmidi_write(handle, data, numBytes))
    {
      if ((size_t) res != numBytes)
      {
        error("Could not write message into midi output device");
      }
    }
    snd_rawmidi_drain(handle);
  }
}

static uint64_t getTimeUSec(void)
{
  int             result;
  struct timespec tp;

  if (clock_gettime(CLOCK_MONOTONIC, &tp))
    return 0;
  else
    return 1000000ul * (uint64_t)(tp.tv_sec) + (uint64_t)(tp.tv_nsec) / 1000ul;
}

static uint8_t dataOut[300];
static uint8_t dataIn[sizeof dataOut];

int main(int argc, char *argv[])
{
  snd_rawmidi_t *midiOut;
  snd_rawmidi_t *midiIn;

  if (snd_rawmidi_open(NULL, &midiOut, argv[1], SND_RAWMIDI_NONBLOCK))
    return 3;
  if (snd_rawmidi_open(&midiIn, NULL, argv[2], 0))  // blocking
    return 3;

  dataOut[0]                    = 0xF0;
  dataOut[(sizeof dataOut) - 1] = 0xF7;

  uint64_t time;
  uint64_t min = ~(uint64_t) 0;
  uint64_t max = 0;

  uint32_t cnt = 0;
  uint64_t sum = 0;

  while (1)
  {
    time = getTimeUSec();
    send(midiOut, dataOut, sizeof dataOut);
    snd_rawmidi_read(midiIn, dataIn, sizeof dataIn);
    time = getTimeUSec() - time;

    if (time > max)
      max = time;
    if (time < min)
      min = time;
    sum += time;

    usleep(1000ul);
    if (cnt++ % 1000 == 0)
    {
      printf("min:%10.3lfms max:%10.3lfms avg:%10.3lfms\n", ((double) min) / 1000.0, ((double) max) / 1000.0, ((double) sum) / 1000.0 / cnt);
    }
  }

  snd_rawmidi_close(midiOut);
  snd_rawmidi_close(midiIn);
  return 0;
}
