#include "MIDI_statemonitor.h"
#include "drv/nl_leds.h"

#define usToTicks(x) (1 + (x) / 125ul)            // usecs to 125 ticker counts
#define msToTicks(x) (1 + ((x) *1000ul) / 125ul)  // msecs to 125 ticker counts

#define LATE_TIME                  usToTicks(1000ul)  // time until packet is considered LATE
#define STALE_TIME                 msToTicks(20ul)    // time until packet is considered STALE
#define REALTIME_INDICATOR_TIMEOUT msToTicks(20ul)    // minimum hot display duration after end of real-time packets
#define DROPPED_INDICATOR_TIMEOUT  msToTicks(200ul)   // hot display time of any dropped incoming packets
#define LATE_INDICATOR_TIMEOUT     msToTicks(2000ul)  // display time of "had late packets recently"
#define STALE_INDICATOR_TIMEOUT    msToTicks(6000ul)  // display time of "had stale packets recently"
#define BLINK_TIME                 msToTicks(3000ul)  // blink cycle time for offline status display
#define BLINK_TIME_FIRST           msToTicks(4000ul)  // blink cycle time for offline status display
#define BLINK_TIME_ON_TIME         msToTicks(300ul)   // active portion of blink time
#define BLINK_TIME_OFF_TIME        (BLINK_TIME - BLINK_TIME_ON_TIME)

#define PWM_RELOAD (32)  // PWM ratio for dimmed LED

typedef enum
{
  DIM = 0,
  NORMAL,
  BRIGHT,
} Brightness_t;

typedef enum
{
  SOLID = 0,
  BLINKING,
} Blink_t;

typedef enum
{
  REALTIME,
  LATE,
  STALE,
} Latency_t;

static struct
{
  int powered;
  int online;

  unsigned  packetTime;
  Latency_t packetLatency;
  int       droppedIncoming;

  unsigned packetDisplayTimer;
  unsigned lateDisplayTimer;
  unsigned staleDisplayTimer;
} state[2];

static struct
{
  LedColor_t   color;
  Brightness_t bright;
  Blink_t      blink;
} led[2];

static unsigned blinkCntr = BLINK_TIME;
static unsigned pwmCntr   = PWM_RELOAD;

static inline void processLeds(void);
static inline void setupDisplay(uint8_t const port);
static inline void doDisplay(uint8_t const port);
static inline void setLED(uint8_t const port, LedColor_t const color, Brightness_t const bright, Blink_t const blink);
static inline void doTimers(void);

// may be called from within interrupt callbacks !
void SMON_monitorEvent(uint8_t const port, MonitorEvent_t const event)
{
  switch (event)
  {
    case UNPOWERED:
    case POWERED:
      state[port].powered = (event == POWERED);
      blinkCntr           = BLINK_TIME_FIRST;  // force blinker to restart with LED on
      break;

    case OFFLINE:
      state[port].online = 0;
      blinkCntr          = BLINK_TIME_FIRST;  // force blinker to restart with LED on
      break;

    case ONLINE:
      state[port].online = 1;
      break;

    case DROPPED_INCOMING:
      state[port].droppedIncoming = 1;
      break;

    case PACKET_START:
      state[port].packetTime = 1;  // start packet timer
      setupDisplay(port);          // start display ...
      doDisplay(port);             // ... immediately
      break;

    case PACKET_DELIVERED:
      state[port].packetTime = 0;
      break;

    case PACKET_DROPPED:
      state[port].packetTime = 0;
      break;
  }
}

void SMON_Process(void)
{
  processLeds();
  doTimers();
}

static inline void processLeds(void)
{
  setupDisplay(0);
  setupDisplay(1);
  doDisplay(0);
  doDisplay(1);
}

static inline void doTimers(void)
{
  for (int port = 0; port < 2; port++)
  {
    if (state[port].online && state[port].packetTime)
      state[port].packetTime++;

    if (state[port].packetDisplayTimer)
      state[port].packetDisplayTimer--;
    if (state[port].staleDisplayTimer)
      state[port].staleDisplayTimer--;
    if (state[port].lateDisplayTimer)
      state[port].lateDisplayTimer--;
  }

  if (!--pwmCntr)
    pwmCntr = PWM_RELOAD;
  if (!--blinkCntr)
    blinkCntr = BLINK_TIME;
}

static inline void setupDisplay(uint8_t const port)
{
  if (state[port].online)
  {
    if (state[port].packetTime)  // packet is running
    {
      state[port].packetDisplayTimer = REALTIME_INDICATOR_TIMEOUT;
      if (state[port].packetTime < LATE_TIME)
        state[port].packetLatency = REALTIME;
      else if (state[port].packetTime < STALE_TIME)
      {
        state[port].packetLatency    = LATE;
        state[port].lateDisplayTimer = LATE_INDICATOR_TIMEOUT;
      }
      else
      {
        state[port].packetLatency     = STALE;
        state[port].staleDisplayTimer = STALE_INDICATOR_TIMEOUT;
      }

      switch (state[port].packetLatency)
      {
        case REALTIME:
          if (state[port].staleDisplayTimer)
            setLED(port, COLOR_MAGENTA, BRIGHT, SOLID);
          else if (state[port].lateDisplayTimer)
            setLED(port, COLOR_YELLOW, BRIGHT, SOLID);
          else
            setLED(port, COLOR_CYAN, BRIGHT, SOLID);
          break;

        case LATE:
          setLED(port, COLOR_YELLOW, BRIGHT, SOLID);
          break;

        case STALE:
          setLED(port, COLOR_RED, BRIGHT, SOLID);
          break;
      }
    }
    else  // no packet running
    {
      if (state[port].droppedIncoming)
      {
        state[port].droppedIncoming    = 0;
        state[port].packetDisplayTimer = DROPPED_INDICATOR_TIMEOUT;
        state[port].staleDisplayTimer  = STALE_INDICATOR_TIMEOUT;
        state[port].packetLatency      = STALE;
      }

      if (state[port].packetDisplayTimer)  // packet display still running
      {
        switch (state[port].packetLatency)
        {
          case REALTIME:
            setLED(port, COLOR_GREEN, NORMAL, SOLID);
            break;
          case LATE:
            setLED(port, COLOR_YELLOW, NORMAL, SOLID);
            break;
          case STALE:
            setLED(port, COLOR_RED, NORMAL, SOLID);
            break;
        }
      }
      else  // no packets, show latency history
      {
        if (state[port].staleDisplayTimer)
          setLED(port, COLOR_RED, DIM, SOLID);
        else if (state[port].lateDisplayTimer)
          setLED(port, COLOR_YELLOW, DIM, SOLID);
        else
          setLED(port, COLOR_GREEN, DIM, SOLID);
      }
    }
  }
  else  // offline
  {
    if (state[port].powered)
      setLED(port, COLOR_MAGENTA, DIM, BLINKING);
    else
      setLED(port, COLOR_BLUE, DIM, BLINKING);
  }
}

static inline void doDisplay(uint8_t const port)
{
  int output = 0;
  switch (led[port].bright)
  {
    case DIM:
      output = (pwmCntr == PWM_RELOAD);
      break;
    case NORMAL:
      output = ((pwmCntr & 0b11) == 0);
      break;
    case BRIGHT:
      output = 1;
      break;
  }

  if (led[port].blink && (blinkCntr < BLINK_TIME_OFF_TIME))
    output = 0;
  if (output)
    LED_SetDirect(port, led[port].color);
  else
    LED_SetDirect(port, COLOR_OFF);
}

static inline void setLED(uint8_t const port, LedColor_t const color, Brightness_t const bright, Blink_t const blink)
{
  led[port].color  = color;
  led[port].bright = bright;
  led[port].blink  = blink;
}
