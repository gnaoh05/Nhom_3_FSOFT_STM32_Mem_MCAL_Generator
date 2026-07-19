/* ========================================================================== */
/* FLASH IP HARDWARE CONFIGURATION (STM32F401RE)                              */
/* ========================================================================== */
#ifndef FLASH_IP_CFG_H
#define FLASH_IP_CFG_H

#include "Std_Types.h"

/* Thong so vat ly cua Flash STM32F401RE */
#define FLASH_IP_START_ADDRESS       (0x08000000U)
#define FLASH_IP_NUMBER_OF_SECTORS   (32U)
#define FLASH_IP_ERASE_SECTOR_SIZE   (2048U)
#define FLASH_IP_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_MIN_READ_SIZE       (1U)

#endif /* FLASH_IP_CFG_H */