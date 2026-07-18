/* ========================================================================== */
/* AUTOSAR MEMORY DRIVER (MEM) CONFIGURATION                                  */
/* ========================================================================== */
#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"

/* Bat/Tat Development Error Detect */
#define MEM_DEV_ERROR_DETECT    (STD_ON)

/* Dinh danh Instance cua Mem Driver */
#define MEM_INSTANCE_ID         (0U)

/* ========================================================================== */
/* CAU HINH THONG SO VAT LY BO NHO FLASH (SECTOR BATCH)                       */
/* ========================================================================== */
#define MEM_START_ADDRESS       (0x08000000U)
#define MEM_NUMBER_OF_SECTORS   (64U)
#define MEM_ERASE_SECTOR_SIZE   (2048U)
#define MEM_WRITE_PAGE_SIZE     (4U)
#define MEM_MIN_READ_SIZE       (1U)

#endif /* MEM_CFG_H */