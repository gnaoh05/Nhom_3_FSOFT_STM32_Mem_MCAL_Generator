#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include "Std_Types.h"
#include "MemAcc_GeneralTypes.h"

typedef MemAcc_AddressType Mem_AddressType;
typedef uint8              Mem_DataType;
typedef uint32             Mem_InstanceIdType;
typedef uint32             Mem_LengthType;
typedef uint32             Mem_HwServiceIdType;

typedef struct
{
    uint32 MemStartAddress;
    uint32 MemNumberOfSectors;
    uint32 MemEraseSectorSize;
    uint32 MemMinReadSize;
    uint32 MemWritePageSize;
    uint32 MemSpecifiedEraseCycles;
} Mem_SectorBatchConfigType;

typedef struct
{
    uint8                            MemInstanceId;
    uint16                           MemNumberOfBatches;
    const Mem_SectorBatchConfigType* MemSectorBatches;
} Mem_InstanceConfigType;

/* Named struct: Mem_Cfg.h and Mem.c now refer to exactly the same C type. */
typedef struct Mem_ConfigType
{
    const Mem_InstanceConfigType* MemInstances;
} Mem_ConfigType;

#endif /* MEM_TYPES_H */
