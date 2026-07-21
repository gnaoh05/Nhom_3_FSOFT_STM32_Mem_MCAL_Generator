/**********************************************************************************************************************
 *  FILE:         Flash_IP_Cfg.c
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver) - Configuration
 *  DESCRIPTION:  Sector geometry table for the STM32F401RE internal Flash (512 KB, 8 sectors).
 *                See Flash_IP_Cfg.h for the type definition and macro documentation.
 *********************************************************************************************************************/

#include "Flash_IP_Cfg.h"

const Flash_IP_SectorType Flash_IP_SectorTable[] =
{
    /* StartAddress    Size          */
    { 0x08000000UL, 0x00004000UL }, /* Sector 0: 16 KB  */
    { 0x08004000UL, 0x00004000UL }, /* Sector 1: 16 KB  */
    { 0x08008000UL, 0x00004000UL }, /* Sector 2: 16 KB  */
    { 0x0800C000UL, 0x00004000UL }, /* Sector 3: 16 KB  */
    { 0x08010000UL, 0x00010000UL }, /* Sector 4: 64 KB  */
    { 0x08020000UL, 0x00020000UL }, /* Sector 5: 128 KB */
    { 0x08040000UL, 0x00020000UL }, /* Sector 6: 128 KB */
    { 0x08060000UL, 0x00020000UL }  /* Sector 7: 128 KB */
};

/*======================================================================================================================
 *  [SWS_Mem_00033] "The Mem driver shall check static configuration parameters statically (at the latest
 *  during compile time) for correctness."
 *
 *  The two checks below catch a mismatched edit of FLASH_IP_SECTOR_COUNT vs. the table above (array-size
 *  check) and a mismatched edit of FLASH_IP_TOTAL_SIZE vs. the sum of the individual sector sizes
 *  (total-size check) - turning a silent, runtime-corrupting configuration error (e.g. sector geometry no
 *  longer covering the whole Flash, or overlapping) into a compile-time failure. Implemented with the
 *  classic "negative array size" trick for C90/C99 portability (no _Static_assert available pre-C11).
 *====================================================================================================================*/
#define FLASH_IP_CFG_STATIC_ASSERT(cond, uniqueName) typedef char uniqueName[(cond) ? 1 : -1]

FLASH_IP_CFG_STATIC_ASSERT(
        (sizeof(Flash_IP_SectorTable) / sizeof(Flash_IP_SectorTable[0])) == FLASH_IP_SECTOR_COUNT,
        Flash_IP_Cfg_SectorTableSizeCheck);

#define FLASH_IP_CFG_SECTOR_TOTAL_SIZE_LITERAL \
        (0x00004000UL + 0x00004000UL + 0x00004000UL + 0x00004000UL + \
         0x00010000UL + 0x00020000UL + 0x00020000UL + 0x00020000UL)

FLASH_IP_CFG_STATIC_ASSERT(
        FLASH_IP_CFG_SECTOR_TOTAL_SIZE_LITERAL == FLASH_IP_TOTAL_SIZE,
        Flash_IP_Cfg_TotalSizeCheck);