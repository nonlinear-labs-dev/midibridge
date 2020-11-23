#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>

typedef enum
{
  FALSE = 0,
  TRUE
} BOOL;

static BOOL           send;   // flag for program function: 1-->send , 0-->receive
static char const *   pName;  // port name
static snd_rawmidi_t *port;   // MIDI port
static BOOL           stop;
static int const      BUFFER_SIZE        = 1000;
static int const      encodedPayloadSize = 40;  // encoded payload transfer size, >= 8, <= BUFFER_SIZE !!
static BOOL           dump               = FALSE;

static void error(char const *const format, ...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  putc('\n', stderr);
}

static void usage(void)
{
  printf(
      "Usage: perf-test -s|-r port\n"
      "\n"
      "-s     : send test data\n"
      "-r     : receive test data\n"
      "port   : MIDI port to test (in hw:x,y,z notation, see ouput of 'amidi -l'\n");
}

static inline void getCmdLineParams(int const argc, char const *const argv[])
{
  if (argc != 3)
  {
    error("missing parameters\n");
    usage();
    exit(1);
  }

  if (strlen(argv[1]) == 2 && 0 == strcmp(argv[1], "-s"))
    send = TRUE;
  else if (strlen(argv[1]) == 2 && 0 == strcmp(argv[1], "-r"))
    send = FALSE;
  else
  {
    error("illegal parameter '%s'\n", argv[1]);
    usage();
    exit(1);
  }

  pName = argv[2];
}

static inline void openPort(void)
{
  int err;
  if (send)
    err = snd_rawmidi_open(NULL, &port, pName, 0);  // send will be blocking
  else
    err = snd_rawmidi_open(&port, NULL, pName, SND_RAWMIDI_NONBLOCK);
  if (err < 0)
  {
    error("cannot open port \"%s\": %s", pName, snd_strerror(err));
    exit(1);
  }
}

static inline void closePort(void)
{
  snd_rawmidi_close(port);
}

static void sigHandler(int dummy)
{
  stop = TRUE;
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

//
// -------- functions for sending --------
//

static inline int encodeSysex(void const *const pSrc, int len, void *const pDest)
{
  uint8_t *src         = (uint8_t *) pSrc;
  uint8_t *dst         = (uint8_t *) pDest;
  uint8_t  topBitsMask = 0;
  uint8_t *topBits;

  // write start of sysex
  *(dst++) = 0xF0;

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

  // write end of sysex
  *(dst++) = 0xF7;
  return (void *) dst - pDest;  // total bytes written to destination buffer;
}

static inline void doSend(void)
{
  int     messageLen;
  uint8_t dataBuf[BUFFER_SIZE];
  uint8_t sendBuf[BUFFER_SIZE * 2];  // need room for decoding sysex
  int     err, written;
#if 0
  snd_rawmidi_params_t *pParams;
  snd_rawmidi_params_alloca(&pParams);

  if ((err = snd_rawmidi_params_set_buffer_size(port, pParams, sizeof sendBuf)) < 0)
  {
    error("cannot set buffer size of %d : %s", sizeof sendBuf, snd_strerror(err));
    return;
  }
#endif
  do
  {
    memset(dataBuf, 0x7F, sizeof(dataBuf));
    *((uint64_t *) dataBuf) = getTimeUSec();
    messageLen              = encodeSysex(dataBuf, encodedPayloadSize, sendBuf);
    int total               = 0;
    while (total != messageLen)
    {
      written = snd_rawmidi_write(port, &(sendBuf[total]), messageLen - total);
      snd_rawmidi_drain(port);
      if (written < 0)
      {
        error("cannot send data: %s", snd_strerror(written));
        return;
      }
      // printf("chunk of %d bytes transferred\n", written);
      total += written;
    }
    if (written != total)
      ;  //printf("total of %d bytes transferred\n", total);

  } while (TRUE);
}

//
// -------- functions for receiving --------
//

static inline void examineContent(void const *const data, int const len)
{
  if (len != encodedPayloadSize)
  {
    error("receive: payload has wrong length %d, expected %d", len, encodedPayloadSize);
    exit(3);
  }

  uint64_t const *const pTime = data;
  uint64_t const        now   = getTimeUSec();
  uint64_t              time  = now - *pTime;

  static uint64_t min = ~(uint64_t) 0;
  static uint64_t max = 0;
  static uint32_t cnt = 0;
  static uint64_t sum = 0;

  if (time > max)
    max = time;
  if (time < min)
    min = time;
  sum += time;
  cnt++;

  if (cnt % 100 == 0)
  {
    printf("min:%10.3lfms max:%10.3lfms avg:%10.3lfms\n",
           ((double) min) / 1000.0, ((double) max) / 1000.0, ((double) sum) / 1000.0 / cnt);
    fflush(stdout);
  }
}

static inline void parseReceivedByte(uint8_t byte)
{
  static int step;

  static uint8_t buf[BUFFER_SIZE];
  static int     bufPos = 0;
  static uint8_t topBitsMask;
  static uint8_t topBits;

  switch (step)
  {
    case 0:  // wait for sysex begin
      if (byte == 0xF0)
      {
        bufPos      = 0;
        topBitsMask = 0;
        step        = 1;
        if (dump)
          printf("\n%2X ", byte);
      }
      break;
    case 1:              // sysex content
      if (byte == 0xF7)  // end of sysex ?
      {
        if (dump)
          printf("%02X\n", byte);
        examineContent(buf, bufPos);
        step = 0;
        break;
      }
      if (dump)
        printf("%02X ", byte);
      if (byte & 0x80)
      {
        error("unexpected byte >= 0x80 within sysex!");
        exit(3);
      }
      if (topBitsMask == 0)
      {
        topBitsMask = 0x40;  // reset top bit mask to first bit (bit6)
        topBits     = byte;  // save top bits
      }
      else
      {
        if (topBits & topBitsMask)
          byte |= 0x80;  // set top bit when required
        topBitsMask >>= 1;
        if (bufPos >= BUFFER_SIZE)
        {
          error("unexpected buffer overrun (no sysex terminator found)!");
          exit(3);
        }
        buf[bufPos++] = byte;
      }
      break;
  }
}

static inline void doReceive(void)
{
  int            npfds;
  struct pollfd *pfds;
  int            read;
  uint8_t        buf[BUFFER_SIZE];
  int            err;
  unsigned short revents;

  snd_rawmidi_status_t *pStatus;
  snd_rawmidi_status_alloca(&pStatus);

#if 0
  snd_rawmidi_params_t *pParams;

  snd_rawmidi_params_alloca(&pParams);

  if ((err = snd_rawmidi_params_set_buffer_size(port, pParams, sizeof buf)) < 0)
  {
    error("cannot set buffer size of %d : %s", sizeof buf, snd_strerror(err));
    return;
  }
#endif
  if ((err = snd_rawmidi_nonblock(port, 0)) < 0)
  {
    error("cannot set blocking mode: %s", snd_strerror(err));
    return;
  }

  npfds = snd_rawmidi_poll_descriptors_count(port);
  pfds  = alloca(npfds * sizeof(struct pollfd));
  snd_rawmidi_poll_descriptors(port, pfds, npfds);
  signal(SIGINT, sigHandler);

  do
  {
    if (snd_rawmidi_status_get_xruns(pStatus))
      error("rawmidi receive buffer overrun!");
    err = poll(pfds, npfds, 0);               // poll as fast as possible
    if (stop || (err < 0 && errno == EINTR))  // interrupted ?
      break;
    if (err < 0)
    {
      error("poll failed: %s", strerror(errno));
      break;
    }

    if ((err = snd_rawmidi_poll_descriptors_revents(port, pfds, npfds, &revents)) < 0)
    {
      error("cannot get poll events: %s", snd_strerror(errno));
      break;
    }
    if (revents & (POLLERR | POLLHUP))
      break;
    if (!(revents & POLLIN))
      continue;

    read = snd_rawmidi_read(port, buf, sizeof buf);
    if (read == -EAGAIN)
      continue;  // nothing read yet

    if (read < 0)
    {
      error("cannot read from port \"%s\": %s", pName, snd_strerror(read));
      break;
    }

    for (int i = 0; i < read; ++i)
      parseReceivedByte(buf[i]);
  } while (TRUE);
  printf("\n");
  fflush(stdout);
}

//
// ------------------------------------------
//
int main(int argc, char const *const argv[])
{
  getCmdLineParams(argc, argv);
  openPort();
  if (send)
    doSend();
  else
    doReceive();
  closePort();
  return 0;
}
