/******************************************************************************/
/** @file	Emphase_M4_Main.c
    @date	2016-03-09 SSC
	@changes 2020-05-11 KSTR
*******************************************************************************/

#pragma GCC diagnostic ignored "-Wmain"

#include <stdint.h>

#include "sys/nl_coos.h"
#include "sys/nl_watchdog.h"
#include "CPU_clock.h"
#include "drv/nl_leds.h"
#include "drv/nl_cgu.h"
#include "io/pins.h"
#include "midi/MIDI_relay.h"
#include "midi/MIDI_statemonitor.h"
#include "sys/nl_version.h"
#include "sys/ticker.h"

#define DBG_CLOCK_MONITOR (0)

#define WATCHDOG_TIMEOUT_MS (100ul)  // timeout in ms

static volatile uint16_t waitForFirstSysTick = 1;

void M4SysTick_Init(void);

// --- all 125us processes combined in one single tasks
void FastProcesses(void)
{
  SYS_WatchDogClear();  // every 125 us, clear Watchdog
}

volatile char dummy;

void dummyFunction(const char *string)
{
  while (*string)
  {
    dummy = *string++;
  }
}

void Init(void)
{
  // referencing the version string so compiler won't optimize it away
  dummyFunction(VERSION_STRING);

  /* CPU clock */
  CPU_ConfigureClocks();

  /* I/O pins */
  PINS_Init();

  /* scheduler */
  COOS_Init();

  // clang-format off
  COOS_Task_Add(SYS_WatchDogClear,  0, 1);  // every 125 us
  COOS_Task_Add(SMON_Process,       1, 1);  // every 125 us  // State Monitor LEDs
  // clang-format on

  /* M4 sysTick */
  M4SysTick_Init();

  /* watchdog */
  // SYS_WatchDogInit(WATCHDOG_TIMEOUT_MS);
#warning "watchdog is off!"
}

/******************************************************************************/
void main(void)
{
  Init();

  /* MIDI relay */
  MIDI_Relay_Init();  // we wait until here because USB handlers are interrupt-driven and everything has to be set up

  while (1)
  {
    MIDI_Relay_ProcessFast();
    // USB_MIDI_Poll(0);  // Send/receive MIDI data, may do callbacks
    // USB_MIDI_Poll(1);  // Send/receive MIDI data, may do callbacks
    COOS_Dispatch();  // Standard dispatching of the slower stuff
  }
}

/*************************************************************************/ /**
* @brief	ticker interrupt routines using the standard M4 system ticker
*           IRQ will be triggered every 125us, our basic scheduler time-slice
******************************************************************************/
void M4SysTick_Init(void)
{
#define SYST_CSR   (uint32_t *) (0xE000E010)  // SysTick Control & Status Reg
#define SYST_RVR   (uint32_t *) (0xE000E014)  // SysTick Reload Value
#define SYST_CVR   (uint32_t *) (0xE000E018)  // SysTick Counter Value
#define SYST_CALIB (uint32_t *) (0xE000E01C)  // SysTick Calibration
  *SYST_RVR = (M4coreClock / M4_FREQ_HZ) - 1;
  *SYST_CVR = 0;
  *SYST_CSR = 0b111;  // processor clock | IRQ enabled | counter enabled
}

void SysTick_Handler(void)
{
  ticker++;
  COOS_Update();
}
