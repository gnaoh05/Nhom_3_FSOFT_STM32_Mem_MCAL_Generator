#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Std_Types.h"
#include "Mem_Types.h"
#include "Mem_Cfg.h"
#include "Mem_IPW_Types.h"
#include "Flash_IP.h"
#include "Flash_IP_Cfg.h"

/**
 * @brief Initializes the low-level Flash hardware driver using provided configuration.
 * 
 * @param[in] ConfigPtr Pointer to the memory driver configuration structure.
 */
void Mem_Ipw_Init(const Mem_ConfigType *ConfigPtr);

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
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType InstanceId);

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
                            Mem_LengthType Length);

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
                             Mem_LengthType Length);

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
                             Mem_LengthType Length);

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
                                  Mem_LengthType Length);

/**
 * @brief Cancels the ongoing asynchronous hardware operation.
 * 
 * @param[in] InstanceId Identification of the memory instance.
 */
void Mem_Ipw_Cancel(Mem_InstanceIdType InstanceId);

#endif /* MEM_IPW_H */
