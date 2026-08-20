#include "Mem_IPW.h"

/**
 * @brief Initializes the low-level Flash hardware driver using provided configuration.
 * 
 * @param[in] ConfigPtr Pointer to the memory driver configuration structure.
 */
void Mem_Ipw_Init(const Mem_ConfigType *ConfigPtr) 
{
    (void)ConfigPtr;

    /* Initialize Flash IP using the configured settings from Flash_IP_Cfg.c */
    Flash_IP_Init(&Flash_IP_Config);
    Flash_IP_Unlock();
}

/**
 * @brief Retrieves the current busy/idle execution status of the underlying Flash hardware.
 * 
 * @param[in] InstanceId Identification of the memory instance.
 * 
 * @return Mem_Ipw_StatusType
 * @retval MEM_IPW_IDLE  Hardware is idle and ready for a new request.
 * @retval MEM_IPW_BUSY  Hardware is currently processing an ongoing job.
 * @retval MEM_IPW_ERROR Hardware reported an error or operation timed out.
 */
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType InstanceId) 
{
    Mem_Ipw_StatusType retVal = MEM_IPW_IDLE;
    Flash_IP_JobResultType hwStatus = FLASH_IP_JOB_OK;

    (void)InstanceId;

    hwStatus = Flash_IP_GetStatus();

    if (hwStatus == FLASH_IP_JOB_BUSY) 
    {
        retVal = MEM_IPW_BUSY;
    }
    else if (hwStatus == FLASH_IP_JOB_OK) 
    {
        retVal = MEM_IPW_IDLE;
    }
    else 
    {
        retVal = MEM_IPW_ERROR;
    }

    return retVal;
}

/**
 * @brief Reads a contiguous block of data directly from Flash memory space.
 * 
 * @param[in]  InstanceId Identification of the memory instance.
 * @param[in]  Address    Physical source address to read from.
 * @param[out] DataPtr    Pointer to destination buffer where data will be stored.
 * @param[in]  Length     Number of bytes to read.
 * 
 * @return Std_ReturnType
 * @retval E_OK     Data read operation completed successfully.
 * @retval E_NOT_OK Read operation failed or invalid parameter provided.
 */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType InstanceId, 
                            Mem_AddressType Address, 
                            Mem_DataType *DataPtr, 
                            Mem_LengthType Length) 
{
    Std_ReturnType retVal = E_NOT_OK;
    Flash_IP_JobResultType hwResult = FLASH_IP_JOB_FAILED;

    (void)InstanceId;

    hwResult = Flash_IP_Read((uint32)Address, (uint8*)DataPtr, (uint32)Length);

    if (hwResult == FLASH_IP_JOB_OK) 
    {
        retVal = E_OK;
    }
    else 
    {
        retVal = E_NOT_OK;
    }

    return retVal;
}

/**
 * @brief Dispatches a write/programming request to the low-level Flash hardware.
 * 
 * @param[in] InstanceId Identification of the memory instance.
 * @param[in] Address    Physical destination address to write to.
 * @param[in] DataPtr    Pointer to the source data buffer.
 * @param[in] Length     Number of bytes to write.
 * 
 * @return Std_ReturnType
 * @retval E_OK     Write operation was triggered successfully.
 * @retval E_NOT_OK Write operation failed or invalid alignment/parameters.
 */
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType InstanceId, 
                             Mem_AddressType Address, 
                             const Mem_DataType *DataPtr, 
                             Mem_LengthType Length) 
{
    Std_ReturnType retVal = E_NOT_OK;
    Flash_IP_JobResultType hwResult = FLASH_IP_JOB_FAILED;

    (void)InstanceId;

    hwResult = Flash_IP_Write((uint32)Address, (const uint8*)DataPtr, (uint32)Length);

    if (hwResult == FLASH_IP_JOB_BUSY) 
    {
        retVal = E_OK;
    }
    else 
    {
        retVal = E_NOT_OK;
    }

    return retVal;
}

/**
 * @brief Converts the target address to sector ID and triggers physical sector erase.
 * 
 * @param[in] InstanceId Identification of the memory instance.
 * @param[in] Address    Physical memory address within the sector to erase.
 * @param[in] Length     Length of the erase region.
 * 
 * @return Std_ReturnType
 * @retval E_OK     Erase operation was triggered successfully.
 * @retval E_NOT_OK Erase trigger failed or address out of range.
 */
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType InstanceId, 
                             Mem_AddressType Address, 
                             Mem_LengthType Length) 
{
    Std_ReturnType retVal = E_NOT_OK;
    Flash_IP_JobResultType hwResult = FLASH_IP_JOB_FAILED;
    uint8 sectorNum = 0xFFU;

    (void)InstanceId;
    (void)Length;

    sectorNum = Flash_IP_GetSectorFromAddress((uint32)Address);

    if (sectorNum != 0xFFU) 
    {
        hwResult = Flash_IP_Erase(sectorNum);

        if (hwResult == FLASH_IP_JOB_BUSY) 
        {
            retVal = E_OK;
        }
        else 
        {
            retVal = E_NOT_OK;
        }
    }
    else 
    {
        retVal = E_NOT_OK;
    }

    return retVal;
}

/**
 * @brief Verifies whether the specified Flash memory area is blank (erased to 0xFF).
 * 
 * @param[in] InstanceId Identification of the memory instance.
 * @param[in] Address    Physical start address to verify.
 * @param[in] Length     Number of bytes to verify.
 * 
 * @return Std_ReturnType
 * @retval E_OK     Memory area is completely blank (all 0xFF).
 * @retval E_NOT_OK Area is not blank, hardware busy, or validation failed.
 */
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType InstanceId, 
                                  Mem_AddressType Address, 
                                  Mem_LengthType Length) 
{
    Std_ReturnType retVal = E_NOT_OK;
    Flash_IP_JobResultType hwResult = FLASH_IP_JOB_FAILED;

    (void)InstanceId;

    hwResult = Flash_IP_BlankCheck((uint32)Address, (uint32)Length);

    if (hwResult == FLASH_IP_JOB_OK) 
    {
        retVal = E_OK;
    }
    else 
    {
        retVal = E_NOT_OK;
    }

    return retVal;
}

/**
 * @brief Cancels the ongoing asynchronous hardware operation.
 * 
 * @param[in] InstanceId Identification of the memory instance.
 */
void Mem_Ipw_Cancel(Mem_InstanceIdType InstanceId) 
{
    (void)InstanceId;

    /* Hardware operation cancellation handled at Mem upper layer state machine */
}
