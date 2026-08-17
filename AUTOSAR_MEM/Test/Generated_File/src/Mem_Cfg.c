#include "Mem_Cfg.h"

const uint32 Mem_ErasedValue = MEM_ERASED_VALUE;

static const Mem_SectorBatchConfigType Mem_InternalFlashBatches[] =
{

    { 0x08000000UL, 4u,
      16384u, 1u,
      4u, 10000u },

    { 0x08010000UL, 1u,
      65536u, 1u,
      4u, 10000u },

    { 0x08020000UL, 3u,
      131072u, 1u,
      4u, 10000u }

};

static const Mem_InstanceConfigType Mem_Instances[MEM_INSTANCE_COUNT] =
{
    {
        MemConf_MemInstance_MemInstance_0,
        (uint16)(sizeof(Mem_InternalFlashBatches) / sizeof(Mem_InternalFlashBatches[0])),
        Mem_InternalFlashBatches
    }
};

const Mem_ConfigType Mem_ConfigData = { Mem_Instances };