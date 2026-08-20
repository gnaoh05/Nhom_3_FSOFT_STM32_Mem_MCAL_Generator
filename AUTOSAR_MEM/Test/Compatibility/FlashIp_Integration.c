#include "Flash_IP.h"
#include "Mem_IPW.h"

const Flash_IP_ConfigType Flash_IP_Config =
{
    .latency        = FLASH_IP_LATENCY,
    .prefetchEnable = (boolean)FLASH_IP_PREFETCH_ENABLE,
    .iCacheEnable   = (boolean)FLASH_IP_ICACHE_ENABLE,
    .dCacheEnable   = (boolean)FLASH_IP_DCACHE_ENABLE
};

void Mem_Ipw_MainFunction(Mem_InstanceIdType InstanceId)
{
    (void)InstanceId;
}
