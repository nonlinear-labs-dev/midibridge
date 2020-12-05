/******************************************************************************/
/** @file	Emphase_M4_Main.c
    @date	2016-03-09 SSC
	@changes 2020-05-11 KSTR
*******************************************************************************/

#pragma GCC diagnostic ignored "-Wmain"

#include <stdint.h>

#include "sys/nl_watchdog.h"
#include "CPU_clock.h"
#include "drv/nl_leds.h"
#include "drv/nl_cgu.h"
#include "io/pins.h"
#include "midi/MIDI_relay.h"
#include "midi/MIDI_statemonitor.h"
#include "sys/nl_version.h"
#include "sys/ticker.h"

#define WATCHDOG_TIMEOUT_MS (100ul)  // timeout in ms

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

  /* M4 sysTick */
  M4SysTick_Init();

  /* watchdog */
  // SYS_WatchDogInit(WATCHDOG_TIMEOUT_MS);
#warning "watchdog is off!"
}

/******************************************************************************/

static int periodicTimer = TIME_SLICE;
static int trigger       = 0;

void main(void)
{
  Init();

  /* MIDI relay */
  MIDI_Relay_Init();  // we wait until here because USB handlers are interrupt-driven and everything has to be set up

  while (1)
  {
    MIDI_Relay_ProcessFast();
    if (trigger)
    {
      trigger = 0;
      SMON_Process();
    }
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
  if (!--periodicTimer)
  {
    periodicTimer = TIME_SLICE;
    trigger       = 1;
  }
}
