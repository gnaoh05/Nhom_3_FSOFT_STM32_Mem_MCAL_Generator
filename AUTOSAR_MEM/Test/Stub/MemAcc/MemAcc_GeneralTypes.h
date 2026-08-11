#ifndef MEMACC_GENERALTYPES_H
#define MEMACC_GENERALTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/*
 * AUTOSAR Mem imports these types from MemAcc_GeneralTypes.
 * The integration project does not contain a complete MemAcc module, so this
 * header supplies the interface contract required by the Mem driver.
 */

#ifndef MEMACC_ADDRESS_64BIT
#define MEMACC_ADDRESS_64BIT STD_OFF
#endif

#if (MEMACC_ADDRESS_64BIT == STD_ON)
typedef uint64 MemAcc_AddressType;
#else
typedef uint32 MemAcc_AddressType;
#endif

typedef enum
{
    MEM_JOB_OK = 0,
    MEM_JOB_PENDING,
    MEM_JOB_FAILED,
    MEM_INCONSISTENT,
    MEM_ECC_CORRECTED,
    MEM_ECC_UNCORRECTED
} MemAcc_MemJobResultType;

#ifdef __cplusplus
}
#endif

#endif /* MEMACC_GENERALTYPES_H */
