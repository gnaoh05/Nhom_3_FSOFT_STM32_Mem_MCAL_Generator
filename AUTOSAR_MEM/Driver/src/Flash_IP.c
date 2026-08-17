#include <Stm32F401_BareMetal.h>
#include "Flash_IP.h"

#define FLASH_ACR_LATENCY_MASK        0x7UL
#define FLASH_ACR_PRFTEN              (1UL << 8)
#define FLASH_SR_RDERR                (1UL << 8)

#define FLASH_IP_SR_ERROR_FLAGS \
        (STM32_FLASH_SR_OPERR | STM32_FLASH_SR_WRPERR | STM32_FLASH_SR_PGAERR | \
         STM32_FLASH_SR_PGPERR | STM32_FLASH_SR_PGSERR | FLASH_SR_RDERR)
#define FLASH_IP_SR_CLEAR_FLAGS       (STM32_FLASH_SR_EOP | FLASH_IP_SR_ERROR_FLAGS)
#define FLASH_IP_KEY1                 0x45670123UL
#define FLASH_IP_KEY2                 0xCDEF89ABUL

#if (FLASH_IP_REGISTER_BASE_ADDRESS != STM32_FLASH_REG_BASE)
#error "Flash_IP_Cfg register base does not match the STM32F401 bare-metal register map."
#endif

typedef enum
{
    FLASH_IP_OP_NONE = 0,
    FLASH_IP_OP_PROGRAM,
    FLASH_IP_OP_ERASE
} Flash_IP_OperationType;

static volatile Flash_IP_StatusType Flash_IP_Status = FLASH_IP_IDLE;
static Flash_IP_OperationType       Flash_IP_Operation = FLASH_IP_OP_NONE;
static const uint8*                 Flash_IP_SourcePtr;
static uint32                       Flash_IP_TargetAddress;
static uint32                       Flash_IP_RemainingLength;
static uint32                       Flash_IP_TimeoutCounter;
static boolean                      Flash_IP_CancelRequested;

static uint32 Flash_IP_ProgramUnitSize(void)
{
    return (uint32)(1UL << (uint32)FLASH_IP_PSIZE);
}

static boolean Flash_IP_IsRangeValid(uint32 address, uint32 length)
{
    const uint32 flashEnd = FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE;

    return (boolean)((length > 0u) &&
                     (address >= FLASH_IP_BASE_ADDRESS) &&
                     (address < flashEnd) &&
                     (length <= (flashEnd - address)));
}

static void Flash_IP_Unlock(void)
{
    if ((STM32_FLASH->CR & STM32_FLASH_CR_LOCK) != 0u)
    {
        STM32_FLASH->KEYR = FLASH_IP_KEY1;
        STM32_FLASH->KEYR = FLASH_IP_KEY2;
    }
}

static void Flash_IP_Lock(void)
{
    STM32_FLASH->CR |= STM32_FLASH_CR_LOCK;
}

static void Flash_IP_ClearFlags(void)
{
    STM32_FLASH->SR = FLASH_IP_SR_CLEAR_FLAGS;
}

static void Flash_IP_ClearOperationBits(void)
{
    STM32_FLASH->CR &= ~(STM32_FLASH_CR_PG |
                         STM32_FLASH_CR_SER |
                         STM32_FLASH_CR_MER |
                         STM32_FLASH_CR_SNB |
                         STM32_FLASH_CR_EOPIE |
                         STM32_FLASH_CR_ERRIE);
}

static void Flash_IP_RefreshCaches(void)
{
    uint32 acr = STM32_FLASH->ACR;
    uint32 enabled = acr & (STM32_FLASH_ACR_ICEN | STM32_FLASH_ACR_DCEN);

    STM32_FLASH->ACR = acr & ~(STM32_FLASH_ACR_ICEN | STM32_FLASH_ACR_DCEN);
    STM32_FLASH->ACR |= STM32_FLASH_ACR_ICRST | STM32_FLASH_ACR_DCRST;
    STM32_FLASH->ACR &= ~(STM32_FLASH_ACR_ICRST | STM32_FLASH_ACR_DCRST);
    STM32_FLASH->ACR |= enabled;
}

static uint64 Flash_IP_LoadLittleEndian(const uint8* source, uint32 length)
{
    uint64 value = 0u;
    uint32 index;

    for (index = 0u; index < length; index++)
    {
        value |= ((uint64)source[index] << (index * 8u));
    }

    return value;
}

static void Flash_IP_TriggerProgramUnit(void)
{
    const uint32 unitSize = Flash_IP_ProgramUnitSize();
    const uint64 value = Flash_IP_LoadLittleEndian(Flash_IP_SourcePtr, unitSize);

    STM32_FLASH->CR &= ~STM32_FLASH_CR_PSIZE;
    STM32_FLASH->CR |= ((uint32)FLASH_IP_PSIZE << STM32_FLASH_CR_PSIZE_POS) |
                       STM32_FLASH_CR_PG;

    switch (unitSize)
    {
        case 1u:
            *(volatile uint8*)(uintptr_t)Flash_IP_TargetAddress = (uint8)value;
            break;
        case 2u:
            *(volatile uint16*)(uintptr_t)Flash_IP_TargetAddress = (uint16)value;
            break;
        case 4u:
            *(volatile uint32*)(uintptr_t)Flash_IP_TargetAddress = (uint32)value;
            break;
        case 8u:
            *(volatile uint64*)(uintptr_t)Flash_IP_TargetAddress = value;
            break;
        default:
            /* Configuration is guarded in Flash_IP_ProgramStart(). */
            break;
    }
}

static void Flash_IP_Finish(Flash_IP_StatusType result)
{
    const Flash_IP_OperationType completedOperation = Flash_IP_Operation;

    Flash_IP_ClearOperationBits();
    Flash_IP_Lock();

    if ((result == FLASH_IP_OK) &&
        ((completedOperation == FLASH_IP_OP_PROGRAM) ||
         (completedOperation == FLASH_IP_OP_ERASE)))
    {
        Flash_IP_RefreshCaches();
    }

    Flash_IP_Operation = FLASH_IP_OP_NONE;
    Flash_IP_Status = result;
    Flash_IP_TimeoutCounter = 0u;
    Flash_IP_CancelRequested = FALSE;
}

void Flash_IP_Init(void)
{
    uint32 acr = STM32_FLASH->ACR;

    acr &= ~(FLASH_ACR_LATENCY_MASK | FLASH_ACR_PRFTEN |
             STM32_FLASH_ACR_ICEN | STM32_FLASH_ACR_DCEN);
    acr |= ((uint32)FLASH_IP_LATENCY & FLASH_ACR_LATENCY_MASK);

#if (FLASH_IP_PREFETCH_ENABLE == STD_ON)
    acr |= FLASH_ACR_PRFTEN;
#endif
#if (FLASH_IP_ICACHE_ENABLE == STD_ON)
    acr |= STM32_FLASH_ACR_ICEN;
#endif
#if (FLASH_IP_DCACHE_ENABLE == STD_ON)
    acr |= STM32_FLASH_ACR_DCEN;
#endif

    STM32_FLASH->ACR = acr;
    Flash_IP_ClearOperationBits();
    Flash_IP_ClearFlags();
    Flash_IP_Lock();

    Flash_IP_Operation = FLASH_IP_OP_NONE;
    Flash_IP_Status = FLASH_IP_IDLE;
    Flash_IP_TimeoutCounter = 0u;
    Flash_IP_CancelRequested = FALSE;
}

void Flash_IP_DeInit(void)
{
    Flash_IP_Cancel();

    if ((STM32_FLASH->SR & STM32_FLASH_SR_BSY) == 0u)
    {
        Flash_IP_Status = FLASH_IP_IDLE;
    }
}

void Flash_IP_Cancel(void)
{
    if (Flash_IP_Status == FLASH_IP_BUSY)
    {
        Flash_IP_CancelRequested = TRUE;

        if ((STM32_FLASH->SR & STM32_FLASH_SR_BSY) == 0u)
        {
            Flash_IP_ClearFlags();
            Flash_IP_Finish(FLASH_IP_ERROR);
        }
    }
    else
    {
        Flash_IP_ClearOperationBits();
        Flash_IP_Lock();
        Flash_IP_Operation = FLASH_IP_OP_NONE;
    }
}

Flash_IP_StatusType Flash_IP_GetStatus(void)
{
    return Flash_IP_Status;
}

Std_ReturnType Flash_IP_Read(uint32 address, uint8* data, uint32 length)
{
    uint32 index;

    if ((Flash_IP_Status == FLASH_IP_BUSY) ||
        (data == NULL_PTR) ||
        (Flash_IP_IsRangeValid(address, length) == FALSE))
    {
        return E_NOT_OK;
    }

    for (index = 0u; index < length; index++)
    {
        data[index] = *(const volatile uint8*)(uintptr_t)(address + index);
    }

    return E_OK;
}

Std_ReturnType Flash_IP_ProgramStart(uint32 address, const uint8* data, uint32 length)
{
    const uint32 unitSize = Flash_IP_ProgramUnitSize();

    if ((Flash_IP_Status == FLASH_IP_BUSY) ||
        (data == NULL_PTR) ||
        (Flash_IP_IsRangeValid(address, length) == FALSE) ||
        ((unitSize != 1u) && (unitSize != 2u) && (unitSize != 4u) && (unitSize != 8u)) ||
        ((address % unitSize) != 0u) ||
        ((length % unitSize) != 0u))
    {
        return E_NOT_OK;
    }

    Flash_IP_Unlock();
    if ((STM32_FLASH->CR & STM32_FLASH_CR_LOCK) != 0u)
    {
        return E_NOT_OK;
    }

    Flash_IP_ClearFlags();
    Flash_IP_ClearOperationBits();

    Flash_IP_SourcePtr = data;
    Flash_IP_TargetAddress = address;
    Flash_IP_RemainingLength = length;
    Flash_IP_TimeoutCounter = 0u;
    Flash_IP_CancelRequested = FALSE;
    Flash_IP_Operation = FLASH_IP_OP_PROGRAM;
    Flash_IP_Status = FLASH_IP_BUSY;

    Flash_IP_TriggerProgramUnit();
    return E_OK;
}

Std_ReturnType Flash_IP_EraseSectorStart(uint8 sectorNumber)
{
    uint32 cr;

    if ((Flash_IP_Status == FLASH_IP_BUSY) || (sectorNumber >= FLASH_IP_SECTOR_COUNT))
    {
        return E_NOT_OK;
    }

    Flash_IP_Unlock();
    if ((STM32_FLASH->CR & STM32_FLASH_CR_LOCK) != 0u)
    {
        return E_NOT_OK;
    }

    Flash_IP_ClearFlags();

    /* Clear MER explicitly: MER + SER would request a mass erase on STM32F401. */
    cr = STM32_FLASH->CR;
    cr &= ~(STM32_FLASH_CR_PG | STM32_FLASH_CR_SER | STM32_FLASH_CR_MER |
            STM32_FLASH_CR_SNB | STM32_FLASH_CR_EOPIE | STM32_FLASH_CR_ERRIE);
    cr |= ((uint32)sectorNumber << STM32_FLASH_CR_SNB_POS) | STM32_FLASH_CR_SER;
    STM32_FLASH->CR = cr;

    Flash_IP_TimeoutCounter = 0u;
    Flash_IP_CancelRequested = FALSE;
    Flash_IP_Operation = FLASH_IP_OP_ERASE;
    Flash_IP_Status = FLASH_IP_BUSY;

    STM32_FLASH->CR |= STM32_FLASH_CR_STRT;
    return E_OK;
}

void Flash_IP_MainFunction(void)
{
    uint32 sr;

    if (Flash_IP_Status != FLASH_IP_BUSY)
    {
        return;
    }

    sr = STM32_FLASH->SR;

    if ((sr & STM32_FLASH_SR_BSY) != 0u)
    {
        if (Flash_IP_TimeoutCounter < (uint32)FLASH_IP_TIMEOUT_VALUE)
        {
            Flash_IP_TimeoutCounter++;
        }
        else
        {
            /* Hardware cannot be made idle safely while BSY=1. Stop scheduling
             * future program units and finish with ERROR as soon as BSY clears. */
            Flash_IP_CancelRequested = TRUE;
        }
        return;
    }

    if ((sr & FLASH_IP_SR_ERROR_FLAGS) != 0u)
    {
        Flash_IP_ClearFlags();
        Flash_IP_Finish(FLASH_IP_ERROR);
        return;
    }

    if ((sr & STM32_FLASH_SR_EOP) != 0u)
    {
        STM32_FLASH->SR = STM32_FLASH_SR_EOP;
    }

    if (Flash_IP_CancelRequested == TRUE)
    {
        Flash_IP_Finish(FLASH_IP_ERROR);
        return;
    }

    if (Flash_IP_Operation == FLASH_IP_OP_PROGRAM)
    {
        const uint32 unitSize = Flash_IP_ProgramUnitSize();

        Flash_IP_SourcePtr += unitSize;
        Flash_IP_TargetAddress += unitSize;
        Flash_IP_RemainingLength -= unitSize;
        Flash_IP_TimeoutCounter = 0u;

        if (Flash_IP_RemainingLength == 0u)
        {
            Flash_IP_Finish(FLASH_IP_OK);
        }
        else
        {
            Flash_IP_TriggerProgramUnit();
        }
    }
    else if (Flash_IP_Operation == FLASH_IP_OP_ERASE)
    {
        Flash_IP_Finish(FLASH_IP_OK);
    }
    else
    {
        Flash_IP_Finish(FLASH_IP_ERROR);
    }
}

uint8 Flash_IP_GetSectorFromAddress(uint32 address)
{
    uint8 sector;

    for (sector = 0u; sector < FLASH_IP_SECTOR_COUNT; sector++)
    {
        const uint32 start = Flash_IP_SectorTable[sector].StartAddress;
        const uint32 size = Flash_IP_SectorTable[sector].Size;

        if ((address >= start) && ((address - start) < size))
        {
            return sector;
        }
    }

    return 0xFFu;
}

uint32 Flash_IP_GetSectorStartAddress(uint8 sectorNumber)
{
    return (sectorNumber < FLASH_IP_SECTOR_COUNT) ?
            Flash_IP_SectorTable[sectorNumber].StartAddress : 0u;
}

uint32 Flash_IP_GetSectorSize(uint8 sectorNumber)
{
    return (sectorNumber < FLASH_IP_SECTOR_COUNT) ?
            Flash_IP_SectorTable[sectorNumber].Size : 0u;
}
