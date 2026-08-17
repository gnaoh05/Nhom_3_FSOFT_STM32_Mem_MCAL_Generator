#ifndef FLASH_IP_CFG_H
#define FLASH_IP_CFG_H

#include "Std_Types.h"

#define FLASH_IP_PSIZE_X8             0u
#define FLASH_IP_PSIZE_X16            1u
#define FLASH_IP_PSIZE_X32            2u
#define FLASH_IP_PSIZE_X64            3u

#define FLASH_IP_LATENCY              (2u)
#define FLASH_IP_PREFETCH_ENABLE      (STD_ON)
#define FLASH_IP_ICACHE_ENABLE        (STD_ON)
#define FLASH_IP_DCACHE_ENABLE        (STD_ON)
#define FLASH_IP_TIMEOUT_VALUE        (0x000FFFFFUL)
#define FLASH_IP_PSIZE                FLASH_IP_PSIZE_X32
#define FLASH_IP_WRITE_ALIGNMENT      (4u)
#define FLASH_IP_REGISTER_BASE_ADDRESS (0x40023C00UL)

#define FLASH_IP_BASE_ADDRESS         0x08000000UL
#define FLASH_IP_TOTAL_SIZE           0x00080000UL
#define FLASH_IP_SECTOR_COUNT         (8u)
#define FLASH_IP_TOTAL_SECTORS        FLASH_IP_SECTOR_COUNT
#define FLASH_IP_ERASED_VALUE         0xFFu

typedef struct
{
    uint32 StartAddress;
    uint32 Size;
} Flash_IP_SectorType;

extern const Flash_IP_SectorType Flash_IP_SectorTable[];

#endif /* FLASH_IP_CFG_H */