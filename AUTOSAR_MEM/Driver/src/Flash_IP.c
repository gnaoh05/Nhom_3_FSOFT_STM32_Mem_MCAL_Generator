/**********************************************************************************************************************
 *  FILE:         Flash_IP.c
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver)
 *  DESCRIPTION:  See Flash_IP.h for module description and important single-bank limitations.
 *
 *                Register reference (RM0368 / PM0059):
 *                  FLASH_CR  : PG(0) SER(1) MER(2) SNB(6:3) PSIZE(9:8) STRT(16) EOPIE(24) ERRIE(25) LOCK(31)
 *                  FLASH_SR  : EOP(0) OPERR(1) WRPERR(4) PGAERR(5) PGPERR(6) PGSERR(7) BSY(16)
 *                  FLASH_KEYR: unlock sequence KEY1=0x45670123, KEY2=0xCDEF89AB
 *
 *  DEPENDENCY:   Requires the CMSIS device header for STM32F401 (e.g. "stm32f401xe.h", normally pulled in
 *                transitively via "stm32f4xx.h") for the FLASH peripheral struct/bit-mask macros and for
 *                NVIC_EnableIRQ()/FLASH_IRQn. This header ships with every STM32F4 CMSIS device pack, HAL
 *                or not, and is therefore assumed present even in a bare-metal (register-only) project.
 *********************************************************************************************************************/

#include "Flash_IP.h"
#include "stm32f401xe.h" /* CMSIS device header: FLASH_TypeDef, FLASH_CR_x / FLASH_SR_x bit masks, FLASH_IRQn */

/* NOTE: On the real 32-bit Cortex-M4 target, sizeof(void*) == sizeof(uint32), so the (volatile uint8*)
 * casts of a uint32 address below are exact-width and generate no warning. They only warn under
 * -Wint-to-pointer-cast when this file is compiled on a 64-bit host (e.g. for a quick desktop syntax
 * check), which is not the deployment target. */

/*======================================================================================================================
 *  Fallback bit-mask / position definitions.
 *  Guarded by #ifndef so this file still compiles unmodified against slightly older/newer CMSIS device
 *  header revisions that may not define every _Pos/_Msk variant.
 *====================================================================================================================*/
#ifndef FLASH_CR_PG
#define FLASH_CR_PG            (1UL << 0)
#endif
#ifndef FLASH_CR_SER
#define FLASH_CR_SER           (1UL << 1)
#endif
#ifndef FLASH_CR_MER
#define FLASH_CR_MER           (1UL << 2)
#endif
#ifndef FLASH_CR_SNB_Pos
#define FLASH_CR_SNB_Pos       3U
#endif
#ifndef FLASH_CR_SNB
#define FLASH_CR_SNB           (0xFUL << FLASH_CR_SNB_Pos)
#endif
#ifndef FLASH_CR_PSIZE_Pos
#define FLASH_CR_PSIZE_Pos     8U
#endif
#ifndef FLASH_CR_PSIZE
#define FLASH_CR_PSIZE         (0x3UL << FLASH_CR_PSIZE_Pos)
#endif
#ifndef FLASH_CR_STRT
#define FLASH_CR_STRT          (1UL << 16)
#endif
#ifndef FLASH_CR_EOPIE
#define FLASH_CR_EOPIE         (1UL << 24)
#endif
#ifndef FLASH_CR_ERRIE
#define FLASH_CR_ERRIE         (1UL << 25)
#endif
#ifndef FLASH_CR_LOCK
#define FLASH_CR_LOCK          (1UL << 31)
#endif

#ifndef FLASH_ACR_ICEN
#define FLASH_ACR_ICEN         (1UL << 9)
#endif
#ifndef FLASH_ACR_DCEN
#define FLASH_ACR_DCEN         (1UL << 10)
#endif
#ifndef FLASH_ACR_ICRST
#define FLASH_ACR_ICRST        (1UL << 11)
#endif
#ifndef FLASH_ACR_DCRST
#define FLASH_ACR_DCRST        (1UL << 12)
#endif

#ifndef FLASH_SR_EOP
#define FLASH_SR_EOP           (1UL << 0)
#endif
#ifndef FLASH_SR_OPERR
#define FLASH_SR_OPERR         (1UL << 1)
#endif
#ifndef FLASH_SR_WRPERR
#define FLASH_SR_WRPERR        (1UL << 4)
#endif
#ifndef FLASH_SR_PGAERR
#define FLASH_SR_PGAERR        (1UL << 5)
#endif
#ifndef FLASH_SR_PGPERR
#define FLASH_SR_PGPERR        (1UL << 6)
#endif
#ifndef FLASH_SR_PGSERR
#define FLASH_SR_PGSERR        (1UL << 7)
#endif
#ifndef FLASH_SR_BSY
#define FLASH_SR_BSY           (1UL << 16)
#endif

#define FLASH_IP_SR_ALL_ERROR_FLAGS  (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)
#define FLASH_IP_SR_ALL_CLEAR_FLAGS  (FLASH_SR_EOP | FLASH_IP_SR_ALL_ERROR_FLAGS)

#define FLASH_IP_KEY1                0x45670123UL
#define FLASH_IP_KEY2                0xCDEF89ABUL

#define FLASH_IP_PSIZE_BYTE          0x0UL   /* PSIZE = 00: x8  parallelism (byte programming)  */

/*======================================================================================================================
 *  Module state
 *  Sector geometry (Flash_IP_SectorType / Flash_IP_SectorTable[]) is defined in Flash_IP_Cfg.c and
 *  declared in Flash_IP_Cfg.h (included transitively via Flash_IP.h).
 *====================================================================================================================*/
typedef enum
{
    FLASH_IP_OP_NONE = 0,
    FLASH_IP_OP_PROGRAM,
    FLASH_IP_OP_ERASE
} Flash_IP_OperationType;

static volatile Flash_IP_StatusType    Flash_IP_Status    = FLASH_IP_IDLE;
static volatile Flash_IP_OperationType Flash_IP_CurrentOp = FLASH_IP_OP_NONE;

/* Program job progress - one byte is programmed per hardware operation / interrupt cycle.
 * Byte-wise programming (PSIZE = x8) is used so arbitrary, non-word-aligned addresses and lengths
 * (as may be requested by Mem_Write()) are supported without extra alignment/padding logic. */
static const uint8* Flash_IP_ProgSrcPtr;
static uint32        Flash_IP_ProgAddress;
static uint32        Flash_IP_ProgRemaining;

/*======================================================================================================================
 *  Local helpers
 *====================================================================================================================*/
static void Flash_IP_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0u)
    {
        FLASH->KEYR = FLASH_IP_KEY1;
        FLASH->KEYR = FLASH_IP_KEY2;
    }
}

static void Flash_IP_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void Flash_IP_ClearAllFlags(void)
{
    /* FLASH_SR error/EOP bits are rc_w1 (cleared by writing 1) */
    FLASH->SR = FLASH_IP_SR_ALL_CLEAR_FLAGS;
}

/* RM0368 3.5.5 notes that erase can leave stale entries in the flash I/D caches. Reset the caches only
 * after temporarily disabling them, then restore the previous enable state. */
static void Flash_IP_RefreshCachesAfterErase(void)
{
    uint32 cacheEnableMask = FLASH->ACR & (FLASH_ACR_ICEN | FLASH_ACR_DCEN);

    FLASH->ACR &= ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    FLASH->ACR |= FLASH_ACR_ICRST | FLASH_ACR_DCRST;
    FLASH->ACR &= ~(FLASH_ACR_ICRST | FLASH_ACR_DCRST);
    FLASH->ACR |= cacheEnableMask;
}

/* Triggers programming of exactly one byte at Flash_IP_ProgAddress from *Flash_IP_ProgSrcPtr.
 * Completion (success or error) is reported later by FLASH_IRQHandler(). */
static void Flash_IP_TriggerNextByte(void)
{
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= (FLASH_IP_PSIZE_BYTE << FLASH_CR_PSIZE_Pos);
    FLASH->CR |= FLASH_CR_PG | FLASH_CR_EOPIE | FLASH_CR_ERRIE;

    *(volatile uint8*)(Flash_IP_ProgAddress) = *Flash_IP_ProgSrcPtr;
}

/*======================================================================================================================
 *  Lifecycle
 *====================================================================================================================*/
void Flash_IP_Init(void)
{
    Flash_IP_Status    = FLASH_IP_IDLE;
    Flash_IP_CurrentOp = FLASH_IP_OP_NONE;

    Flash_IP_Lock(); /* ensure Flash starts locked */
    Flash_IP_ClearAllFlags();

    NVIC_EnableIRQ(FLASH_IRQn);
}

void Flash_IP_DeInit(void)
{
    /* Best-effort cancel: disable interrupt sources and lock the Flash. A hardware operation already
     * in progress (BSY=1) cannot be aborted - the bus will stall as documented in Flash_IP.h until it
     * completes on its own. */
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
    Flash_IP_Lock();

    Flash_IP_Status    = FLASH_IP_IDLE;
    Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
}

/*======================================================================================================================
 *  Status
 *====================================================================================================================*/
Flash_IP_StatusType Flash_IP_GetStatus(void)
{
    return Flash_IP_Status;
}

/*======================================================================================================================
 *  Read (synchronous - Flash is memory mapped)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_Read(uint32 address, uint8* data, uint32 length)
{
    uint32 i;

    for (i = 0u; i < length; i++)
    {
        data[i] = *(volatile uint8*)(address + i);
    }

    return E_OK;
}

/*======================================================================================================================
 *  Program (asynchronous, byte-wise, interrupt driven)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_ProgramStart(uint32 address, const uint8* data, uint32 length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Flash_IP_Status != FLASH_IP_BUSY) && (length > 0u))
    {
        Flash_IP_ProgSrcPtr    = data;
        Flash_IP_ProgAddress   = address;
        Flash_IP_ProgRemaining = length;
        Flash_IP_CurrentOp     = FLASH_IP_OP_PROGRAM;
        Flash_IP_Status        = FLASH_IP_BUSY;

        Flash_IP_Unlock();
        Flash_IP_ClearAllFlags();
        Flash_IP_TriggerNextByte();

        retVal = E_OK;
    }

    return retVal;
}

/*======================================================================================================================
 *  Erase one sector (asynchronous, interrupt driven)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_EraseSectorStart(uint8 sectorNumber)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Flash_IP_Status != FLASH_IP_BUSY) && (sectorNumber < FLASH_IP_SECTOR_COUNT))
    {
        Flash_IP_CurrentOp = FLASH_IP_OP_ERASE;
        Flash_IP_Status    = FLASH_IP_BUSY;

        Flash_IP_Unlock();
        Flash_IP_ClearAllFlags();

        FLASH->CR &= ~FLASH_CR_SNB;
        FLASH->CR |= ((uint32)sectorNumber << FLASH_CR_SNB_Pos) | FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE;
        FLASH->CR |= FLASH_CR_STRT;

        retVal = E_OK;
    }

    return retVal;
}

/*======================================================================================================================
 *  Scheduling - completion is interrupt driven, nothing to poll here today. Reserved for future
 *  timeout/watchdog supervision (e.g. detect a stuck BSY bit if the interrupt is ever lost/masked).
 *====================================================================================================================*/
void Flash_IP_MainFunction(void)
{
    /* intentionally empty - see header comment */
}

/*======================================================================================================================
 *  Sector geometry helpers
 *====================================================================================================================*/
uint8 Flash_IP_GetSectorFromAddress(uint32 address)
{
    uint8 sector;
    uint8 found = FLASH_IP_SECTOR_COUNT; /* used as "not found" sentinel, cast to 0xFF below */

    for (sector = 0u; sector < FLASH_IP_SECTOR_COUNT; sector++)
    {
        uint32 start = Flash_IP_SectorTable[sector].StartAddress;
        uint32 end   = start + Flash_IP_SectorTable[sector].Size; /* exclusive */

        if ((address >= start) && (address < end))
        {
            found = sector;
            break;
        }
    }

    return (found == FLASH_IP_SECTOR_COUNT) ? 0xFFu : found;
}

uint32 Flash_IP_GetSectorStartAddress(uint8 sectorNumber)
{
    uint32 result = 0u;

    if (sectorNumber < FLASH_IP_SECTOR_COUNT)
    {
        result = Flash_IP_SectorTable[sectorNumber].StartAddress;
    }

    return result;
}

uint32 Flash_IP_GetSectorSize(uint8 sectorNumber)
{
    uint32 result = 0u;

    if (sectorNumber < FLASH_IP_SECTOR_COUNT)
    {
        result = Flash_IP_SectorTable[sectorNumber].Size;
    }

    return result;
}

/*======================================================================================================================
 *  FLASH global interrupt handler
 *  Fires on EOP (successful completion) or on any of the error flags, as enabled via EOPIE/ERRIE in
 *  Flash_IP_TriggerNextByte()/Flash_IP_EraseSectorStart(). See Flash_IP.h for the single-bank stall
 *  behavior that determines exactly when this handler actually executes.
 *====================================================================================================================*/
void FLASH_IRQHandler(void)
{
    uint32 sr = FLASH->SR;

    if ((sr & FLASH_IP_SR_ALL_ERROR_FLAGS) != 0u)
    {
        Flash_IP_ClearAllFlags();
        FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
        Flash_IP_Lock();

        Flash_IP_Status    = FLASH_IP_ERROR;
        Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
    }
    else if ((sr & FLASH_SR_EOP) != 0u)
    {
        FLASH->SR = FLASH_SR_EOP; /* clear EOP (rc_w1) */

        if (Flash_IP_CurrentOp == FLASH_IP_OP_PROGRAM)
        {
            FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_EOPIE | FLASH_CR_ERRIE);

            Flash_IP_ProgSrcPtr++;
            Flash_IP_ProgAddress++;
            Flash_IP_ProgRemaining--;

            if (Flash_IP_ProgRemaining == 0u)
            {
                Flash_IP_Lock();
                Flash_IP_Status    = FLASH_IP_OK;
                Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
            }
            else
            {
                /* still BUSY - program next byte (re-unlock not needed, LOCK was not set) */
                Flash_IP_TriggerNextByte();
            }
        }
        else if (Flash_IP_CurrentOp == FLASH_IP_OP_ERASE)
        {
            FLASH->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
            Flash_IP_Lock();
            Flash_IP_RefreshCachesAfterErase();

            Flash_IP_Status    = FLASH_IP_OK;
            Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
        }
        else
        {
            /* Spurious EOP with no operation tracked - clear and ignore. */
        }
    }
    else
    {
        /* Spurious interrupt (neither EOP nor error flag set) - nothing to do. */
    }
}
