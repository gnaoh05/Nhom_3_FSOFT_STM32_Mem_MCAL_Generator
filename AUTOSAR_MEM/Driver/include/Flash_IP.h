#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Flash_IP_Types.h"
#include "Flash_IP_Cfg.h"

extern const Flash_IP_ConfigType Flash_IP_Config;

/**
 * @brief Initializes the Flash IP driver according to the provided configuration.
 * 
 * @param[in] ConfigPtr Pointer to configuration structure.
 */
void Flash_IP_Init(const Flash_IP_ConfigType *ConfigPtr);

/**
 * @brief De-initializes the Flash IP driver and locks Flash registers.
 */
void Flash_IP_DeInit(void);

/**
 * @brief Unlocks Flash control registers for write/erase operations.
 */
void Flash_IP_Unlock(void);

/**
 * @brief Locks Flash control registers.
 */
void Flash_IP_Lock(void);

/**
 * @brief Retrieves the current execution status of Flash hardware operations.
 * 
 * @return Flash_IP_JobResultType
 * @retval FLASH_IP_JOB_OK              Operation completed successfully.
 * @retval FLASH_IP_JOB_BUSY            Hardware is currently busy.
 * @retval FLASH_IP_JOB_FAILED          Hardware error or timeout occurred.
 * @retval FLASH_IP_WRITE_PROTECT_ERROR Write attempt on protected sector.
 * @retval FLASH_IP_ALIGNMENT_ERROR     Alignment or programming sequence error.
 */
Flash_IP_JobResultType Flash_IP_GetStatus(void);

/**
 * @brief Initiates an asynchronous sector erase sequence.
 * 
 * @param[in] sectorNum Index of the sector to erase.
 * 
 * @return Flash_IP_JobResultType
 * @retval FLASH_IP_JOB_BUSY   Erase operation started successfully.
 * @retval FLASH_IP_JOB_FAILED Driver uninitialized or invalid sector.
 */
Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum);

/**
 * @brief Initiates an asynchronous write operation on Flash memory.
 * 
 * @param[in] address   Destination physical memory address.
 * @param[in] sourcePtr Pointer to source data buffer.
 * @param[in] length    Number of bytes to write.
 * 
 * @return Flash_IP_JobResultType
 * @retval FLASH_IP_JOB_BUSY         Write operation initiated successfully.
 * @retval FLASH_IP_ALIGNMENT_ERROR  Address or length is not word-aligned.
 * @retval FLASH_IP_JOB_FAILED       Driver uninitialized or invalid input buffer.
 */
Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length);

/**
 * @brief Reads a memory block from Flash memory.
 * 
 * @param[in]  address   Source physical memory address.
 * @param[out] targetPtr Destination buffer pointer.
 * @param[in]  length    Number of bytes to read.
 * 
 * @return Flash_IP_JobResultType
 * @retval FLASH_IP_JOB_OK     Data read successfully.
 * @retval FLASH_IP_JOB_BUSY   Hardware is busy.
 * @retval FLASH_IP_JOB_FAILED Driver uninitialized or invalid pointer.
 */
Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length);

/**
 * @brief Checks if a memory area is blank (erased to 0xFF).
 * 
 * @param[in] address Starting physical address to verify.
 * @param[in] length  Number of bytes to check.
 * 
 * @return Flash_IP_JobResultType
 * @retval FLASH_IP_JOB_OK        Memory area is erased completely.
 * @retval FLASH_IP_INCONSISTENT  Memory area contains non-0xFF values.
 * @retval FLASH_IP_JOB_BUSY      Hardware is busy.
 * @retval FLASH_IP_JOB_FAILED    Driver uninitialized.
 */
Flash_IP_JobResultType Flash_IP_BlankCheck(uint32 address, uint32 length);

/**
 * @brief Maps a physical Flash memory address to its corresponding sector ID.
 * 
 * @param[in] address Target physical memory address.
 * 
 * @return uint8 Sector index (0 to 7), or 0xFF if out of range.
 */
uint8 Flash_IP_GetSectorFromAddress(uint32 address);

#endif /* FLASH_IP_H */
