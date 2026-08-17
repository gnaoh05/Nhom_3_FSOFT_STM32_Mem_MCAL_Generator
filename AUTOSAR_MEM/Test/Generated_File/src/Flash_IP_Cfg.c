#include "Flash_IP_Cfg.h"

const Flash_IP_SectorType Flash_IP_SectorTable[] =
{


    { 0x08000000UL,
      16384u },

    { 0x08004000UL,
      16384u },

    { 0x08008000UL,
      16384u },

    { 0x0800C000UL,
      16384u },



    { 0x08010000UL,
      65536u },



    { 0x08020000UL,
      131072u },

    { 0x08040000UL,
      131072u },

    { 0x08060000UL,
      131072u },


};

#define FLASH_IP_CFG_STATIC_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
FLASH_IP_CFG_STATIC_ASSERT(
        (sizeof(Flash_IP_SectorTable) / sizeof(Flash_IP_SectorTable[0])) == FLASH_IP_SECTOR_COUNT,
        Flash_IP_Cfg_SectorTableSizeCheck);