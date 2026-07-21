/**********************************************************************************************************************
 *  FILE:         Mem.h
 *  MODULE:       Mem (Memory Driver)
 *  DESCRIPTION:  Public API of the AUTOSAR Classic Platform Memory Driver, implemented according to
 *                AUTOSAR_CP_SWS_MemoryDriver, Document ID 1018, AUTOSAR CP R25-11.
 *
 *                Covers:
 *                  8.1 Imported Types
 *                  8.2 Type Definitions
 *                  8.3 Function Definitions (Synchronous / Asynchronous)
 *                  8.5 Scheduled Functions
 *
 *  TRACEABILITY: Requirement IDs (e.g. [SWS_Mem_xxxxx]) are quoted in comments throughout this file and
 *                Mem.c so that the implementation can be traced back to the specification.
 *********************************************************************************************************************/

#ifndef MEM_H
#define MEM_H

/*======================================================================================================================
 *  INCLUDES
 *====================================================================================================================*/
#include "Std_Types.h"              /* Std_ReturnType, Std_VersionInfoType, NULL_PTR, TRUE/FALSE  */
#include "Mem_Cfg.h"                /* Pre-compile configuration (chapter 10)                      */

/*======================================================================================================================
 *  TYPES NORMALLY IMPORTED FROM MemAcc  [SWS_Mem_10020]
 *  This project does not use a separate Memory Access Module (MemAcc), so the two types that
 *  AUTOSAR_CP_SWS_MemoryDriver expects to import from MemAcc_GeneralTypes.h are defined directly here
 *  instead. If a MemAcc module is introduced later, replace this block with
 *  "#include MemAcc_GeneralTypes.h" and remove these definitions to avoid duplicate types.
 *====================================================================================================================*/

/* MemAcc_AddressType - physical address type. Width depends on whether 64-Bit addressing is required,
 * see [SRS_MemHwAb_14046] / [SWS_Mem_00036] / [SWS_Mem_00037]. STM32F401RE only needs 32-Bit addresses. */
#ifndef MEMACC_ADDRESS_64BIT
#define MEMACC_ADDRESS_64BIT STD_OFF
#endif

#if (MEMACC_ADDRESS_64BIT == STD_ON)
typedef uint64 MemAcc_AddressType;
#else
typedef uint32 MemAcc_AddressType;
#endif

/* MemAcc_MemJobResultType - job result values as used by Mem_GetJobResult(), see chapter 7.2.1/7.2.1.1. */
typedef enum
{
    MEM_JOB_OK = 0,             /* [SWS_Mem_00067] job completed successfully                                    */
    MEM_JOB_PENDING,            /* [SWS_Mem_00030] job accepted, still being processed                           */
    MEM_JOB_FAILED,             /* [SWS_Mem_00031] pending job was not able to complete                          */
    MEM_INCONSISTENT,           /* [SWS_Mem_00076] job completed but result did not meet expectation (BlankCheck) */
    MEM_ECC_CORRECTED,          /* [SWS_Mem_00077] job completed, correctable ECC error encountered              */
    MEM_ECC_UNCORRECTED         /* [SWS_Mem_00063][SWS_Mem_00078][SWS_Mem_00061] uncorrectable ECC error         */
} MemAcc_MemJobResultType;

/*======================================================================================================================
 *  MODULE / VENDOR IDENTIFICATION  [SWS_Mem_00074] / SWS_BSW_00101..00103, SWS_BSW_00171
 *====================================================================================================================*/
#define MEM_MODULE_ID                       255u   /* example / vendor specific */
#define MEM_VENDOR_ID                       1u     /* example / vendor specific */

#define MEM_AR_RELEASE_MAJOR_VERSION         25u
#define MEM_AR_RELEASE_MINOR_VERSION         11u
#define MEM_AR_RELEASE_REVISION_VERSION       0u

#define MEM_SW_MAJOR_VERSION                  1u
#define MEM_SW_MINOR_VERSION                  0u
#define MEM_SW_PATCH_VERSION                  0u

/*======================================================================================================================
 *  SERVICE IDs  (Service ID [hex] column of each API definition in chapter 8.3/8.5)
 *====================================================================================================================*/
#define MEM_SID_INIT                        0x01u  /* Mem_Init               [SWS_Mem_10008] */
#define MEM_SID_GET_VERSION_INFO            0x02u  /* Mem_GetVersionInfo     [SWS_Mem_10009] */
#define MEM_SID_MAIN_FUNCTION                0x03u  /* Mem_MainFunction       [SWS_Mem_10010] */
#define MEM_SID_GET_JOB_RESULT              0x04u  /* Mem_GetJobResult       [SWS_Mem_10011] */
#define MEM_SID_READ                        0x05u  /* Mem_Read               [SWS_Mem_10012] */
#define MEM_SID_WRITE                       0x06u  /* Mem_Write              [SWS_Mem_10013] */
#define MEM_SID_ERASE                       0x07u  /* Mem_Erase              [SWS_Mem_10014] */
#define MEM_SID_PROPAGATE_ERROR             0x08u  /* Mem_PropagateError     [SWS_Mem_10015] */
#define MEM_SID_BLANK_CHECK                 0x09u  /* Mem_BlankCheck         [SWS_Mem_10016] */
#define MEM_SID_HW_SPECIFIC_SERVICE         0x0Au  /* Mem_HwSpecificService  [SWS_Mem_10017] */
#define MEM_SID_DEINIT                      0x0Bu  /* Mem_DeInit             [SWS_Mem_10018] */
#define MEM_SID_SUSPEND                     0x0Cu  /* Mem_Suspend            [SWS_Mem_10024] */
#define MEM_SID_RESUME                      0x0Du  /* Mem_Resume             [SWS_Mem_10025] */

/*======================================================================================================================
 *  DEVELOPMENT ERROR CODES  [SWS_Mem_00052]
 *====================================================================================================================*/
#define MEM_E_UNINIT                        0x01u  /* API service called without module initialization      */
#define MEM_E_PARAM_POINTER                 0x02u  /* API service called with NULL pointer                  */
#define MEM_E_PARAM_ADDRESS                 0x03u  /* API service called with an invalid address            */
#define MEM_E_PARAM_LENGTH                  0x04u  /* API service called with an invalid length             */
#define MEM_E_PARAM_INSTANCE_ID             0x05u  /* API service called with an invalid driver instance ID */
#define MEM_E_JOB_PENDING                   0x06u  /* API service called while a job request is pending     */

/*======================================================================================================================
 *  ADDITIONAL RETURN VALUE
 *  Used by several services to indicate that the underlying Mem driver service function is not
 *  implemented for the given memory device technology, see [SWS_Mem_00070].
 *====================================================================================================================*/
#ifndef E_MEM_SERVICE_NOT_AVAIL
#define E_MEM_SERVICE_NOT_AVAIL             ((Std_ReturnType)2U)
#endif

/*======================================================================================================================
 *  8.2  TYPE DEFINITIONS
 *====================================================================================================================*/

/* [SWS_Mem_10002] Mem_AddressType - physical memory device address type, derived from MemAcc_AddressType */
typedef MemAcc_AddressType Mem_AddressType;

/* [SWS_Mem_10000] Mem_ConfigType - postbuild configuration structure type.
 * NOTE: per [SWS_Mem_00087] the configPtr argument passed to Mem_Init() is currently not used and shall
 * be a NULL pointer; the type is nevertheless declared to fulfill SRS_BSW_00414. */
typedef struct
{
    uint8 Mem_ConfigType_Reserved; /* not used - kept only for interface completeness */
} Mem_ConfigType;

/* [SWS_Mem_10003] Mem_DataType - read/write data user buffer type */
typedef uint8 Mem_DataType;

/* [SWS_Mem_10004] Mem_InstanceIdType - Memory driver instance ID type */
typedef uint32 Mem_InstanceIdType;

/* [SWS_Mem_10007] Mem_LengthType - physical memory device length type */
typedef uint32 Mem_LengthType;

/* [SWS_Mem_10026] Mem_HwServiceIdType - Hardware specific service request identifier type */
typedef uint32 Mem_HwServiceIdType;

/*======================================================================================================================
 *  8.3.1  SYNCHRONOUS FUNCTIONS
 *====================================================================================================================*/

/* [SWS_Mem_10008] Mem_Init - Initialization function */
extern void Mem_Init(const Mem_ConfigType* configPtr);

/* [SWS_Mem_10018] Mem_DeInit - De-Initialization function */
extern void Mem_DeInit(void);

#if (MEM_VERSION_INFO_API == STD_ON)
/* [SWS_Mem_10009] Mem_GetVersionInfo - returns version information of the Mem module */
extern void Mem_GetVersionInfo(Std_VersionInfoType* versionInfoPtr);
#endif

/* [SWS_Mem_10011] Mem_GetJobResult - returns result of the most recent job */
extern MemAcc_MemJobResultType Mem_GetJobResult(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10024] Mem_Suspend - suspend active memory operation using hardware mechanism */
extern Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10025] Mem_Resume - resume suspended memory operation using hardware mechanism */
extern Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10015] Mem_PropagateError - report an access error (e.g. ECC) from the system ECC handler */
extern void Mem_PropagateError(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  8.3.2  ASYNCHRONOUS FUNCTIONS
 *====================================================================================================================*/

/* [SWS_Mem_10012] Mem_Read - triggers a read job */
extern Std_ReturnType Mem_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length);

/* [SWS_Mem_10013] Mem_Write - triggers a write job */
extern Std_ReturnType Mem_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length);

/* [SWS_Mem_10014] Mem_Erase - triggers an erase job */
extern Std_ReturnType Mem_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

/* [SWS_Mem_10016] Mem_BlankCheck - triggers a job to check the erased state of a memory area */
extern Std_ReturnType Mem_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

/* [SWS_Mem_10017] Mem_HwSpecificService - dispatches a hardware specific memory driver job */
extern Std_ReturnType Mem_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr);

/*======================================================================================================================
 *  8.5  SCHEDULED FUNCTIONS
 *  [SWS_Mem_10010] Mem_MainFunction - handles the requested jobs and internal management operations.
 *  Available via SchM_Mem.h in a full RTE/SchM integration; declared here as well so the module is
 *  usable stand-alone. Must be called cyclically, see [SWS_Mem_00066]. No fixed cycle time is required.
 *====================================================================================================================*/
extern void Mem_MainFunction(void);

#endif /* MEM_H */