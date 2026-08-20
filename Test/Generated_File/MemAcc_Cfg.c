#include "MemAcc_Cfg.h"

/* Cấu hình một Address Area mẫu trỏ đến Mem_InstanceId 0 */
const MemAcc_AddressAreaConfigType MemAcc_AddressAreaConfig[MEMACC_MAX_ADDRESS_AREAS] = {
    {
        .AddressAreaId = 0,
        .MemInstanceId = 0,
        .LogicalStartAddress = 0x00000000,
        .PhysicalStartAddress = 0x08000000, /* Ví dụ địa chỉ Flash của STM32 */
        .Length = 0x10000, /* Kích thước vùng nhớ: 64KB */
        .BurstSize = 0,    /* Không sử dụng Burst */
        .Priority = 0      /* Ưu tiên thấp nhất */
    }
};
