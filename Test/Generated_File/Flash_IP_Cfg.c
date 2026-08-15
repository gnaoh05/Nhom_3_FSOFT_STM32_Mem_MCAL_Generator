#include "Flash_IP_Cfg.h"

/* Default configuration structure initialized from configuration macros */
const Flash_IP_ConfigType Flash_IP_Config = 
{
    .latency        = (uint8)FLASH_IP_LATENCY,
    .prefetchEnable = (boolean)(FLASH_IP_PREFETCH_ENABLE == STD_ON),
    .iCacheEnable   = (boolean)(FLASH_IP_ICACHE_ENABLE == STD_ON),
    .dCacheEnable   = (boolean)(FLASH_IP_DCACHE_ENABLE == STD_ON)
};