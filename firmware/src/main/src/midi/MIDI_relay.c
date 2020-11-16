#include "MIDI_relay.h"
#include "usb/nl_usb_core.h"
#include "usb/nl_usb_midi.h"
#include "usb/nl_usb_descmidi.h"
#include "io/pins.h"
#include "sys/globals.h"
#include "drv/nl_leds.h"
#include "sys/nl_version.h"
#include "sys/nl_stdlib.h"

#define TX_BUFFERSIZE          (4096)  // buffers for outgoing messages
#define NUMBER_OF_BUFFERS      (4)     // number of buffers per port, must be 2^N and >= 2 !
#define NUMBER_OF_BUFFERS_MASK (NUMBER_OF_BUFFERS - 1)

#define A (0)
#define B (1)

// ringbuffer of buffers
__attribute__((section(".noinit.$RamLoc32"))) static uint8_t txBuffer[2][NUMBER_OF_BUFFERS][TX_BUFFERSIZE];

typedef struct
{
  uint16_t bufIn;       // 0..(NUMBER_OF_BUFFERS-1)
  int16_t  bufInUse;    // buffers in use
  uint16_t bufInIndex;  // used bytes in the current head (front) buffer
  uint16_t bufOut;      // 0..(NUMBER_OF_BUFFERS-1)
  uint8_t  step;
} ringbuffer_t;

static ringbuffer_t rb[2];

// ------------- buffer functions
static inline void RB_Reset(uint8_t const bufno)
{
  rb[bufno].bufIn      = 0;
  rb[bufno].bufOut     = 0;
  rb[bufno].bufInIndex = 0;
  rb[bufno].bufInUse   = 0;
  rb[bufno].step       = 0;
}

static inline int RB_BuffersInUse(uint8_t const bufno)  // -1 when severe counting error
{
  ringbuffer_t *const p = &rb[bufno];
  if ((p->bufInUse >= 0) && (p->bufInUse <= NUMBER_OF_BUFFERS))
    return p->bufInUse;
  return -1;
}

static inline int RB_RemaingBuffers(uint8_t const bufno)  // -1 when severe counting error
{
  ringbuffer_t *const p = &rb[bufno];
  if ((p->bufInUse >= 0) && (p->bufInUse <= NUMBER_OF_BUFFERS))
    return NUMBER_OF_BUFFERS - p->bufInUse;
  return -1;
}

static inline int RB_Alloc(uint8_t const bufno)
{
  ringbuffer_t *const p = &rb[bufno];
  if (RB_RemaingBuffers(bufno) == 0)  // no space left ?
    return 0;
  p->bufIn      = (p->bufIn + 1) & NUMBER_OF_BUFFERS_MASK;
  p->bufInIndex = 0;
  p->bufInUse++;
  if (p->bufInUse == 1)    // if buffer chain was empty before
    p->bufOut = p->bufIn;  // set transmit buffer to this
  return 1;
}

static inline void RB_Free(uint8_t const bufno)
{
  ringbuffer_t *const p = &rb[bufno];
  if (RB_BuffersInUse(bufno) <= 0)
    LED_SetDirectAndHalt(0b111);
  p->bufOut = (p->bufOut + 1) & NUMBER_OF_BUFFERS_MASK;
  p->bufInUse--;
}

static inline int RB_InputIsFull(uint8_t const bufno)
{
  ringbuffer_t *const p = &rb[bufno];
  return (p->bufInIndex >= TX_BUFFERSIZE);
}

static inline int RB_PutByte(uint8_t const bufno, uint8_t const byte)
{
  ringbuffer_t *const p = &rb[bufno];
  if (p->bufInIndex >= TX_BUFFERSIZE)
    p->bufInIndex = 0;
  if (p->bufInIndex == 0)  // open a new buffer ?
  {
    if (RB_Alloc(bufno) == 0)
      return 0;
  }
  txBuffer[bufno][p->bufIn][p->bufInIndex++] = byte;
  return 1;
}

static inline uint16_t RB_GetTxSize(uint8_t const bufno)
{
  ringbuffer_t *const p = &rb[bufno];
  if (RB_BuffersInUse(bufno) == 1)  // single buffer ?
    return p->bufInIndex;
  else if (RB_BuffersInUse(bufno) > 1)  // stashed buffers ?
    return TX_BUFFERSIZE;
  return 0;
}
// ------------- end of buffer functions

// >= 0 == ok. -1 = data loss
static inline int fillBuffer(uint8_t const port, uint8_t *buff, uint32_t len)
{

  ringbuffer_t *const p                = &rb[port];
  ringbuffer_t const  savedBufferState = *p;
  int                 ret              = len;

  while (len)
  {
    len--;

    uint8_t const c = *(buff++);  // copy the byte
    if (!RB_PutByte(port, c))
    {
      *p = savedBufferState;
      return -1;
    }

#if 0
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
#endif
  }
  //  p->bufHead      = savedBufHead; // ???
  //  p->bufHeadIndex = savedBufHeadIndex; // ???
  return ret;
}

// ------------------------------------------------------------------

typedef struct
{
  uint8_t *buff;
  int32_t  len;
} PendingData_t;

static PendingData_t pendingData[2];

static inline void checkSends(uint8_t const port)
{
  uint8_t const incomingPort = port ^ 1;
  if (!USB_MIDI_IsConfigured(port))
  {
    USB_MIDI_SuspendReceive(incomingPort, 0);
    pendingData[port].len = 0;
    return;
  }

  if (pendingData[port].len > 0)
  {
    if (USB_MIDI_Send(port, pendingData[port].buff, pendingData[port].len))  // "start transmit" succeeded ?
    {
      pendingData[port].len = -1;  // mark transmit started
      return;
    }
  }

  if (pendingData[port].len != -1)
    return;  // no transmit running

  if (USB_MIDI_BytesToSend(port) > 0)
    return;

  if (USB_MIDI_BytesToSend(port) == -1)
    LED_SetDirectAndHalt(0b100);

  if (port == 0)
    LED_DBG1 = 0;
  else
    LED_DBG2 = 0;

  USB_MIDI_SuspendReceive(incomingPort, 0);  // resume receiver

#if 0
  ringbuffer_t *const p = &rb[port];

  if (!USB_MIDI_IsConfigured(port))
  {
    RB_Reset(port);
    return;
  }

  switch (p->step)
  {
    case 0:  // wait for jobs
    {
      uint16_t txSize = RB_GetTxSize(port);
      if (!txSize)
        break;
      if (USB_MIDI_Send(port, txBuffer[port][p->bufOut], txSize))
        p->step = 1;  // send accepted-->wait, else retry

      if (port == 0)
        LED_DBG1 = 1;
      else
        LED_DBG2 = 1;
    }
    break;

    case 1:  // wait for send to complete
      if (USB_MIDI_BytesToSend(port) > 0)
        break;

      p->step = 0;    // check for incoming jobs
      RB_Free(port);  // release write buffer

      if (port == 0)
        LED_DBG1 = 0;
      else
        LED_DBG2 = 0;
      break;
  }
#endif
}

void MIDI_Relay_ProcessFast(void)
{
  checkSends(A);
  checkSends(B);
  // LED_DBG3 = (USB_MIDI_SuspendReceiveGet(0) || USB_MIDI_SuspendReceiveGet(1));
#if 0
  LED_DBG3 = 0;
  for (int i = 0; i <= 1; i++)
  {
    int freeBuffers = RB_RemaingBuffers(i);
    if (freeBuffers < 0)
      LED_SetDirectAndHalt(0b101);
    if (freeBuffers < 3)
      LED_DBG3 = 1;
    USB_MIDI_SuspendReceive(i ^ 1, (freeBuffers < 3));
  }
#endif
}

void MIDI_Relay_Process(void)
{
}
// ------------------------------------------------------------

static inline void ReceiveCallback(uint8_t const port, uint8_t *buff, uint32_t len)
{
  uint8_t const outgoingPort = port ^ 1;
  if (!USB_MIDI_IsConfigured(outgoingPort))  // output side not ready ?
  {
    USB_MIDI_SuspendReceive(port, 0);
    pendingData[outgoingPort].len = 0;
    return;
  }

  if (len == 0)
  {
    LED_DBG3 = 1;
    return;
  }
  LED_DBG3 = 0;

  if ((outgoingPort) == 0)
    LED_DBG1 = 1;
  else
    LED_DBG2 = 1;

  USB_MIDI_SuspendReceive(port, 1);  // suspend receiver until transmit finished

  pendingData[outgoingPort].buff = buff;
  pendingData[outgoingPort].len  = len;

#if 0
  int ret = fillBuffer(port ^ 1, buff, len);
  if (ret >= 0)  // success
    ;
  else  // had to buffer or even data loss --> suspend
    ;
#endif
}

void MIDI_Relay_Init(void)
{
  for (uint8_t port = 0; port <= 1; port++)
  {
    RB_Reset(port);
    USB_MIDI_Config(port, ReceiveCallback);
    USB_MIDI_Init(port);
    USB_MIDI_Init(port);
  }
}
