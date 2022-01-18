/* Define each memory region, for M4 core */
/* Suitable for LP4337 and LPC4333 */
MEMORY
{
  MFlashA256 (   rx) : ORIGIN = 0x1a000000, LENGTH = 0x40000   /* 256K bytes (alias Flash)  -- code and initialized data image */  
  MFlashB256    (rx) : ORIGIN = 0x1b000000, LENGTH = 0x40000   /* 256K bytes (alias Flash2) -- unused */  
  RamLoc40     (rwx) : ORIGIN = 0x10080000, LENGTH = 0xa000    /* 40K bytes  (alias RAM1)   -- data upload buffer */  
  RamAHB32     (rwx) : ORIGIN = 0x20000000, LENGTH = 0x8000    /* 32K bytes  (alias RAM2)   -- Shared between application and flasher */
  RamAHB16     (rwx) : ORIGIN = 0x20008000, LENGTH = 0x4000    /* 16K bytes  (alias RAM3)   -- Main RAM for DATA and STACK */  
  RamLoc32     (rwx) : ORIGIN = 0x10000000, LENGTH = 0x8000    /* 32K bytes  (alias RAM4)   -- Unused */
  RamAHB_ETB16 (rwx) : ORIGIN = 0x2000c000, LENGTH = 0x4000    /* 16K bytes  (alias RAM5)   -- Unused */
}

/* Define a symbol for base and top of each memory region */
  __base_MFlashA256 = 0x1a000000  ; /* MFlashA256 */  
  __base_Flash = 0x1a000000 ;       /* Flash */  
  __top_MFlashA256 = 0x1a000000 + 0x40000 ; /* 256K bytes */  
  __top_Flash = 0x1a000000 + 0x40000 ;      /* 256K bytes */  

  __base_MFlashB256 = 0x1b000000  ; /* MFlashB256 */  
  __base_Flash2 = 0x1b000000 ;      /* Flash2 */  
  __top_MFlashB256 = 0x1b000000 + 0x40000 ; /* 256K bytes */  
  __top_Flash2 = 0x1b000000 + 0x40000 ;     /* 256K bytes */  

  __base_RamLoc40 = 0x10080000  ; /* RamLoc40 */  
  __top_RamLoc40 = 0x10080000 + 0xa000 ; /* 40K bytes */  

  __base_RamAHB32 = 0x20000000  ; /* RamAHB32 */  
  __top_RamAHB32 = 0x20000000 + 0x8000 ; /* 32K bytes */  

  __base_RamAHB16 = 0x20008000  ; /* RamAHB16 */  
  __top_RamAHB16 = 0x20008000 + 0x4000 ; /* 16K bytes */  

  __base_RamLoc32 = 0x10000000  ; /* RamLoc32 */  
  __top_RamLoc32 = 0x10000000 + 0x8000 ; /* 32K bytes */  

  __base_RamAHB_ETB16 = 0x2000c000  ; /* RamAHB_ETB16 */  
  __top_RamAHB_ETB16 = 0x2000c000 + 0x4000 ; /* 16K bytes */  
