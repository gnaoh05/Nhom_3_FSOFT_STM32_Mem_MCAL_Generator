/**********************************************************************************************************************
 *  FILE:         Flash_IP_Cfg.h
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver) - Configuration
 *  DESCRIPTION:  Configuration data for Flash_IP: Flash geometry macros and the sector table of the
 *                STM32F401RE internal Flash (single Flash bank, 512 KB, 8 sectors, base 0x08000000),
 *                based on RM0368 / PM0059:
 *                  - Sector 0..3 : 16 KB  each  (0x08000000 .. 0x0800FFFF)
 *                  - Sector 4    : 64 KB        (0x08010000 .. 0x0801FFFF)
 *                  - Sector 5..7 : 128 KB each  (0x08020000 .. 0x0807FFFF)
 *
 *                Declarations only - the actual sector table data is defined in Flash_IP_Cfg.c.
 *                Regenerate/edit this pair of files if porting to a different STM32F4xx density/variant.
 *********************************************************************************************************************/

#ifndef FLASH_IP_CFG_H
#define FLASH_IP_CFG_H

#include "Std_Types.h"

/*======================================================================================================================
 *  Flash geometry (STM32F401RE - 512 KB, 8 sectors)
 *====================================================================================================================*/
#define FLASH_IP_BASE_ADDRESS       0x08000000UL
#define FLASH_IP_TOTAL_SIZE         0x00080000UL   /* 512 KB */
#define FLASH_IP_SECTOR_COUNT       8u
#define FLASH_IP_ERASED_VALUE       0xFFu

/*======================================================================================================================
 *  Sector table type and data (defined in Flash_IP_Cfg.c).
 *  Declared here as an INCOMPLETE array type ("[]") on purpose: the actual element count is determined by
 *  the initializer list in Flash_IP_Cfg.c, not repeated here, so that Flash_IP_Cfg.c's compile-time check
 *  (actual initializer count vs. FLASH_IP_SECTOR_COUNT) is meaningful - see [SWS_Mem_00033] note there.
 *====================================================================================================================*/
typedef struct
{
    uint32 StartAddress;
    uint32 Size;
} Flash_IP_SectorType;

extern const Flash_IP_SectorType Flash_IP_SectorTable[];

#endif /* FLASH_IP_CFG_H */