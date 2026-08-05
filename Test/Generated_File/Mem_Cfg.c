#include "Mem_Cfg.h"

/* Dữ liệu giả lập (Mock data) cho 1 Sector để test hàm Validate */
static const Mem_SectorBatchConfigType Mock_FlashBatches[1] = {
    { 
        .MemStartAddress         = 0x08000000u, 
        .MemNumberOfSectors      = 1u, 
        .MemEraseSectorSize      = 16384u, 
        .MemMinReadSize          = 1u, 
        .MemWritePageSize        = 1u, 
        .MemSpecifiedEraseCycles = 10000u 
    }
};

static const Mem_InstanceConfigType Mock_Instances[MEM_MAX_INSTANCES] = {
    {
        .MemInstanceId            = 0u,
        .MemNumberOfBatches       = 1u, 
        .MemSectorBatches         = Mock_FlashBatches
    }
};

/* Định nghĩa biến toàn cục cấu hình (Sẽ được gọi tự động ở Mem_Init) */
const Mem_ConfigType Mem_ConfigData = {
    .MemInstances = Mock_Instances
};