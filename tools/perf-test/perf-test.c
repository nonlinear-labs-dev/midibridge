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
#include <inttypes.h>

#define PAYLOAD_BUFFER_SIZE (100000ul)

#define RAW_TX_BUFFER_SIZE (PAYLOAD_BUFFER_SIZE * 2ul)  // need headroom for 8-to-7 bit encoding of the payload
#define RAW_RX_BUFFER_SIZE (PAYLOAD_BUFFER_SIZE * 2ul)  // should have headroom for 8-to-7 bit encoding of the payload
#define PARSER_BUFFER_SIZE (RAW_RX_BUFFER_SIZE)

#define SEND_WAIT_MS (1ul)  // milliseconds to wait when send buffer is going to overrun

#define DISPLAY_PERIOD (1000000u)
#define MAX_DEGEN      (0.97)
#define MIN_DEGEN      (0.97)

#define TTY_DEFAULT "\033[0m"
#define TTY_RED     "\033[0;31m"
#define TTY_GREEN   "\033[0;32m"

typedef enum
{
  FALSE = 0,
  TRUE
} BOOL;

static BOOL           send;   // flag for program function: 1-->send , 0-->receive
static BOOL           local;  // flag for using local relative time
static char const *   pName;  // port name
static snd_rawmidi_t *port;   // MIDI port
static BOOL           stop;
static int            blkSize   = -1;
static int            delayInUs = 1000;

static BOOL dump = FALSE;

static void error(char const *const format, ...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  putc('\n', stderr);
}

static void cursorUp(uint8_t lines)
{
  char buffer[12];
  sprintf(buffer, "\033[%dA", lines);
  printf("%s", buffer);
}

static void usage(void)
{
  printf(
      "Usage: perf-test -s|-r port [blksize [delay]]\n"
      "\n"
      "-s     : send test data\n"
      "-r     : receive test data\n"
      "-rl    : receive test data, use relative local time\n"
      "port   : MIDI port to test (in hw:x,y,z notation, see ouput of 'amidi -l'\n"
      "blksize: fixed block size of data chunk, for send only\n"
      "         (also forces a constant <delay> sleep time between messages)\n"
      "delay  : delay between messages in millisconds, default is 1\n");
}

static inline void getCmdLineParams(int const argc, char const *const argv[])
{
  if (argc != 3 && argc != 4 && argc != 5)
  {
    error("missing parameters\n");
    usage();
    exit(1);
  }

  if (strlen(argv[1]) == 2 && 0 == strcmp(argv[1], "-s"))
    send = TRUE;
  else if (strlen(argv[1]) == 2 && 0 == strcmp(argv[1], "-r"))
    send = FALSE;
  else if (strlen(argv[1]) == 3 && 0 == strcmp(argv[1], "-rl"))
  {
    send  = FALSE;
    local = TRUE;
  }
  else
  {
    error("illegal parameter '%s'\n", argv[1]);
    usage();
    exit(1);
  }

  pName = argv[2];

  if (argc >= 4)
  {
    if ((1 != sscanf(argv[3], "%i", &blkSize)) || (blkSize < 0) || (blkSize > 99000))
    {
      error("illegal block size (must be 1...99000\n");
      usage();
      exit(1);
    }
    if (argc > 4)
    {
      if ((1 != sscanf(argv[4], "%i", &delayInUs)) || (delayInUs < 0))
      {
        error("illegal delay\n");
        usage();
        exit(1);
      }
      delayInUs *= 1000;
      if (delayInUs < 1)
        delayInUs = 1;
    }
  }
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void            sigHandler(int dummy)
{
  stop = TRUE;
}
#pragma GCC diagnostic pop

static uint64_t getTimeUSec(void)
{
  struct timespec tp;

  if (clock_gettime(CLOCK_MONOTONIC, &tp))
    return 0;
  else
    return 1000000ul * (uint64_t)(tp.tv_sec) + (uint64_t)(tp.tv_nsec) / 1000ul;
}

//
// -------- functions for sending --------
//

uint64_t sndStartTime      = 0;
uint64_t sndTotalTime      = 1;
uint64_t sndTotalBytes     = 1;
uint64_t sndTime           = 0;
uint64_t sndLastPacketTime = 0;

static inline int encodeSysex(void const *const pSrc, int len, uint8_t *const pDest)
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
  return dst - pDest;  // total bytes written to destination buffer;
}

static inline void doSend(void)
{
  int     messageLen;
  uint8_t dataBuf[PAYLOAD_BUFFER_SIZE];
  uint8_t sendBuf[RAW_TX_BUFFER_SIZE];
  int     err, written;

  snd_rawmidi_status_t *pStatus;
  snd_rawmidi_status_alloca(&pStatus);

  snd_rawmidi_params_t *pParams;
  snd_rawmidi_params_alloca(&pParams);

  snd_rawmidi_params_current(port, pParams);
  snd_rawmidi_params_set_buffer_size(port, pParams, sizeof sendBuf);
  snd_rawmidi_params(port, pParams);
  snd_rawmidi_params_current(port, pParams);
  if (snd_rawmidi_params_get_buffer_size(pParams) != sizeof sendBuf)
  {
    error("cannot set send buffer size of %d", sizeof sendBuf);
    return;
  }

  if ((err = snd_rawmidi_nonblock(port, 0)) < 0)
  {
    error("cannot set blocking mode: %s", snd_strerror(err));
    return;
  }

  // messageLen = encodeSysex(dataBuf, PAYLOAD_SIZE, sendBuf);
  // printf("sysex size: %d\n", messageLen);
  uint8_t  runningCntr = 0;
  uint64_t messageNo   = 0;

  printf("Sending data to port: %s\n\n\n", pName);

  unsigned maxCntr = 0;
  unsigned minCntr = 0;

  do
  {
    memset(dataBuf, runningCntr++, sizeof(dataBuf));
    if (runningCntr >= 128)
      runningCntr = 0;

    ((uint64_t *) dataBuf)[0] = getTimeUSec();
    ((uint64_t *) dataBuf)[1] = messageNo;

    unsigned size;
    if (blkSize >= 0)
      size = blkSize;
    else
    {
      unsigned expo = ((unsigned) rand()) % 11;  // 0..10
      size          = 1 << expo;                 // 2^0...2^10(1024)
      size += ((unsigned) rand()) & (size - 1);  // 0..2047
    }
    if (size & 1)
      size++;
    size += 24;

    ((uint64_t *) dataBuf)[2] = size;

    uint16_t *p   = (uint16_t *) &(dataBuf[24]);
    uint16_t  val = messageNo & 0xFFFF;
    for (unsigned i = 0; i < (size - 24) / 2; i++)
      *p++ = val++;

    cursorUp(1);
    printf("n:%" PRIu64 ", s:%5u\n", messageNo++, size);
    fflush(stdout);

    messageLen = encodeSysex(dataBuf, size, sendBuf);
    int total  = 0;

    uint64_t messageTime = getTimeUSec();
    while (total != messageLen)
    {
      int toTransfer = messageLen - total;

      if ((err = snd_rawmidi_status(port, pStatus)))
      {
        error("cannot get status: %s", snd_strerror(errno));
        return;
      }
      int avail = snd_rawmidi_status_get_avail(pStatus);
      if (0)  //toTransfer > avail)
      {
        error("rawmidi send larger than buffer by: %d", toTransfer - avail);
        usleep(1000ul * SEND_WAIT_MS);
        continue;
      }

      uint64_t time = getTimeUSec();
      written       = snd_rawmidi_write(port, &(sendBuf[total]), toTransfer);

      if ((err = snd_rawmidi_drain(port)) < 0)
      {
        error("cannot drain buffer: %s", snd_strerror(err));
        return;
      }
      if (written < 0)
      {
        error("cannot send data: %s", snd_strerror(written));
        return;
      }
      // printf("chunk of %d bytes transferred\n", written);

      total += written;
      time = getTimeUSec() - time;
      time *= 1 + (((unsigned) rand()) & 0xF);  // * 1..16
      //time /= 3;
      if (time > 1000ul * 1000ul)
        time = 1000ul * 1000ul;

      uint64_t sleepTime = getTimeUSec();
      if (blkSize < 0)  // dynamically pause with non-constant block sizes
        usleep(time);
      else
        usleep(delayInUs);
      sleepTime = getTimeUSec() - sleepTime;
      messageTime += sleepTime;
    }
    if (written != total)
    {
      ;  //printf("total of %d bytes transferred\n", total);
    }

    uint64_t const now = getTimeUSec();
    messageTime        = now - messageTime;
    if (sndStartTime == 0)
      sndStartTime = now;
    else
      sndTotalBytes += messageLen;
    sndTotalTime = (now - sndStartTime);

    static uint64_t min         = ~(uint64_t) 0;
    static uint64_t max         = 0;
    static uint32_t cnt         = 0;
    static uint64_t sum         = 0;
    static uint64_t period      = 0;
    static uint64_t displayTime = 0;

    if (messageTime > max)
      maxCntr = 6, max = messageTime;
    if (messageTime < min)
      minCntr = 6, min = messageTime;
    sum += messageTime;

    if (sndLastPacketTime)
      period += (now - sndLastPacketTime);
    cnt++;

    if (cnt == 10000)
    {
      cnt /= 2;
      sum /= 2;
      period /= 2;
    }
    sndLastPacketTime = now;

    if (now > displayTime)
    {
      displayTime = now + DISPLAY_PERIOD;
      cursorUp(2);
      printf("%s%6.2lfms(min) %s%6.2lfms(max)" TTY_DEFAULT " %6.2lfms(avg) %6.2lfms(period)  %6.2lfkB/s\n\n",
             minCntr == 0 ? TTY_DEFAULT : (minCntr >= 6 ? TTY_RED : TTY_GREEN), ((double) min) / 1000.0,
             maxCntr == 0 ? TTY_DEFAULT : (maxCntr >= 6 ? TTY_RED : TTY_GREEN), ((double) max) / 1000.0,
             ((double) sum) / 1000.0 / cnt,
             ((double) period) / 1000.0 / cnt,
             1000.0 * (double) sndTotalBytes / (double) sndTotalTime);
      fflush(stdout);

      if (maxCntr)
        maxCntr--;
      if (minCntr)
        minCntr--;

      if (max > 10 * sum / cnt)
        max = max * 0.9;
      else
        max = max * MAX_DEGEN;

      if (min * 10 > sum / cnt)
        min = min / 0.9;
      else
        min = min / MIN_DEGEN;
    }

  } while (TRUE);
}

//
// -------- functions for receiving --------
//

uint64_t packetCntr;
uint64_t rcvStartTime      = 0;
uint64_t rcvTotalTime      = 1;
uint64_t rcvTotalBytes     = 0;
uint64_t rcvTime           = 0;
uint64_t rcvLastPacketTime = 0;

static inline BOOL examineContent(void const *const data, unsigned const len)
{
  static unsigned maxCntr = 0;
  static unsigned minCntr = 0;

  static uint64_t displayTime = 0;
  if (len < 24)
  {
    error("receive: payload has wrong minimum length %d, expected %d", len, 16);
    return FALSE;
  }

  uint64_t const packetTime   = ((uint64_t *) data)[0];
  uint64_t const packetNumber = ((uint64_t *) data)[1];
  uint64_t const packetSize   = ((uint64_t *) data)[2];

  if (packetSize != len)
  {
    error("receive: packet has wrong length %" PRIu64 ", expected %" PRIu64, len, packetSize);
    return FALSE;
  }

  if (packetCntr++ != 0)
  {
    if (packetNumber != packetCntr)
    {
      error("receive: packet has wrong number %" PRIu64 ", expected %" PRIu64, packetNumber, packetCntr);
      return FALSE;
    }
  }
  packetCntr = packetNumber;

  cursorUp(1);
  printf("n:%" PRIu64 ", s:%5" PRIu64 "\n", packetNumber, packetSize);
  fflush(stdout);

  uint16_t *p   = (uint16_t *) &(((uint64_t *) data)[3]);
  uint16_t  val = packetNumber & 0xFFFF;
  for (unsigned i = 0; i < (packetSize - 24) / 2; i++)
  {
    if (*p++ != val++)
    {
      error("receive: packet is corrupted");
      return FALSE;
    }
  }

  uint64_t const now = getTimeUSec();
  if (rcvLastPacketTime == 0)
    rcvLastPacketTime = now;

  uint64_t time;

  if (local)
    time = now - rcvLastPacketTime;
  else
    time = now - packetTime;

  static uint64_t min = ~(uint64_t) 0;
  static uint64_t max = 0;
  static uint32_t cnt = 0;
  static uint64_t sum = 0;

  if (time > max)
    maxCntr = 6, max = time;
  if (time < min)
    minCntr = 6, min = time;
  sum += time;
  cnt++;

  if (cnt == 10000)
  {
    cnt /= 2;
    sum /= 2;
  }
  rcvLastPacketTime = now;
  if (now > displayTime)
  {
    displayTime = now + DISPLAY_PERIOD;
    cursorUp(2);
    printf("%s%6.2lfms(min) %s%6.2lfms(max) " TTY_DEFAULT "%6.2lfms(avg) %s  %6.2lfkB/s\n\n",
           minCntr == 0 ? TTY_DEFAULT : (minCntr >= 6 ? TTY_RED : TTY_GREEN), ((double) min) / 1000.0,
           maxCntr == 0 ? TTY_DEFAULT : (maxCntr >= 6 ? TTY_RED : TTY_GREEN), ((double) max) / 1000.0,
           ((double) sum) / 1000.0 / cnt,
           local ? "<-period" : "",
           1000.0 * (double) rcvTotalBytes / (double) rcvTotalTime);
    fflush(stdout);

    if (maxCntr)
      maxCntr--;
    if (minCntr)
      minCntr--;

    if (max > 10 * sum / cnt)
      max = max * 0.9;
    else
      max = max * MAX_DEGEN;

    if (min * 10 > sum / cnt)
      min = min / 0.9;
    else
      min = min / MIN_DEGEN;
  }
  return TRUE;
}

static inline BOOL parseReceivedByte(uint8_t byte)
{
  static int step;

  static uint8_t  buf[PARSER_BUFFER_SIZE];
  static unsigned bufPos = 0;
  static uint8_t  topBitsMask;
  static uint8_t  topBits;

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
      return TRUE;
    case 1:              // sysex content
      if (byte == 0xF7)  // end of sysex ?
      {
        if (dump)
          printf("%02X\n", byte);
        step = 0;
        return examineContent(buf, bufPos);
      }
      if (dump)
        printf("%02X ", byte);
      if (byte & 0x80)
      {
        error("unexpected byte >= 0x80 within sysex!");
        return FALSE;
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
        if (bufPos >= PARSER_BUFFER_SIZE)
        {
          error("unexpected buffer overrun (no sysex terminator found)!");
          return FALSE;
        }
        buf[bufPos++] = byte;
      }
      break;
  }
  return TRUE;
}

static inline void doReceive(void)
{
  int            npfds;
  struct pollfd *pfds;
  int            read;
  uint8_t        buf[RAW_RX_BUFFER_SIZE];
  int            err;
  unsigned short revents;

  snd_rawmidi_status_t *pStatus;
  snd_rawmidi_status_alloca(&pStatus);

  snd_rawmidi_params_t *pParams;
  snd_rawmidi_params_alloca(&pParams);

  snd_rawmidi_params_current(port, pParams);
  snd_rawmidi_params_set_buffer_size(port, pParams, sizeof buf);
  snd_rawmidi_params(port, pParams);
  snd_rawmidi_params_current(port, pParams);
  if (snd_rawmidi_params_get_buffer_size(pParams) != sizeof buf)
  {
    error("cannot set send buffer size of %d", sizeof buf);
    return;
  }

  if ((err = snd_rawmidi_nonblock(port, 1)) < 0)
  {
    error("cannot set non-blocking mode: %s", snd_strerror(err));
    return;
  }

  npfds = snd_rawmidi_poll_descriptors_count(port);
  pfds  = alloca(npfds * sizeof(struct pollfd));
  snd_rawmidi_poll_descriptors(port, pfds, npfds);
  signal(SIGINT, sigHandler);

  printf("Receiving data from port: %s\n\n\n", pName);

  do
  {
    err = poll(pfds, npfds, 10);              // timeout if no data even after 10msec
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

    if ((err = snd_rawmidi_status(port, pStatus)))
    {
      error("cannot get status: %s", snd_strerror(errno));
      break;
    }

    if (rcvStartTime == 0)
      rcvStartTime = getTimeUSec();
    else
      rcvTotalBytes += read;
    rcvTotalTime = (getTimeUSec() - rcvStartTime);
    for (int i = 0; i < read; ++i)
    {
      if (!parseReceivedByte(buf[i]))  // parser error
      {
        for (int j = 0; j < read; j++)
        {
          if (j == i)
            printf(">>%02X<< ", buf[j]);  // mark the offending byte
          else
            printf("%02X ", buf[j]);
        }
        stop = TRUE;
      }
    }
    if ((err = snd_rawmidi_status_get_xruns(pStatus)))
    {
      error("rawmidi receive buffer overrun: %d", err);
      break;
    }
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
