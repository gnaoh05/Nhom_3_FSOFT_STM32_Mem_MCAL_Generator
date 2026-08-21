#include "Flash_IP.h"
#include "Flash_IP_HwAccess.h"

/* Global driver internal state */
static Flash_IP_StatusType Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;

/* Global timeout counter variable */
static uint32 Flash_IP_TimeoutCounter = 0U; 

/**
 * @brief Initializes the Flash IP driver according to the provided configuration.
 *        If ConfigPtr is NULL_PTR, default Flash_IP_Config from Flash_IP_Cfg.c is used.
 * 
 * @param[in] ConfigPtr Pointer to configuration structure or NULL_PTR.
 */
void Flash_IP_Init(const Flash_IP_ConfigType *ConfigPtr) 
{
    const Flash_IP_ConfigType *activeConfigPtr = ConfigPtr;

    /* Fallback sang cấu hình mặc định từ Flash_IP_Cfg.c nếu truyền NULL_PTR */
    if (activeConfigPtr == NULL_PTR) 
    {
        activeConfigPtr = &Flash_IP_Config;
    }
    else
    {
        /* Use caller-provided configuration */
    }

    Flash_IP_SetLatency(activeConfigPtr->latency);
    Flash_IP_ResetCaches();
    Flash_IP_ConfigureFeatures(activeConfigPtr->prefetchEnable,
                               activeConfigPtr->iCacheEnable,
                               activeConfigPtr->dCacheEnable);
    
    Flash_IP_DriverState = FLASH_IP_INITIALIZED;
    Flash_IP_TimeoutCounter = 0U;
}

/**
 * @brief De-initializes the Flash IP driver and locks Flash registers.
 */
void Flash_IP_DeInit(void) 
{
    Flash_IP_LockHw();
    Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
    Flash_IP_TimeoutCounter = 0U;
}

/**
 * @brief Unlocks Flash control registers for write/erase operations.
 */
void Flash_IP_Unlock(void)
{
    Flash_IP_UnlockHw();
}

/**
 * @brief Locks Flash control registers.
 */
void Flash_IP_Lock(void) 
{
    Flash_IP_LockHw();
}

/**
 * @brief Retrieves the current execution status of Flash hardware operations.
 * 
 * @return Flash_IP_JobResultType Execution result.
 */
Flash_IP_JobResultType Flash_IP_GetStatus(void) 
{
    Flash_IP_JobResultType retVal = FLASH_IP_JOB_OK;
    uint32 srReg = 0U;

    srReg = Flash_IP_GetStatusRegister();

    if (Flash_IP_IsBusy() == TRUE) 
    {
        Flash_IP_TimeoutCounter++;
        if (Flash_IP_TimeoutCounter >= FLASH_IP_TIMEOUT_MAX_TICKS) 
        {
            Flash_IP_EndOperation();
            Flash_IP_TimeoutCounter = 0U;
            retVal = FLASH_IP_JOB_FAILED;
        }
        else
        {
            retVal = FLASH_IP_JOB_BUSY;
        }
    }
    else
    {
        Flash_IP_TimeoutCounter = 0U;

        if ((srReg & FLASH_IP_SR_RDERR) != 0U) 
        {
            Flash_IP_ClearStatusFlags(FLASH_IP_SR_RDERR);
            Flash_IP_EndOperation();
            retVal = FLASH_IP_JOB_FAILED;
        }
        else if ((srReg & FLASH_IP_SR_WRPERR) != 0U) 
        {
            Flash_IP_ClearStatusFlags(FLASH_IP_SR_WRPERR);
            Flash_IP_EndOperation();
            retVal = FLASH_IP_WRITE_PROTECT_ERROR;
        }
        else if ((srReg & (FLASH_IP_SR_PGAERR | FLASH_IP_SR_PGPERR | FLASH_IP_SR_PGSERR | FLASH_IP_SR_OPERR)) != 0U) 
        {
            Flash_IP_ClearStatusFlags(FLASH_IP_SR_PGAERR | FLASH_IP_SR_PGPERR | FLASH_IP_SR_PGSERR | FLASH_IP_SR_OPERR);
            Flash_IP_EndOperation();
            retVal = FLASH_IP_ALIGNMENT_ERROR;
        }
        else if ((srReg & FLASH_IP_SR_EOP) != 0U) 
        {
            Flash_IP_ClearStatusFlags(FLASH_IP_SR_EOP);
            Flash_IP_EndOperation();
            Flash_IP_ResetCaches();
            retVal = FLASH_IP_JOB_OK;
        }
        else
        {
            retVal = FLASH_IP_JOB_OK;
        }
    }

    return retVal;
}

/**
 * @brief Initiates an asynchronous sector erase sequence.
 * 
 * @param[in] sectorNum Index of the sector to erase.
 * 
 * @return Flash_IP_JobResultType Result of erase triggering.
 */
Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum) 
{
    Flash_IP_JobResultType retVal = FLASH_IP_JOB_BUSY;

    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sectorNum > 7U)) 
    {
        retVal = FLASH_IP_JOB_FAILED;
    }
    else if (Flash_IP_IsBusy() == TRUE) 
    {
        retVal = FLASH_IP_JOB_BUSY;
    }
    else
    {
        Flash_IP_ClearErrors();
        Flash_IP_TimeoutCounter = 0U;
        Flash_IP_StartSectorErase(sectorNum);
        retVal = FLASH_IP_JOB_BUSY;
    }

    return retVal;
}

/**
 * @brief Initiates an asynchronous write operation on Flash memory.
 * 
 * @param[in] address   Destination physical memory address.
 * @param[in] sourcePtr Pointer to source data buffer.
 * @param[in] length    Number of bytes to write.
 * 
 * @return Flash_IP_JobResultType Result of write triggering.
 */
/**
 * @brief Initiates an asynchronous write operation on Flash memory.
 * 
 * @param[in] address   Destination physical memory address.
 * @param[in] sourcePtr Pointer to source data buffer.
 * @param[in] length    Number of bytes to write.
 * 
 * @return Flash_IP_JobResultType Result of write triggering.
 */
Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length) 
{
    Flash_IP_JobResultType retVal = FLASH_IP_JOB_BUSY;

    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sourcePtr == NULL_PTR)) 
    {
        retVal = FLASH_IP_JOB_FAILED;
    }
    else if (((address % (uint32)FLASH_IP_WRITE_ALIGNMENT) != 0U) || 
             ((length % (uint32)FLASH_IP_WRITE_ALIGNMENT) != 0U) || 
             (length == 0U)) 
    {
        retVal = FLASH_IP_ALIGNMENT_ERROR;
/*Lỗi 501
Testcase yêu cầu ghi 16 byte
RemainingLength = 16
ChunkSize = 1
processLen = 1
Mem_Ipw_Write(..., Length = 1)
Flash_IP_Write(..., length = 1)
FLASH_IP_WRITE_ALIGNMENT = 4
1 % 4 != 0
FLASH_IP_ALIGNMENT_ERROR
E_NOT_OK
MEM_JOB_FAILED
WRITE_001 / 0x0501 thất bại
-> Đồng bộ MemWritePageSize với Flash_IP_Write Aligment từ 1 ->4 bằng GUI*/


    }
    else if (Flash_IP_IsBusy() == TRUE) 
    {
        retVal = FLASH_IP_JOB_BUSY;
    }
    else
    {
        Flash_IP_ClearErrors();
        Flash_IP_TimeoutCounter = 0U;

        Flash_IP_StartProgramData(address, sourcePtr);
        retVal = FLASH_IP_JOB_BUSY;
    }

    return retVal;
}

/**
 * @brief Reads a memory block from Flash memory.
 * 
 * @param[in]  address   Source physical memory address.
 * @param[out] targetPtr Destination buffer pointer.
 * @param[in]  length    Number of bytes to read.
 * 
 * @return Flash_IP_JobResultType Result of the read operation.
 */
Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length) 
{
    Flash_IP_JobResultType retVal = FLASH_IP_JOB_OK;
    uint32 wordsCount = 0U;
    uint32 remainderBytes = 0U;
    uint32 currentAddr = 0U;
    uint32 idx = 0U;
    uint32 *targetPtr32 = NULL_PTR;
    uint8 *targetPtr8 = NULL_PTR;

    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (targetPtr == NULL_PTR)) 
    {
        retVal = FLASH_IP_JOB_FAILED;
    }
    else if (Flash_IP_IsBusy() == TRUE) 
    {
        retVal = FLASH_IP_JOB_BUSY;
    }
    else
    {
        wordsCount = length / 4U;
        remainderBytes = length % 4U;
        currentAddr = address;
        targetPtr32 = (uint32 *)(void *)targetPtr;

        for (idx = 0U; idx < wordsCount; idx++) 
        {
            targetPtr32[idx] = Flash_IP_Read32(currentAddr);
            currentAddr += 4U;
        }

        if (remainderBytes > 0U) 
        {
            targetPtr8 = &targetPtr[wordsCount * 4U];
            for (idx = 0U; idx < remainderBytes; idx++) 
            {
                targetPtr8[idx] = Flash_IP_Read8(currentAddr + idx);
            }
        }
        else
        {
            /* No remaining bytes */
        }

        retVal = FLASH_IP_JOB_OK;
    }

    return retVal;
}

/**
 * @brief Checks if a memory area is blank (erased to 0xFF).
 * 
 * @param[in] address Starting physical address to verify.
 * @param[in] length  Number of bytes to check.
 * 
 * @return Flash_IP_JobResultType Result of the blank check.
 */
Flash_IP_JobResultType Flash_IP_BlankCheck(uint32 address, uint32 length)
{
    Flash_IP_JobResultType retVal = FLASH_IP_JOB_OK;
    uint32 wordsCount = 0U;
    uint32 remainderBytes = 0U;
    uint32 currentAddr = 0U;
    uint32 idx = 0U;
    boolean isInconsistent = FALSE;

    if (Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) 
    {
        retVal = FLASH_IP_JOB_FAILED;
    }
    else if (Flash_IP_IsBusy() == TRUE) 
    {
        retVal = FLASH_IP_JOB_BUSY;
    }
    else
    {
        wordsCount = length / 4U;
        remainderBytes = length % 4U;
        currentAddr = address;

        for (idx = 0U; (idx < wordsCount) && (isInconsistent == FALSE); idx++) 
        {
            if (Flash_IP_Read32(currentAddr) != 0xFFFFFFFFU) 
            {
                isInconsistent = TRUE;
            }
            else
            {
                currentAddr += 4U;
            }
        }

        if (isInconsistent == FALSE)
        {
            for (idx = 0U; (idx < remainderBytes) && (isInconsistent == FALSE); idx++) 
            {
                if (Flash_IP_Read8(currentAddr + idx) != 0xFFU) 
                {
                    isInconsistent = TRUE;
                }
                else
                {
                    /* Byte is blank */
                }
            }
        }
        else
        {
            /* Already found inconsistent word */
        }

        if (isInconsistent == TRUE)
        {
            retVal = FLASH_IP_INCONSISTENT;
        }
        else
        {
            retVal = FLASH_IP_JOB_OK;
        }
    }

    return retVal;
}

/**
 * @brief Maps a physical Flash memory address to its corresponding sector ID.
 * 
 * @param[in] address Target physical memory address.
 * 
 * @return uint8 Sector index (0 to 7), or 0xFF if out of range.
 */
uint8 Flash_IP_GetSectorFromAddress(uint32 address) 
{
    uint8 sectorId = 0xFFU;

    if ((address >= 0x08000000UL) && (address <= 0x08003FFFUL)) 
    {
        sectorId = 0U;
    }
    else if ((address >= 0x08004000UL) && (address <= 0x08007FFFUL)) 
    {
        sectorId = 1U;
    }
    else if ((address >= 0x08008000UL) && (address <= 0x0800BFFFUL)) 
    {
        sectorId = 2U;
    }
    else if ((address >= 0x0800C000UL) && (address <= 0x0800FFFFUL)) 
    {
        sectorId = 3U;
    }
    else if ((address >= 0x08010000UL) && (address <= 0x0801FFFFUL)) 
    {
        sectorId = 4U;
    }
    else if ((address >= 0x08020000UL) && (address <= 0x0803FFFFUL)) 
    {
        sectorId = 5U;
    }
    else if ((address >= 0x08040000UL) && (address <= 0x0805FFFFUL)) 
    {
        sectorId = 6U;
    }
    else if ((address >= 0x08060000UL) && (address <= 0x0807FFFFUL)) 
    {
        sectorId = 7U;
    }
    else
    {
        sectorId = 0xFFU;
    }

    return sectorId;
}
