#ifndef MEMACC_CFG_H
#define MEMACC_CFG_H

#include "MemAcc_GeneralTypes.h"
#include "Mem.h" /* For Mem Instance Id definitions if needed */

/* Số lượng Address Area tối đa được định nghĩa */
#define MEMACC_MAX_ADDRESS_AREAS 1u

/* Cấu trúc cấu hình ánh xạ Address Area tới Mem Driver (Sector Batch) */
typedef struct {
    MemAcc_AddressAreaIdType AddressAreaId;
    Mem_InstanceIdType MemInstanceId;
    uint32 LogicalStartAddress;
    uint32 PhysicalStartAddress;
    uint32 Length;
    uint32 BurstSize;      /* (EraseBurstSize / WriteBurstSize) tuỳ cấu hình */
    uint8 Priority;
} MemAcc_AddressAreaConfigType;

/* Mảng cấu hình cho toàn bộ MemAcc */
extern const MemAcc_AddressAreaConfigType MemAcc_AddressAreaConfig[MEMACC_MAX_ADDRESS_AREAS];

#endif /* MEMACC_CFG_H */
