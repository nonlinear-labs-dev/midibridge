#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"

#define TX_BUFFERSIZE          (4096)  // buffers for outgoing messages
#define NUMBER_OF_BUFFERS      (4)     // number of buffers per port, must be 2^N !
#define NUMBER_OF_BUFFERS_MASK (NUMBER_OF_BUFFERS - 1)

#define A (0)
#define B (1)

// ringbuffer of buffers
__attribute__((section(".noinit.$RamLoc32"))) static uint8_t txBuffer[2][NUMBER_OF_BUFFERS][TX_BUFFERSIZE];

typedef struct
{
  uint16_t bufHead;       // 0..(NUMBER_OF_BUFFERS-1)
  uint16_t bufTail;       // 0..(NUMBER_OF_BUFFERS-1)
  uint16_t bufHeadIndex;  // used bytes in the current head (front) buffer
  uint8_t  step;
} ringbuffer_t;

static ringbuffer_t rb[2];

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

static inline int fillBuffer(uint8_t const port, uint8_t *buff, uint32_t len)
{

  ringbuffer_t *const p                 = &rb[port];
  uint16_t const      savedBufHead      = p->bufHead;
  uint16_t const      savedBufHeadIndex = p->bufHeadIndex;

  while (len)
  {
#if LED_TEST != 0
    if (*buff < 0x80)
      LED_SetState(port, (*buff & 0xF0) >> 4, (*buff) & 0x02, (*buff) & 0x01);
#endif
    if (p->bufHeadIndex >= TX_BUFFERSIZE)
    {  // current head is full, switch to next one
      uint16_t tmpBufHead = (p->bufHead + 1) & NUMBER_OF_BUFFERS_MASK;
      if (tmpBufHead == p->bufTail)  // no space left ?
      {                              // discard data and signal failure
        p->bufHead      = savedBufHead;
        p->bufHeadIndex = savedBufHeadIndex;
        // status[port].dataLoss = TIMEOUT_DATALOSS;  // indicate that we had to discard data
        LED_DBG3 = 1;
        return 0;
      }
      // status[port].buffering = TIMEOUT_BUFFERING;  // indicate that we had to buffer data
      p->bufHead      = tmpBufHead;
      p->bufHeadIndex = 0;
    }

    uint8_t const c                               = *(buff++);  // copy the byte
    txBuffer[port][p->bufHead][p->bufHeadIndex++] = c;

    static uint32_t open, count, firstCount;
    if (c == 0xF0)
    {
      open++;
      count = 0;
    }
    if (c == 0xF7)
    {
      open--;
      if (firstCount == 0)
        count = firstCount;
      else if (count != firstCount)
        LED_DBG3 = 1;
    }
    count++;
    if (open != 0 && open != 1)
      LED_DBG3 = 1;

    len--;
  }
  //  p->bufHead      = savedBufHead; // ???
  //  p->bufHeadIndex = savedBufHeadIndex; // ???
  return 1;
}

// ------------------------------------------------------------------

static inline void checkSends(uint8_t const port)
{
  ringbuffer_t *const p = &rb[port];

  if (!USB_MIDI_IsConfigured(port))
  {
    p->bufHead      = 0;
    p->bufHeadIndex = 0;
    p->bufTail      = 0;
    p->step         = 0;
    return;
  }

  switch (p->step)
  {
    case 0:  // wait for jobs
      if (p->bufHead != p->bufTail)
      {  // send stashed buffers first
        if (port == 0)
          LED_DBG1 = 1;
        else
          LED_DBG2 = 1;
        if (USB_MIDI_Send(port, txBuffer[port][p->bufTail], TX_BUFFERSIZE))
          p->step = 1;  // send accepted-->wait, else retry
      }
      else if (p->bufHeadIndex != 0)
      {  // send current buffer
        if (port == 0)
          LED_DBG1 = 1;
        else
          LED_DBG2 = 1;
        if (USB_MIDI_Send(port, txBuffer[port][p->bufTail], p->bufHeadIndex))
        {  // send accepted-->wait, else retry
          p->step = 1;
          // advance head (new current write buffer)
          p->bufHead      = (p->bufHead + 1) & NUMBER_OF_BUFFERS_MASK;
          p->bufHeadIndex = 0;
        }
      }
      break;

    case 1:  // wait for send to complete
      if (USB_MIDI_BytesToSend(port) > 0)
        break;
      if (port == 0)
        LED_DBG1 = 0;
      else
        LED_DBG2 = 0;
      // release write buffer
      p->bufTail = (p->bufTail + 1) & NUMBER_OF_BUFFERS_MASK;
      p->step    = 0;  // check for more to send
      break;
  }
}

static inline void checkSends_old(uint8_t const port)
{
  ringbuffer_t *const p = &rb[port];
  if ((p->bufHead == p->bufTail && p->bufHeadIndex == 0))
    return;  // nothing to send

  if (!USB_MIDI_IsConfigured(port) || (USB_MIDI_BytesToSend(port) > 0))
    return;  // not ready or still sending

  if (p->bufHead != p->bufTail)
  {  // send stashed buffers first
    if (!USB_MIDI_Send(port, txBuffer[port][p->bufTail], TX_BUFFERSIZE))
      ;  //status[port].dataLoss = TIMEOUT_DATALOSS;  // send failed
    else
    {
      ;  //status[port].midiTraffic = TIMEOUT_MIDI_TRAFFIC;  // success
      p->bufTail = (p->bufTail + 1) & NUMBER_OF_BUFFERS_MASK;
    }
    return;
  }
  if (p->bufHeadIndex != 0)
  {  // send current buffer
    if (!USB_MIDI_Send(port, txBuffer[port][p->bufHead], p->bufHeadIndex))
      ;  //status[port].dataLoss = TIMEOUT_DATALOSS;  // send failed
    else
    {
      ;  //status[port].midiTraffic = TIMEOUT_MIDI_TRAFFIC;  // success
      p->bufHead      = (p->bufHead + 1) & NUMBER_OF_BUFFERS_MASK;
      p->bufHeadIndex = 0;
      p->bufTail      = p->bufHead;
    }
  }
}
void MIDI_Relay_ProcessFast(void)
{
  checkSends(A);
  checkSends(B);
}

void MIDI_Relay_Process(void)
{
}
// ------------------------------------------------------------

static inline void ReceiveCallback(uint8_t const port, uint8_t *buff, uint32_t len)
{
  if (!fillBuffer(port ^ 1, buff, len))
    ;
}

void MIDI_Relay_Init(void)
{
  ringbufferReset();
  for (uint8_t port = 0; port <= 1; port++)
  {
    USB_MIDI_Config(port, ReceiveCallback);
    USB_MIDI_Init(port);
    USB_MIDI_Init(port);
  }
}
