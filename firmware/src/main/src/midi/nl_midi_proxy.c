#include "nl_midi_proxy.h"
#include "usb/nl_usba_core.h"
#include "usb/nl_usbb_core.h"
#include "usb/nl_usba_midi.h"
#include "usb/nl_usbb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "midi/nl_midi_isp.h"
#include "midi/nl_sysex.h"
#include "sys/nl_version.h"

#define BUFFER_SIZE            (1024)  // same size as low-level MIDI transfer buffers !!
#define NUMBER_OF_BUFFERS      (8)     // number of buffers, must be 2^N !
#define NUMBER_OF_BUFFERS_MASK (NUMBER_OF_BUFFERS - 1)

#define A (0)
#define B (1)

// ringbuffer of buffers
typedef struct
{
  uint8_t  buff[NUMBER_OF_BUFFERS][BUFFER_SIZE];
  uint16_t bufHead;       // 0..(NUMBER_OF_BUFFERS-1)
  uint16_t bufTail;       // 0..(NUMBER_OF_BUFFERS-1)
  uint16_t bufHeadIndex;  // used bytes in the current head (front) buffer
} ringbuffer_t;

static ringbuffer_t rb[2];

typedef struct
{
  uint16_t buspower;
  uint16_t initialized;
  uint16_t buffering;
  uint16_t dataLoss;
  uint16_t lowLevelTraffic;
  uint16_t midiTraffic;
} proxyStatus_t;

static proxyStatus_t status[2];

// timeouts in 50ms multiples
#define TIMEOUT_MIDI_TRAFFIC      (4)    // 200ms
#define TIMEOUT_LOW_LEVEL_TRAFFIC (2)    // 100ms
#define TIMEOUT_BUFFERING         (100)  // 5s
#define TIMEOUT_DATALOSS          (400)  // 20s

// -----------------------------
static void ringbufferReset(void)
{
  for (int i = 0; i <= 1; i++)
  {
    rb[i].bufHead      = 0;
    rb[i].bufTail      = 0;
    rb[i].bufHeadIndex = 0;
  }
}

static inline int fillBuffer(uint8_t const which, uint8_t *buff, uint32_t len);

static void sendInfo(uint8_t const which)
{
  uint8_t  buffer[sizeof VERSION_STRING_STRIPPED];
  uint16_t len = MIDI_encodeRawSysex((uint8_t *) VERSION_STRING_STRIPPED, sizeof VERSION_STRING_STRIPPED, buffer);
  fillBuffer(1 - which, buffer, len);
}

static inline int fillBuffer(uint8_t const which, uint8_t *buff, uint32_t len)
{

  if (!status[which].initialized)
  {
    status[which].dataLoss = TIMEOUT_DATALOSS;  // indicate that we had to discard data
    return 0;
  }

  ringbuffer_t *const p                 = &rb[which];
  uint16_t const      savedBufHead      = p->bufHead;
  uint16_t const      savedBufHeadIndex = p->bufHeadIndex;

  while (len)
  {
#if LED_TEST != 0
    if (*buff < 0x80)
      LED_SetState(which, (*buff & 0xF0) >> 4, (*buff) & 0x02, (*buff) & 0x01);
#endif
    if (p->bufHeadIndex >= BUFFER_SIZE)
    {  // current head is full, switch to next one
      uint16_t tmpBufHead = (p->bufHead + 1) & NUMBER_OF_BUFFERS_MASK;
      if (tmpBufHead == p->bufTail)  // no space left ?
      {                              // discard data and signal failure
        p->bufHead             = savedBufHead;
        p->bufHeadIndex        = savedBufHeadIndex;
        status[which].dataLoss = TIMEOUT_DATALOSS;  // indicate that we had to discard data
        return 0;
      }
      status[which].buffering = TIMEOUT_BUFFERING;  // indicate that we had to buffer data
      p->bufHead              = tmpBufHead;
      p->bufHeadIndex         = 0;
    }
    p->buff[p->bufHead][p->bufHeadIndex++] = *(buff++);
    len--;
  }
  return 1;
}

// ------------------------------------------------------------

static uint8_t inCommandMode;
static uint8_t commandModeChannel;

// return != 0 when in command mode and message shall be discarded by caller
static inline int processCommand(uint8_t const which, uint8_t *const buff, uint32_t const len)
{
  static uint8_t first = 1;

  if (!inCommandMode)
  {
    if (first)  // only check for command ID on first received message ever
    {
      if (ISP_isOurCommand(buff, len))
      {  // OK, a NLL MIDI-Bridge command was found
        inCommandMode      = 1;
        commandModeChannel = which;
      }
    }
  }
  first = 0;

  if (inCommandMode && (which == commandModeChannel))
  {                                                           // collect commands only from one and the same port
    inCommandMode = ISP_collectAndExecuteCommand(buff, len);  // may terminate command mode
    return 1;                                                 // but the command itself must be discarded
  }
  return 0;  // regular transfer
}
static inline void ReceiveACallback(uint8_t *buff, uint32_t len)
{
  if (processCommand(A, buff, len) == 0)
    fillBuffer(B, buff, len);
}

static inline void ReceiveBCallback(uint8_t *buff, uint32_t len)
{
  if (processCommand(B, buff, len) == 0)
    fillBuffer(A, buff, len);
}

void MIDI_PROXY_Init(void)
{
  USBA_MIDI_Config(ReceiveACallback);
  USBB_MIDI_Config(ReceiveBCallback);
}

// ------------------------------------------------------------------

#if USBA_PORT_FOR_MIDI == 0
#define pinUSBA_VBUS pinUSB0_VBUS
#else
#define pinUSBA_VBUS pinUSB1_VBUS
#endif

#if USBB_PORT_FOR_MIDI == 0
#define pinUSBB_VBUS pinUSB0_VBUS
#else
#define pinUSBB_VBUS pinUSB1_VBUS
#endif

static uint8_t USBA_NoBuspower = 3;
static uint8_t USBB_NoBuspower = 3;

static inline uint32_t bytesToSend(uint8_t const which)
{
  return (which == A) ? USBA_MIDI_BytesToSend() : USBB_MIDI_BytesToSend();
}

static inline uint32_t MIDI_Send(uint8_t const which, uint8_t const *const buff, uint32_t const cnt)
{
  return (which == A) ? USBA_MIDI_Send(buff, cnt, 0) : USBB_MIDI_Send(buff, cnt, 0);
}

static inline uint32_t MIDI_isConfigured(uint8_t const which)
{
  return (which == A) ? USBA_MIDI_IsConfigured() : USBB_MIDI_IsConfigured();
}

static inline void checkSends(uint8_t const which)
{
  ringbuffer_t *const p = &rb[which];
  if ((!status[which].initialized) || (p->bufHead == p->bufTail && p->bufHeadIndex == 0))
    return;  // nothing to send

  if (bytesToSend(which))
  {
    status[which].buffering = TIMEOUT_BUFFERING;  // indicate that we had to buffer data
    return;                                       // last transfer still in progress
  }

  if (p->bufHead != p->bufTail)
  {  // send stashed buffers first
    if (!MIDI_Send(which, p->buff[p->bufTail], BUFFER_SIZE))
      status[which].dataLoss = TIMEOUT_DATALOSS;  // send failed
    else
      status[which].midiTraffic = TIMEOUT_MIDI_TRAFFIC;  // success
    p->bufTail = (p->bufTail + 1) & NUMBER_OF_BUFFERS_MASK;
    return;
  }
  if (p->bufHeadIndex != 0)
  {  // send current buffer
    if (!MIDI_Send(which, p->buff[p->bufHead], p->bufHeadIndex))
      status[which].dataLoss = TIMEOUT_DATALOSS;  // send failed
    else
      status[which].midiTraffic = TIMEOUT_MIDI_TRAFFIC;  // success
    p->bufHead      = (p->bufHead + 1) & NUMBER_OF_BUFFERS_MASK;
    p->bufHeadIndex = 0;
    p->bufTail      = p->bufHead;
  }
}

static void clearStatus(uint8_t const which)
{
  status[which].buspower        = 0;
  status[which].initialized     = 0;
  status[which].buffering       = 0;
  status[which].dataLoss        = 0;
  status[which].lowLevelTraffic = 0;
  status[which].midiTraffic     = 0;
}

static void processStatus(void)
{
  for (int port = 0; port < 2; port++)
  {
    uint8_t baseColor, bright, flickering;
    LED_GetState(port, &baseColor, &bright, &flickering);

    if (status[port].buspower == 0)
      baseColor = COLOR_OFF, bright = 0, flickering = 0;
    else
    {
      if (status[port].initialized == 0)
        baseColor = COLOR_BLUE, bright = 1, flickering = 1;
      else if (status[port].dataLoss)
        baseColor = COLOR_RED;
      else if (status[port].buffering)
        baseColor = COLOR_ORANGE;
      else
        baseColor = COLOR_GREEN;

      if (status[port].midiTraffic)
        bright = 1, flickering = 1;
      else if (status[port].lowLevelTraffic)
        bright = 0, flickering = 1;
      else
        bright = 0, flickering = 0;
    }
#if LED_TEST == 0
    LED_SetState(port, baseColor, bright, flickering);
#endif

    status[port].buspower    = 0;
    status[port].initialized = 0;
    if (status[port].dataLoss)
      status[port].dataLoss--;
    if (status[port].buffering)
      status[port].buffering--;
    if (status[port].midiTraffic)
      status[port].midiTraffic--;
    if (status[port].lowLevelTraffic)
      status[port].lowLevelTraffic--;
  }
}
void MIDI_PROXY_ProcessFast(void)
{
  if (!pinUSBA_VBUS)  // VBUS is off now
  {
    if (USBA_NoBuspower == 0)  // was on before ?
    {                          // then turn USB off
      USBA_MIDI_DeInit();
      USBB_MIDI_DeInit();
      ringbufferReset();
      clearStatus(A);
    }
    USBA_NoBuspower = 3;
  }
  if (!pinUSBB_VBUS)  // VBUS is off now
  {
    if (USBB_NoBuspower == 0)  // was on before ?
    {                          // then turn USB off
      USBB_MIDI_DeInit();
      USBA_MIDI_DeInit();
      ringbufferReset();
      clearStatus(B);
    }
    USBB_NoBuspower = 3;
  }
  status[A].buspower    = !USBA_NoBuspower;
  status[A].initialized = status[A].buspower && MIDI_isConfigured(A) && USBA_SetupComplete();
  status[B].buspower    = !USBB_NoBuspower;
  status[B].initialized = status[B].buspower && MIDI_isConfigured(B) && USBB_SetupComplete();

  checkSends(A);
  checkSends(B);

  if (USBA_GetActivity())
    status[A].lowLevelTraffic = TIMEOUT_LOW_LEVEL_TRAFFIC;
  if (USBB_GetActivity())
    status[B].lowLevelTraffic = TIMEOUT_LOW_LEVEL_TRAFFIC;
}

void MIDI_PROXY_Process(void)
{
  if (inCommandMode)
    return;

  uint8_t armA = 0;
  uint8_t armB = 0;

  if (pinUSBA_VBUS)  // VBUS is on now
  {
    if (USBA_NoBuspower)  // still running plug-in time-out ?
    {
      if (!--USBA_NoBuspower)  // time-out elapsed ?
        armA = 1;              // arm turn-on trigger
    }
  }

  if (pinUSBB_VBUS)  // VBUS is on now
  {
    if (USBB_NoBuspower)  // still running plug-in time-out ?
    {
      if (!--USBB_NoBuspower)  // time-out elapsed ?
        armB = 1;              // arm turn-on trigger
    }
  }

  armA = armA && !USBB_NoBuspower;
  armB = armB && !USBA_NoBuspower;

  if (armA || armB)
  {
    USBA_MIDI_Init();
    USBB_MIDI_Init();
    clearStatus(A);
    clearStatus(B);
  }

  processStatus();
}
