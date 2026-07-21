/**********************************************************************************************************************
 *  FILE:         Flash_IP.h
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver)
 *  DESCRIPTION:  Register-level driver for the internal Flash memory of the STM32F401RE (single Flash bank,
 *                512 KB, 8 sectors, base address 0x08000000), based on RM0368 / PM0059:
 *                  - Sector 0..3 : 16 KB  each  (0x08000000 .. 0x0800FFFF)
 *                  - Sector 4    : 64 KB        (0x08010000 .. 0x0801FFFF)
 *                  - Sector 5..7 : 128 KB each  (0x08020000 .. 0x0807FFFF)
 *
 *                Does NOT use ST HAL or LL - accesses FLASH_ACR/KEYR/SR/CR directly through the CMSIS
 *                device header (FLASH_TypeDef, bit-mask macros), as is standard even for "bare-metal"
 *                STM32 projects.
 *
 *                Program/Erase are triggered and then completed asynchronously via the FLASH global
 *                interrupt (EOPIE/ERRIE), matching the asynchronous job model required by the AUTOSAR
 *                Mem driver (Mem.c / Mem_IPW.c) that sits on top of this file.
 *
 *  IMPORTANT (single Flash bank limitation, RM0368 chapter 3.4):
 *                The STM32F401 has a single Flash bank and does NOT support Read-While-Write. While a
 *                program/erase operation is in progress (BSY=1), any CPU bus access into the Flash
 *                address space - including instruction fetches - is automatically stalled by the AHB
 *                bus matrix until the operation completes. This means: (a) code executing from Flash
 *                will appear "frozen" for the duration of the operation (a few hundred us for a byte
 *                program, up to ~2 s worst-case for a 128 KB sector erase); (b) FLASH_IRQHandler() will
 *                only actually run once the stall ends, i.e. right when the operation completes - this
 *                is expected and NOT a bug. If your application cannot tolerate this stall (e.g. hard
 *                real-time tasks), run the calling code and vector table from RAM.
 *********************************************************************************************************************/

#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Std_Types.h"
#include "Flash_IP_Cfg.h"   /* Flash_IP_SectorType, Flash_IP_SectorTable[] - sector geometry (config data) */

/*======================================================================================================================
 *  Status of the currently / most recently triggered hardware operation
 *====================================================================================================================*/
typedef enum
{
    FLASH_IP_IDLE = 0,   /* no operation triggered since Init/last successful completion read-out */
    FLASH_IP_BUSY,       /* operation ongoing (BSY=1 or waiting for FLASH_IRQHandler)               */
    FLASH_IP_OK,         /* last operation completed successfully                                   */
    FLASH_IP_ERROR       /* last operation completed with a hardware error (WRPERR/PGAERR/...)      */
} Flash_IP_StatusType;

/*======================================================================================================================
 *  Lifecycle
 *====================================================================================================================*/
extern void Flash_IP_Init(void);
extern void Flash_IP_DeInit(void);

/*======================================================================================================================
 *  Status
 *====================================================================================================================*/
extern Flash_IP_StatusType Flash_IP_GetStatus(void);

/*======================================================================================================================
 *  Read - Flash is memory mapped, this is a plain synchronous copy.
 *====================================================================================================================*/
extern Std_ReturnType Flash_IP_Read(uint32 address, uint8* data, uint32 length);

/*======================================================================================================================
 *  Program / Erase - trigger only, completion is asynchronous (see Flash_IP_GetStatus()).
 *  Returns E_NOT_OK immediately if a job is already ongoing or parameters are invalid.
 *====================================================================================================================*/
extern Std_ReturnType Flash_IP_ProgramStart(uint32 address, const uint8* data, uint32 length);
extern Std_ReturnType Flash_IP_EraseSectorStart(uint8 sectorNumber);

/*======================================================================================================================
 *  Call cyclically (e.g. from Mem_Ipw_MainFunction()). Reserved for future watchdog/timeout supervision -
 *  actual completion is detected by FLASH_IRQHandler(), not by this function.
 *====================================================================================================================*/
extern void Flash_IP_MainFunction(void);

/*======================================================================================================================
 *  Sector geometry helpers
 *====================================================================================================================*/
extern uint8  Flash_IP_GetSectorFromAddress(uint32 address); /* returns 0xFFu if address is out of range */
extern uint32 Flash_IP_GetSectorStartAddress(uint8 sectorNumber);
extern uint32 Flash_IP_GetSectorSize(uint8 sectorNumber);

/*======================================================================================================================
 *  FLASH global interrupt handler.
 *  NOTE: STM32CubeMX/HAL projects usually generate an (empty) FLASH_IRQHandler() in stm32f4xx_it.c.
 *  Since this driver is bare-metal (no HAL), remove/rename any pre-existing FLASH_IRQHandler() definition
 *  elsewhere in the project so this one is the single, strong definition linked into the vector table.
 *====================================================================================================================*/
extern void FLASH_IRQHandler(void);

#endif /* FLASH_IP_H */