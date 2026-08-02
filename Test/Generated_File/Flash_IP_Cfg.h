/* ========================================================================== */
/* FLASH IP HARDWARE CONFIGURATION (STM32F401RE)                              */
/* ========================================================================== */
#ifndef FLASH_IP_CFG_H
#define FLASH_IP_CFG_H

#include "Std_Types.h"

/* Total Sector Batches configured */
#define FLASH_IP_SECTOR_BATCH_COUNT  (8U)


/* Thong so vat ly cua Flash Sector Batch 0 (SECTOR_0) */
#define FLASH_IP_BATCH_0_START_ADDRESS       (0x08000000U)
#define FLASH_IP_BATCH_0_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_0_ERASE_SECTOR_SIZE   (16384U)
#define FLASH_IP_BATCH_0_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_0_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 1 (SECTOR_1) */
#define FLASH_IP_BATCH_1_START_ADDRESS       (0x08004000U)
#define FLASH_IP_BATCH_1_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_1_ERASE_SECTOR_SIZE   (16384U)
#define FLASH_IP_BATCH_1_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_1_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 2 (SECTOR_2) */
#define FLASH_IP_BATCH_2_START_ADDRESS       (0x08008000U)
#define FLASH_IP_BATCH_2_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_2_ERASE_SECTOR_SIZE   (16384U)
#define FLASH_IP_BATCH_2_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_2_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 3 (SECTOR_3) */
#define FLASH_IP_BATCH_3_START_ADDRESS       (0x0800C000U)
#define FLASH_IP_BATCH_3_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_3_ERASE_SECTOR_SIZE   (16384U)
#define FLASH_IP_BATCH_3_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_3_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 4 (SECTOR_4) */
#define FLASH_IP_BATCH_4_START_ADDRESS       (0x08010000U)
#define FLASH_IP_BATCH_4_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_4_ERASE_SECTOR_SIZE   (65536U)
#define FLASH_IP_BATCH_4_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_4_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 5 (SECTOR_5) */
#define FLASH_IP_BATCH_5_START_ADDRESS       (0x08020000U)
#define FLASH_IP_BATCH_5_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_5_ERASE_SECTOR_SIZE   (131072U)
#define FLASH_IP_BATCH_5_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_5_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 6 (SECTOR_6) */
#define FLASH_IP_BATCH_6_START_ADDRESS       (0x08040000U)
#define FLASH_IP_BATCH_6_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_6_ERASE_SECTOR_SIZE   (131072U)
#define FLASH_IP_BATCH_6_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_6_MIN_READ_SIZE       (1U)

/* Thong so vat ly cua Flash Sector Batch 7 (SECTOR_7) */
#define FLASH_IP_BATCH_7_START_ADDRESS       (0x08060000U)
#define FLASH_IP_BATCH_7_NUMBER_OF_SECTORS   (1U)
#define FLASH_IP_BATCH_7_ERASE_SECTOR_SIZE   (131072U)
#define FLASH_IP_BATCH_7_WRITE_PAGE_SIZE     (4U)
#define FLASH_IP_BATCH_7_MIN_READ_SIZE       (1U)


#endif /* FLASH_IP_CFG_H */