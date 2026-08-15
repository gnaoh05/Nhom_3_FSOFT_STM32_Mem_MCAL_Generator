#ifndef FLASH_IP_HWACCESS_H
#define FLASH_IP_HWACCESS_H

#include "Flash_IP_Types.h"

/**
 * @brief Sets the Flash memory access latency (Wait States).
 * 
 * @param[in] latency Number of wait states.
 */
static inline void Flash_IP_SetLatency(uint8 latency)
{
    uint32 acrValue = FLASH_IP_REG->ACR;

    acrValue &= ~0x07U;
    acrValue |= ((uint32)latency & 0x07U);
    FLASH_IP_REG->ACR = acrValue;
}

/**
 * @brief Resets Instruction Cache and Data Cache.
 */
static inline void Flash_IP_ResetCaches(void)
{
    FLASH_IP_REG->ACR |= (1U << 11) | (1U << 12);
    FLASH_IP_REG->ACR &= ~((1U << 11) | (1U << 12));
}

/**
 * @brief Configures Prefetch Buffer, Instruction Cache, and Data Cache.
 * 
 * @param[in] prefetch Prefetch buffer enable flag.
 * @param[in] iCache   Instruction cache enable flag.
 * @param[in] dCache   Data cache enable flag.
 */
static inline void Flash_IP_ConfigureFeatures(boolean prefetch, boolean iCache, boolean dCache)
{
    uint32 acrValue = FLASH_IP_REG->ACR;

    if (prefetch != FALSE)
    {
        acrValue |= (1U << 8);
    }
    else
    {
        acrValue &= ~(1U << 8);
    }

    if (iCache != FALSE)
    {
        acrValue |= (1U << 9);
    }
    else
    {
        acrValue &= ~(1U << 9);
    }

    if (dCache != FALSE)
    {
        acrValue |= (1U << 10);
    }
    else
    {
        acrValue &= ~(1U << 10);
    }

    FLASH_IP_REG->ACR = acrValue;
}

/**
 * @brief Unlocks the Flash control register interface.
 */
static inline void Flash_IP_UnlockHw(void)
{
    if ((FLASH_IP_REG->CR & FLASH_IP_CR_LOCK) != 0U)
    {
        FLASH_IP_REG->KEYR = FLASH_IP_KEY1;
        FLASH_IP_REG->KEYR = FLASH_IP_KEY2;
    }
    else
    {
        /* Flash registers already unlocked */
    }
}

/**
 * @brief Locks the Flash control register interface.
 */
static inline void Flash_IP_LockHw(void)
{
    FLASH_IP_REG->CR |= FLASH_IP_CR_LOCK;
}

/**
 * @brief Checks if Flash hardware is busy performing an operation.
 * 
 * @return boolean
 * @retval TRUE  Hardware is currently busy.
 * @retval FALSE Hardware is idle.
 */
static inline boolean Flash_IP_IsBusy(void)
{
    boolean retVal = FALSE;

    if ((FLASH_IP_REG->SR & FLASH_IP_SR_BSY) != 0U)
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return retVal;
}

/**
 * @brief Clears all pending error flags in the Status Register.
 */
static inline void Flash_IP_ClearErrors(void)
{
    FLASH_IP_REG->SR = FLASH_IP_SR_ALL_ERRORS;
}

/**
 * @brief Reads the current raw value of the Status Register.
 * 
 * @return uint32 Raw value of the Flash SR register.
 */
static inline uint32 Flash_IP_GetStatusRegister(void)
{
    uint32 srValue = 0U;

    srValue = FLASH_IP_REG->SR;

    return srValue;
}

/**
 * @brief Clears specific status flags in the Status Register.
 * 
 * @param[in] flags Bitmask of flags to be cleared.
 */
static inline void Flash_IP_ClearStatusFlags(uint32 flags)
{
    FLASH_IP_REG->SR = flags;
}

/**
 * @brief Initiates an erase operation on the selected Flash sector.
 * 
 * @param[in] sectorNum Sector index to be erased.
 */
static inline void Flash_IP_StartSectorErase(uint8 sectorNum)
{
    uint32 crValue = FLASH_IP_REG->CR;

    crValue &= ~(FLASH_IP_CR_SNB_MASK | FLASH_IP_CR_PG);
    crValue |= ((uint32)sectorNum << FLASH_IP_CR_SNB_POS) | FLASH_IP_CR_SER;
    FLASH_IP_REG->CR = crValue;
    FLASH_IP_REG->CR |= FLASH_IP_CR_STRT;
}

/**
 * @brief Programs a 32-bit word into the specified Flash memory address.
 * 
 * @param[in] address Memory target address.
 * @param[in] data    32-bit data to program.
 */
static inline void Flash_IP_StartProgramWord(uint32 address, uint32 data)
{
    uint32 crValue = FLASH_IP_REG->CR;

    crValue &= ~(FLASH_IP_CR_PSIZE_MASK | FLASH_IP_CR_SER);
    crValue |= FLASH_IP_CR_PSIZE_X32 | FLASH_IP_CR_PG;
    FLASH_IP_REG->CR = crValue;

    *(__IO uint32*)address = data;
}

/**
 * @brief Resets Flash control bits to return to standard read mode.
 */
static inline void Flash_IP_EndOperation(void)
{
    FLASH_IP_REG->CR &= ~(FLASH_IP_CR_PG | FLASH_IP_CR_SER | FLASH_IP_CR_STRT);
}

/**
 * @brief Reads an 8-bit byte from physical Flash address.
 * 
 * @param[in] address Memory source address.
 * @return uint8 Byte value read from Flash.
 */
static inline uint8 Flash_IP_Read8(uint32 address)
{
    uint8 data = 0U;

    data = *(__IO uint8*)address;

    return data;
}

/**
 * @brief Reads a 32-bit word from physical Flash address.
 * 
 * @param[in] address Memory source address.
 * @return uint32 32-bit word value read from Flash.
 */
static inline uint32 Flash_IP_Read32(uint32 address)
{
    uint32 data = 0U;

    data = *(__IO uint32*)address;

    return data;
}

#endif /* FLASH_IP_HWACCESS_H */