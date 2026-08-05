#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"
<<<<<<< HEAD

/* MemGeneral Configuration */
#define MEM_DEV_ERROR_DETECT    (STD_ON)
#define MEM_INSTANCE_ID         (0U)
#define MEM_INVOCATION          MEM_INVOCATION_DIRECT_STATIC
#define MEM_MAIN_FUNCTION_PERIOD (0.005)

/* ========================================================================== */
/* MEM SECTOR BATCHES CONFIGURATION                                           */
/* ========================================================================== */
#define MEM_SECTOR_BATCH_COUNT  (8U)


/* Sector Batch 0 (SECTOR_0) */
#define MEM_BATCH_0_START_ADDRESS       (0x08000000U)
#define MEM_BATCH_0_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_0_ERASE_SECTOR_SIZE   (16384U)
#define MEM_BATCH_0_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_0_MIN_READ_SIZE       (1U)

/* Sector Batch 1 (SECTOR_1) */
#define MEM_BATCH_1_START_ADDRESS       (0x08004000U)
#define MEM_BATCH_1_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_1_ERASE_SECTOR_SIZE   (16384U)
#define MEM_BATCH_1_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_1_MIN_READ_SIZE       (1U)

/* Sector Batch 2 (SECTOR_2) */
#define MEM_BATCH_2_START_ADDRESS       (0x08008000U)
#define MEM_BATCH_2_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_2_ERASE_SECTOR_SIZE   (16384U)
#define MEM_BATCH_2_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_2_MIN_READ_SIZE       (1U)

/* Sector Batch 3 (SECTOR_3) */
#define MEM_BATCH_3_START_ADDRESS       (0x0800C000U)
#define MEM_BATCH_3_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_3_ERASE_SECTOR_SIZE   (16384U)
#define MEM_BATCH_3_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_3_MIN_READ_SIZE       (1U)

/* Sector Batch 4 (SECTOR_4) */
#define MEM_BATCH_4_START_ADDRESS       (0x08010000U)
#define MEM_BATCH_4_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_4_ERASE_SECTOR_SIZE   (65536U)
#define MEM_BATCH_4_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_4_MIN_READ_SIZE       (1U)

/* Sector Batch 5 (SECTOR_5) */
#define MEM_BATCH_5_START_ADDRESS       (0x08020000U)
#define MEM_BATCH_5_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_5_ERASE_SECTOR_SIZE   (131072U)
#define MEM_BATCH_5_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_5_MIN_READ_SIZE       (1U)

/* Sector Batch 6 (SECTOR_6) */
#define MEM_BATCH_6_START_ADDRESS       (0x08040000U)
#define MEM_BATCH_6_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_6_ERASE_SECTOR_SIZE   (131072U)
#define MEM_BATCH_6_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_6_MIN_READ_SIZE       (1U)

/* Sector Batch 7 (SECTOR_7) */
#define MEM_BATCH_7_START_ADDRESS       (0x08060000U)
#define MEM_BATCH_7_NUMBER_OF_SECTORS   (1U)
#define MEM_BATCH_7_ERASE_SECTOR_SIZE   (131072U)
#define MEM_BATCH_7_WRITE_PAGE_SIZE     (4U)
#define MEM_BATCH_7_MIN_READ_SIZE       (1U)


/* API Configuration */
#define MEM_READ_API        (STD_ON)
#define MEM_WRITE_API       (STD_ON)
#define MEM_ERASE_API       (STD_ON)
#define MEM_GET_STATUS_API  (STD_ON)

#endif /* MEM_CFG_H */