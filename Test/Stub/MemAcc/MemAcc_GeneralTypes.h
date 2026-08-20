#ifndef MEMACC_GENERALTYPES_H
#define MEMACC_GENERALTYPES_H

#include "Std_Types.h"

/* [SWS_MemAcc_10000] */
typedef uint16 MemAcc_AddressAreaIdType;

/* [SWS_MemAcc_10001] */
typedef uint32 MemAcc_AddressType;

/* [SWS_MemAcc_10007] */
typedef uint32 MemAcc_LengthType;

/* [SWS_MemAcc_10004] */
typedef uint8 MemAcc_DataType;

/* [SWS_MemAcc_10010] */
typedef uint32 MemAcc_HwIdType;

/* [SWS_MemAcc_10039] */
typedef enum {
    MEMACC_OK = 0x00,
    MEMACC_FAILED = 0x01,
    MEMACC_INCONSISTENT = 0x02,
    MEMACC_CANCELED = 0x03,
    MEMACC_ECC_UNCORRECTED = 0x04,
    MEMACC_ECC_CORRECTED = 0x05,
    MEMACC_MEM_SERVICE_NOT_AVAIL = 0x06
} MemAcc_JobResultType;

/* [SWS_MemAcc_10009] */
typedef enum {
    MEMACC_JOB_IDLE = 0x00,
    MEMACC_JOB_PENDING = 0x01
} MemAcc_JobStatusType;

/* [SWS_MemAcc_10011] */
typedef enum {
    MEMACC_NO_JOB = 0x00,
    MEMACC_WRITE_JOB = 0x01,
    MEMACC_READ_JOB = 0x02,
    MEMACC_COMPARE_JOB = 0x03,
    MEMACC_ERASE_JOB = 0x04,
    MEMACC_MEMHWSPECIFIC_JOB = 0x05,
    MEMACC_BLANKCHECK_JOB = 0x06,
    MEMACC_REQUESTLOCK_JOB = 0x07
} MemAcc_JobType;

#endif /* MEMACC_GENERALTYPES_H */
