MEMORY
{
  /* Define each memory region */
  RamLoc40     (rwx) : ORIGIN = 0x10080000, LENGTH = 0xa000    /* 40K bytes  -- data upload buffer */  
  RamAHB32     (rwx) : ORIGIN = 0x20000000, LENGTH = 0x8000    /* 32K bytes  (alias RAM2)   -- Shared between application and flasher */

}

  /* Define a symbol for the top of each memory region */
  __base_RamLoc40 = 0x10080000  ; /* RamLoc32 */  
  __base_RAM = 0x10080000 ; /* RAM */  
  __top_RamLoc40 = 0x10080000 + 0xa000 ; /* 32K bytes */  
  __top_RAM = 0x10080000 + 0xa000 ; /* 32K bytes */  

  __base_RamAHB32 = 0x20000000  ; /* RamAHB32 */  
  __top_RamAHB32 = 0x20000000 + 0x8000 ; /* 32K bytes */  
