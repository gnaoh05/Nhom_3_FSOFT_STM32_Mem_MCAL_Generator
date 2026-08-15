#ifndef FLASH_IP_TYPES_H
#define FLASH_IP_TYPES_H

#include "Std_Types.h"

/* Type definition for hardware base address */
#define FLASH_IP_BASE_ADDR           (0x40023C00UL)

/* Maximum timeout tick count before aborting hardware operations */
#define FLASH_IP_TIMEOUT_MAX_TICKS   (1000000UL)

/* Flash Register Map structure definition using standard AUTOSAR types */
typedef struct 
{
    __IO uint32 ACR;      /* Access Control Register */
    __IO uint32 KEYR;     /* Key Register */
    __IO uint32 OPTKEYR;  /* Option Key Register */
    __IO uint32 SR;       /* Status Register */
    __IO uint32 CR;       /* Control Register */
    __IO uint32 OPTCR;    /* Option Control Register */
} Flash_IP_RegisterType;

/* Pointer macro to access Flash Peripheral Registers */
#define FLASH_IP_REG                 ((Flash_IP_RegisterType*)FLASH_IP_BASE_ADDR)

/* Flash Control Register (CR) Bit Definitions */
#define FLASH_IP_CR_PG               (1U << 0)
#define FLASH_IP_CR_SER              (1U << 1)
#define FLASH_IP_CR_MER              (1U << 2)
#define FLASH_IP_CR_SNB_POS          (3U)
#define FLASH_IP_CR_SNB_MASK         (0xFU << FLASH_IP_CR_SNB_POS)
#define FLASH_IP_CR_PSIZE_POS        (8U)
#define FLASH_IP_CR_PSIZE_MASK       (3U << FLASH_IP_CR_PSIZE_POS)
#define FLASH_IP_CR_PSIZE_X32        (2U << FLASH_IP_CR_PSIZE_POS)
#define FLASH_IP_CR_STRT             (1U << 16)
#define FLASH_IP_CR_LOCK             (1U << 31)

/* Flash Status Register (SR) Bit Definitions */
#define FLASH_IP_SR_EOP              (1U << 0)
#define FLASH_IP_SR_OPERR            (1U << 1)
#define FLASH_IP_SR_WRPERR           (1U << 4)  
#define FLASH_IP_SR_PGAERR           (1U << 5)  
#define FLASH_IP_SR_PGPERR           (1U << 6)  
#define FLASH_IP_SR_PGSERR           (1U << 7)  
#define FLASH_IP_SR_RDERR            (1U << 8)  
#define FLASH_IP_SR_BSY              (1U << 16) 

#define FLASH_IP_SR_ALL_ERRORS       (FLASH_IP_SR_EOP | FLASH_IP_SR_OPERR | FLASH_IP_SR_WRPERR | \
                                      FLASH_IP_SR_PGAERR | FLASH_IP_SR_PGPERR | FLASH_IP_SR_PGSERR | \
                                      FLASH_IP_SR_RDERR)

/* Flash Unlock Keys */
#define FLASH_IP_KEY1                (0x45670123UL)
#define FLASH_IP_KEY2                (0xCDEF89ABUL)

/* Driver execution status */
typedef enum 
{
    FLASH_IP_UNINITIALIZED = 0x00U,
    FLASH_IP_INITIALIZED   = 0x01U
} Flash_IP_StatusType;

/* Flash Job execution result */
typedef enum 
{
    FLASH_IP_JOB_OK = 0U,
    FLASH_IP_JOB_BUSY,
    FLASH_IP_JOB_FAILED,
    FLASH_IP_INCONSISTENT,
    FLASH_IP_WRITE_PROTECT_ERROR,
    FLASH_IP_ALIGNMENT_ERROR
} Flash_IP_JobResultType;

/* Configuration structure definition for Flash IP */
typedef struct
{
    uint8   latency;          /* Latency wait states */
    boolean prefetchEnable;   /* Enable Prefetch buffer */
    boolean iCacheEnable;     /* Enable Instruction cache */
    boolean dCacheEnable;     /* Enable Data cache */
} Flash_IP_ConfigType;

#endif /* FLASH_IP_TYPES_H */