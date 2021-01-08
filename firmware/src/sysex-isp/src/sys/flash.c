#include <stdint.h>
#include "cmsis/LPC43xx.h"
#include "cmsis/lpc43xx_cgu.h"
#include "io/pins.h"

static uint32_t commandParam[5];
static uint32_t statusResult[5];

#define IAP_LOCATION *(volatile uint32_t *) (0x10400100);
typedef void (*IAP_t)(uint32_t[], uint32_t[]);
static IAP_t iapEntry;

#define IAP_CMD_SUCCESS (0)

static inline void LedA(uint8_t const rgb)
{
  LED_RED0   = !(rgb & 0b001);
  LED_GREEN0 = !(rgb & 0b010);
  LED_BLUE0  = !(rgb & 0b100);
}

static inline void LedB(uint8_t const rgb)
{
  LED_RED1   = !(rgb & 0b001);
  LED_GREEN1 = !(rgb & 0b010);
  LED_BLUE1  = !(rgb & 0b100);
}

void FLASH_Init(void)
{
  iapEntry = (IAP_t) IAP_LOCATION;
  CGU_UpdateClock();
}

static void iapInit(void)
{
  LedB(0b001);
  commandParam[0] = 49;  // CMD : Init
  iapEntry(commandParam, statusResult);
}

static int iapPrepSectorsForWrite(uint32_t const startSector, uint32_t const endSector, uint32_t const bank)
{
  LedB(0b010);
  commandParam[0] = 50;  // CMD : Prepare
  commandParam[1] = startSector;
  commandParam[2] = endSector;
  commandParam[3] = bank;
  iapEntry(commandParam, statusResult);
  return statusResult[0] == IAP_CMD_SUCCESS;
}

static int iapCopyRamToFlash(uint32_t const *const dest, uint32_t const *const src, uint32_t const bytes)
{
  LedB(0b100);
  commandParam[0] = 51;  // CMD : Write
  commandParam[1] = (uint32_t) dest;
  commandParam[2] = (uint32_t) src;
  commandParam[3] = bytes;
  commandParam[4] = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE) / 1000;
  iapEntry(commandParam, statusResult);
  return statusResult[0] == IAP_CMD_SUCCESS;
}

static int iapEraseSectors(uint32_t const startSector, uint32_t const endSector, uint32_t const bank)
{
  LedB(0b011);
  commandParam[0] = 52;  // CMD : Erase
  commandParam[1] = startSector;
  commandParam[2] = endSector;
  commandParam[3] = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE) / 1000;
  commandParam[4] = bank;
  iapEntry(commandParam, statusResult);
  return statusResult[0] == IAP_CMD_SUCCESS;
}

static int iapSelectActiveBootBank(uint32_t const bank)
{
  LedB(0b100);
  commandParam[0] = 60;  // CMD : SelectBoot
  commandParam[1] = bank;
  commandParam[2] = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE) / 1000;
  iapEntry(commandParam, statusResult);
  return statusResult[0] == IAP_CMD_SUCCESS;
}

// buf : data in RAM(!!), word boundary, there must be at least 36kB from start
// len : ignored
// bank: 0/1 (Bank A / Bank B)
// returns 1 on success, 0 otherwise
int flashMemory(uint32_t const *const buf, uint16_t const len, uint8_t const bank)
{
  static uint32_t *const flashBankAdr[2] = { (uint32_t *) 0x1A000000, (uint32_t *) 0x1B000000 };

  __disable_irq();
  LedA(0b000);
  iapInit();
  // Prep and erase 5 sectors 8kB each (40k total)
  if (iapPrepSectorsForWrite(0, 4, bank != 0) && iapEraseSectors(0, 4, bank != 0))
  {
    // Flash only 36kB of the buffer, 4k are left for us and the boot/init code of the ISP overlay
    // The "len" parameter is ignored, for now
    for (int i = 0; i < 9; i++)
    {  // prep those 5 sectors again and flash a 4k chunk of data
      LedA(i + 1);
      iapPrepSectorsForWrite(0, 4, bank != 0);
      if (!iapCopyRamToFlash(&(flashBankAdr[bank != 0][i * 1024]), &(buf[i * 1024]), 4096))
        return 0;
    }
    return 1;
  }
  return 0;
}
