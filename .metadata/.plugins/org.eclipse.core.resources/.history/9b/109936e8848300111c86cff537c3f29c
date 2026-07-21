/**********************************************************************************************************************
 *  FILE:         Mem.c
 *  MODULE:       Mem (Memory Driver)
 *  DESCRIPTION:  Implementation of the AUTOSAR Classic Platform Memory Driver, according to
 *                AUTOSAR_CP_SWS_MemoryDriver, Document ID 1018, AUTOSAR CP R25-11.
 *
 *  REVISION NOTE (v2): Reworked so that asynchronous services (Read/Write/Erase/BlankCheck/
 *                HwSpecificService) only validate parameters and record the request synchronously;
 *                the actual hardware trigger now happens inside Mem_MainFunction(), per [SWS_Mem_00066]
 *                ("All job requests triggered by asynchronous Mem driver services shall be executed
 *                within the Mem_MainFunction"). v1 triggered the hardware operation directly from the
 *                API call, which under a strict reading of 00066 was a deviation - see project
 *                traceability review.
 *
 *  TRACEABILITY: Every DET/behavioral check below is commented with the exact [SWS_Mem_xxxxx] requirement
 *                it implements. Where this driver adds a check that is NOT backed by a currently active
 *                numbered requirement in this document revision (R25-11), that is stated explicitly
 *                instead of citing a fabricated ID - see e.g. the MEM_E_UNINIT notes below.
 *********************************************************************************************************************/

#include "Mem.h"
#include "Mem_IPW.h"

#if (MEM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*======================================================================================================================
 *  MODULE STATE
 *====================================================================================================================*/
typedef enum
{
    MEM_UNINIT = 0,
    MEM_INIT
} Mem_StateType;

typedef struct
{
    MemAcc_MemJobResultType JobResult;      /* result of the last processed job, see 7.2.1.1        */
    boolean                 JobPending;     /* TRUE from job acceptance until job completion         */
    boolean                 SuspendActive;  /* TRUE while the current job is suspended                */
} Mem_InstanceRuntimeType;

/* Which asynchronous service is queued for the instance, and its parameters - needed because, per
 * [SWS_Mem_00066], the actual Mem_Ipw_xxx() trigger call must happen inside Mem_MainFunction(), not in
 * the API call itself. Only one entry per instance is ever in use, consistent with [SWS_Mem_00057]. */
typedef enum
{
    MEM_OP_NONE = 0,
    MEM_OP_READ,
    MEM_OP_WRITE,
    MEM_OP_ERASE,
    MEM_OP_BLANK_CHECK,
    MEM_OP_HW_SPECIFIC
} Mem_OperationType;

typedef struct
{
    Mem_OperationType    Operation;
    Mem_AddressType       Address;
    Mem_LengthType        Length;
    Mem_DataType*         DestPtr;     /* Mem_Read() destination buffer                      */
    const Mem_DataType*   SrcPtr;      /* Mem_Write() source buffer                          */
    Mem_HwServiceIdType   HwServiceId; /* Mem_HwSpecificService() service selector           */
    Mem_DataType*         HwDataPtr;   /* Mem_HwSpecificService() data buffer                */
    Mem_LengthType*       HwLengthPtr; /* Mem_HwSpecificService() length pointer             */
    boolean                Started;    /* TRUE once Mem_Ipw_xxx() has actually been called   */
} Mem_PendingRequestType;

static Mem_StateType           Mem_ModuleState = MEM_UNINIT;
static Mem_InstanceRuntimeType Mem_InstanceRuntime[MEM_INSTANCE_COUNT];
static Mem_PendingRequestType  Mem_PendingRequest[MEM_INSTANCE_COUNT];

/*======================================================================================================================
 *  DET REPORTING MACRO
 *====================================================================================================================*/
#if (MEM_DEV_ERROR_DETECT == STD_ON)
#define MEM_DET_REPORT_ERROR(ApiId, ErrorId) \
            ((void)Det_ReportError(MEM_MODULE_ID, MEM_INDEX, (ApiId), (ErrorId)))
#else
#define MEM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*======================================================================================================================
 *  LOCAL PARAMETER VALIDATION HELPERS
 *  Each helper performs the check mandated by the referenced [SWS_Mem_xxxxx] requirement and, if
 *  MEM_DEV_ERROR_DETECT == STD_ON, reports the corresponding development error via Det_ReportError().
 *  Returns TRUE if the checked parameter is valid, FALSE otherwise.
 *====================================================================================================================*/

/* NOTE ON MEM_E_UNINIT: [SWS_Mem_00052] lists "API service called without module initialization" as a
 * valid development error (MEM_E_UNINIT). However, this document revision (R25-11) does not carry a
 * currently active, per-function numbered requirement mandating this specific check for Mem_Read/Write/
 * Erase/BlankCheck/HwSpecificService/Suspend/Resume (candidate IDs such as [SWS_Mem_00003]/[SWS_Mem_00008]
 * were DELETED in R23-11, see Appendix A.3.3). The check below is retained as defensive good practice,
 * consistent with the still-active error table [SWS_Mem_00052] and with SRS_BSW general conventions, but
 * is NOT traceable to one specific still-active numbered SWS_Mem requirement. */
static boolean Mem_CheckModuleInit(uint8 apiId)
{
    boolean valid = (boolean)(Mem_ModuleState == MEM_INIT);

    if (valid == FALSE)
    {
        MEM_DET_REPORT_ERROR(apiId, MEM_E_UNINIT); /* [SWS_Mem_00052] error table; see note above */
    }

    return valid;
}

static boolean Mem_CheckInstanceId(Mem_InstanceIdType instanceId, uint8 apiId)
{
    boolean valid = (boolean)(instanceId < (Mem_InstanceIdType)MEM_INSTANCE_COUNT);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00004](Read) [SWS_Mem_00009](Write) [SWS_Mem_00015](Erase) [SWS_Mem_00022](BlankCheck)
         * [SWS_Mem_00026](HwSpecificService) [SWS_Mem_00090](GetJobResult) [SWS_Mem_00091](Suspend)
         * [SWS_Mem_00092](Resume) [SWS_Mem_00020](PropagateError) */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_PARAM_INSTANCE_ID);
    }

    return valid;
}

static boolean Mem_CheckPointer(const void* ptr, uint8 apiId)
{
    boolean valid = (boolean)(ptr != NULL_PTR);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00005](Read destinationDataPtr) [SWS_Mem_00010](Write sourceDataPtr)
         * [SWS_Mem_00027](HwSpecificService dataPtr/lengthPtr) [SWS_Mem_00087](Init configPtr - inverted
         * polarity, see Mem_Init()) [SWS_Mem_00002](GetVersionInfo versionInfoPtr) */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_PARAM_POINTER);
    }

    return valid;
}

static boolean Mem_CheckAddress(Mem_InstanceIdType instanceId, Mem_AddressType address, uint8 apiId)
{
    boolean valid = Mem_Ipw_IsAddressValid(instanceId, address);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00006](Read) [SWS_Mem_00011](Write) [SWS_Mem_00016](Erase) [SWS_Mem_00023](BlankCheck) */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_PARAM_ADDRESS);
    }

    return valid;
}

static boolean Mem_CheckLength(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length,
        uint8               apiId)
{
    boolean valid = Mem_Ipw_IsLengthValid(instanceId, address, length);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00072](Read) [SWS_Mem_00012](Write) [SWS_Mem_00017](Erase) [SWS_Mem_00024](BlankCheck) */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_PARAM_LENGTH);
    }

    return valid;
}

/* [SWS_Mem_00035]: "The Mem driver shall not perform any sort of address or length alignment in case
 * physical segmentation needs to be considered" - i.e. Erase requests that do not exactly match one
 * physical sector (see Flash_IP_Cfg.c) must be REJECTED, never silently rounded/padded. This document
 * does not define a dedicated error code for "not aligned to physical segmentation", so the rejection is
 * reported via the same MEM_E_PARAM_ADDRESS code used for the generic address check ([SWS_Mem_00016]),
 * since a non-sector-aligned address is a form of invalid address for an Erase request specifically. */
static boolean Mem_CheckEraseAlignment(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length,
        uint8               apiId)
{
    boolean valid = Mem_Ipw_IsEraseAligned(instanceId, address, length);

    if (valid == FALSE)
    {
        MEM_DET_REPORT_ERROR(apiId, MEM_E_PARAM_ADDRESS); /* [SWS_Mem_00016] / [SWS_Mem_00035] */
    }

    return valid;
}

static boolean Mem_CheckJobPending(Mem_InstanceIdType instanceId, uint8 apiId)
{
    /* [SRS_MemHwAb_14050] / [SWS_Mem_00057] only one job at a time per instance */
    boolean valid = (boolean)(Mem_InstanceRuntime[instanceId].JobPending == FALSE);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00007](Read) [SWS_Mem_00013](Write) [SWS_Mem_00018](Erase) [SWS_Mem_00025](BlankCheck).
         * No numbered SWS_Mem requirement exists for HwSpecificService specifically; applied there too
         * for consistency with [SRS_MemHwAb_14050] (one job per instance, without exception per service). */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_JOB_PENDING);
    }

    return valid;
}

/*======================================================================================================================
 *  8.3.1  SYNCHRONOUS FUNCTIONS
 *====================================================================================================================*/

/* [SWS_Mem_10008] */
void Mem_Init(const Mem_ConfigType* configPtr)
{
    Mem_InstanceIdType i;

    /* [SWS_Mem_00087] configPtr is currently not used and shall be a NULL pointer */
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (configPtr != NULL_PTR)
    {
        MEM_DET_REPORT_ERROR(MEM_SID_INIT, MEM_E_PARAM_POINTER);
    }
    else
#endif
    {
        (void)configPtr;

        for (i = 0u; i < (Mem_InstanceIdType)MEM_INSTANCE_COUNT; i++)
        {
            Mem_InstanceRuntime[i].JobResult      = MEM_JOB_OK; /* [SWS_Mem_00001] */
            Mem_InstanceRuntime[i].JobPending     = FALSE;
            Mem_InstanceRuntime[i].SuspendActive  = FALSE;

            Mem_PendingRequest[i].Operation = MEM_OP_NONE;
            Mem_PendingRequest[i].Started   = FALSE;

            Mem_Ipw_Init(i);
        }

        Mem_ModuleState = MEM_INIT;
    }
}

/* [SWS_Mem_10018] */
void Mem_DeInit(void)
{
    Mem_InstanceIdType i;

    for (i = 0u; i < (Mem_InstanceIdType)MEM_INSTANCE_COUNT; i++)
    {
        /* [SWS_Mem_00079] cancel any ongoing operation in the hardware (best-effort - see Flash_IP.c for
         * the single-Flash-bank limitation on truly aborting an in-flight BSY operation) */
        Mem_Ipw_DeInit(i);

        Mem_InstanceRuntime[i].JobPending    = FALSE;
        Mem_InstanceRuntime[i].SuspendActive = FALSE;

        Mem_PendingRequest[i].Operation = MEM_OP_NONE;
        Mem_PendingRequest[i].Started   = FALSE;
    }

    Mem_ModuleState = MEM_UNINIT;
}

#if (MEM_VERSION_INFO_API == STD_ON)
/* [SWS_Mem_10009] */
void Mem_GetVersionInfo(Std_VersionInfoType* versionInfoPtr)
{
    if (Mem_CheckPointer(versionInfoPtr, MEM_SID_GET_VERSION_INFO) == TRUE) /* [SWS_Mem_00002] */
    {
        versionInfoPtr->vendorID         = MEM_VENDOR_ID;
        versionInfoPtr->moduleID         = MEM_MODULE_ID;
        versionInfoPtr->sw_major_version = MEM_SW_MAJOR_VERSION;
        versionInfoPtr->sw_minor_version = MEM_SW_MINOR_VERSION;
        versionInfoPtr->sw_patch_version = MEM_SW_PATCH_VERSION;
    }
}
#endif

/* [SWS_Mem_10011] */
MemAcc_MemJobResultType Mem_GetJobResult(Mem_InstanceIdType instanceId)
{
    MemAcc_MemJobResultType result = MEM_JOB_FAILED;

    if (Mem_CheckInstanceId(instanceId, MEM_SID_GET_JOB_RESULT) == TRUE) /* [SWS_Mem_00090] */
    {
        result = Mem_InstanceRuntime[instanceId].JobResult; /* [SWS_Mem_00029] */
    }

    return result;
}

/* [SWS_Mem_10024] */
Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_SUSPEND) == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_SUSPEND) == TRUE)) /* [SWS_Mem_00091] */
    {
        if (Mem_InstanceRuntime[instanceId].SuspendActive == TRUE)
        {
            retVal = E_NOT_OK; /* [SWS_Mem_00083] already suspended, reject without further action */
        }
        else
        {
            retVal = Mem_Ipw_Suspend(instanceId); /* [SWS_Mem_00080][SWS_Mem_00082] */

            if (retVal == E_OK)
            {
                Mem_InstanceRuntime[instanceId].SuspendActive = TRUE;
            }
        }
    }

    return retVal;
}

/* [SWS_Mem_10025] */
Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_RESUME) == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_RESUME) == TRUE)) /* [SWS_Mem_00092] */
    {
        if (Mem_InstanceRuntime[instanceId].SuspendActive == FALSE)
        {
            retVal = E_NOT_OK; /* [SWS_Mem_00084] no suspend pending, reject without further action */
        }
        else
        {
            retVal = Mem_Ipw_Resume(instanceId); /* [SWS_Mem_00081][SWS_Mem_00082] */

            if (retVal == E_OK)
            {
                Mem_InstanceRuntime[instanceId].SuspendActive = FALSE;
            }
        }
    }

    return retVal;
}

/* [SWS_Mem_10015] */
void Mem_PropagateError(Mem_InstanceIdType instanceId)
{
    if (Mem_CheckInstanceId(instanceId, MEM_SID_PROPAGATE_ERROR) == TRUE) /* [SWS_Mem_00020] */
    {
        /* [SWS_Mem_00061] set job result to MEM_ECC_UNCORRECTED and cancel current job processing */
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_ECC_UNCORRECTED;
        Mem_InstanceRuntime[instanceId].JobPending = FALSE;

        Mem_PendingRequest[instanceId].Operation = MEM_OP_NONE;
        Mem_PendingRequest[instanceId].Started   = FALSE;
    }
}

/*======================================================================================================================
 *  8.3.2  ASYNCHRONOUS FUNCTIONS
 *  Per [SWS_Mem_00066], these functions only validate parameters and record the request; the actual
 *  Mem_Ipw_xxx() hardware trigger is performed later, inside Mem_MainFunction() below.
 *====================================================================================================================*/

/* [SWS_Mem_10012] */
Std_ReturnType Mem_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_READ)                                 == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_READ)                    == TRUE) && /* 00004 */
        (Mem_CheckPointer(destinationDataPtr, MEM_SID_READ)               == TRUE) && /* 00005 */
        (Mem_CheckAddress(instanceId, sourceAddress, MEM_SID_READ)        == TRUE) && /* 00006 */
        (Mem_CheckLength(instanceId, sourceAddress, length, MEM_SID_READ) == TRUE) && /* 00072 */
        (Mem_CheckJobPending(instanceId, MEM_SID_READ)                    == TRUE))   /* 00007 */
    {
        Mem_PendingRequest[instanceId].Operation = MEM_OP_READ;
        Mem_PendingRequest[instanceId].Address   = sourceAddress;
        Mem_PendingRequest[instanceId].Length    = length;
        Mem_PendingRequest[instanceId].DestPtr   = destinationDataPtr;
        Mem_PendingRequest[instanceId].Started   = FALSE;

        Mem_InstanceRuntime[instanceId].JobPending = TRUE;
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_JOB_PENDING; /* [SWS_Mem_00030] */

        retVal = E_OK; /* job accepted; hardware trigger deferred to Mem_MainFunction() [SWS_Mem_00066] */
    }
    /* else: [SWS_Mem_00059] job rejected synchronously via E_NOT_OK, no state change */

    return retVal;
}

/* [SWS_Mem_10013] */
Std_ReturnType Mem_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_WRITE)                                 == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_WRITE)                    == TRUE) && /* 00009 */
        (Mem_CheckPointer(sourceDataPtr, MEM_SID_WRITE)                    == TRUE) && /* 00010 */
        (Mem_CheckAddress(instanceId, targetAddress, MEM_SID_WRITE)        == TRUE) && /* 00011 */
        (Mem_CheckLength(instanceId, targetAddress, length, MEM_SID_WRITE) == TRUE) && /* 00012 */
        (Mem_CheckJobPending(instanceId, MEM_SID_WRITE)                    == TRUE))   /* 00013 */
    {
        Mem_PendingRequest[instanceId].Operation = MEM_OP_WRITE;
        Mem_PendingRequest[instanceId].Address   = targetAddress;
        Mem_PendingRequest[instanceId].Length    = length;
        Mem_PendingRequest[instanceId].SrcPtr    = sourceDataPtr;
        Mem_PendingRequest[instanceId].Started   = FALSE;

        Mem_InstanceRuntime[instanceId].JobPending = TRUE;
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_JOB_PENDING;

        retVal = E_OK;
    }

    return retVal;
}

/* [SWS_Mem_10014] */
Std_ReturnType Mem_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_ERASE)                                         == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_ERASE)                            == TRUE) && /* 00015 */
        (Mem_CheckAddress(instanceId, targetAddress, MEM_SID_ERASE)                == TRUE) && /* 00016 */
        (Mem_CheckLength(instanceId, targetAddress, length, MEM_SID_ERASE)         == TRUE) && /* 00017 */
        (Mem_CheckEraseAlignment(instanceId, targetAddress, length, MEM_SID_ERASE) == TRUE) && /* 00016/00035 */
        (Mem_CheckJobPending(instanceId, MEM_SID_ERASE)                            == TRUE))   /* 00018 */
    {
        Mem_PendingRequest[instanceId].Operation = MEM_OP_ERASE;
        Mem_PendingRequest[instanceId].Address   = targetAddress;
        Mem_PendingRequest[instanceId].Length    = length;
        Mem_PendingRequest[instanceId].Started   = FALSE;

        Mem_InstanceRuntime[instanceId].JobPending = TRUE;
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_JOB_PENDING;

        retVal = E_OK;
    }

    return retVal;
}

/* [SWS_Mem_10016] */
Std_ReturnType Mem_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_BLANK_CHECK)                                 == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_BLANK_CHECK)                    == TRUE) && /* 00022 */
        (Mem_CheckAddress(instanceId, targetAddress, MEM_SID_BLANK_CHECK)        == TRUE) && /* 00023 */
        (Mem_CheckLength(instanceId, targetAddress, length, MEM_SID_BLANK_CHECK) == TRUE) && /* 00024 */
        (Mem_CheckJobPending(instanceId, MEM_SID_BLANK_CHECK)                    == TRUE))   /* 00025 */
    {
        Mem_PendingRequest[instanceId].Operation = MEM_OP_BLANK_CHECK;
        Mem_PendingRequest[instanceId].Address   = targetAddress;
        Mem_PendingRequest[instanceId].Length    = length;
        Mem_PendingRequest[instanceId].Started   = FALSE;

        Mem_InstanceRuntime[instanceId].JobPending = TRUE;
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_JOB_PENDING;

        retVal = E_OK;
    }

    return retVal;
}

/* [SWS_Mem_10017] */
Std_ReturnType Mem_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Mem_CheckModuleInit(MEM_SID_HW_SPECIFIC_SERVICE)              == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_HW_SPECIFIC_SERVICE) == TRUE) && /* 00026 */
        (Mem_CheckPointer(dataPtr, MEM_SID_HW_SPECIFIC_SERVICE)       == TRUE) && /* 00027 */
        (Mem_CheckPointer(lengthPtr, MEM_SID_HW_SPECIFIC_SERVICE)     == TRUE) && /* 00027 */
        (Mem_CheckJobPending(instanceId, MEM_SID_HW_SPECIFIC_SERVICE) == TRUE))   /* not numbered, see
                                                                                       Mem_CheckJobPending() */
    {
        Mem_PendingRequest[instanceId].Operation   = MEM_OP_HW_SPECIFIC;
        Mem_PendingRequest[instanceId].HwServiceId = hwServiceId;
        Mem_PendingRequest[instanceId].HwDataPtr   = dataPtr;
        Mem_PendingRequest[instanceId].HwLengthPtr = lengthPtr;
        Mem_PendingRequest[instanceId].Started     = FALSE;

        Mem_InstanceRuntime[instanceId].JobPending = TRUE;
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_JOB_PENDING;

        retVal = E_OK;
    }

    return retVal;
}

/*======================================================================================================================
 *  8.5  SCHEDULED FUNCTIONS
 *====================================================================================================================*/

/* [SWS_Mem_10010] Mem_MainFunction
 * [SWS_Mem_00066]: this is where every asynchronous job's hardware trigger actually happens (first pass,
 * Started == FALSE -> Started = TRUE), and where its completion is subsequently polled for (every pass
 * afterwards) via Mem_Ipw_MainFunction()/Mem_Ipw_GetJobResult(). */
void Mem_MainFunction(void)
{
    Mem_InstanceIdType i;

    if (Mem_ModuleState == MEM_INIT)
    {
        for (i = 0u; i < (Mem_InstanceIdType)MEM_INSTANCE_COUNT; i++)
        {
            if (Mem_InstanceRuntime[i].JobPending == TRUE)
            {
                if (Mem_PendingRequest[i].Started == FALSE)
                {
                    Std_ReturnType startResult = E_NOT_OK;

                    switch (Mem_PendingRequest[i].Operation)
                    {
                        case MEM_OP_READ:
                            startResult = Mem_Ipw_Read(i, Mem_PendingRequest[i].Address,
                                                        Mem_PendingRequest[i].DestPtr,
                                                        Mem_PendingRequest[i].Length);
                            break;

                        case MEM_OP_WRITE:
                            startResult = Mem_Ipw_Write(i, Mem_PendingRequest[i].Address,
                                                         Mem_PendingRequest[i].SrcPtr,
                                                         Mem_PendingRequest[i].Length);
                            break;

                        case MEM_OP_ERASE:
                            startResult = Mem_Ipw_Erase(i, Mem_PendingRequest[i].Address,
                                                         Mem_PendingRequest[i].Length);
                            break;

                        case MEM_OP_BLANK_CHECK:
                            startResult = Mem_Ipw_BlankCheck(i, Mem_PendingRequest[i].Address,
                                                              Mem_PendingRequest[i].Length);
                            break;

                        case MEM_OP_HW_SPECIFIC:
                            startResult = Mem_Ipw_HwSpecificService(i, Mem_PendingRequest[i].HwServiceId,
                                                                     Mem_PendingRequest[i].HwDataPtr,
                                                                     Mem_PendingRequest[i].HwLengthPtr);
                            break;

                        case MEM_OP_NONE:
                        default:
                            /* Should not happen while JobPending == TRUE; defensively treat as failure. */
                            break;
                    }

                    if (startResult == E_OK)
                    {
                        Mem_PendingRequest[i].Started = TRUE;
                    }
                    else
                    {
                        /* Hardware rejected the trigger at the last moment even though Mem_xxx() already
                         * validated parameters synchronously (should be rare / defensive path only).
                         * [SWS_Mem_00031] pending job not able to complete -> MEM_JOB_FAILED. */
                        Mem_InstanceRuntime[i].JobResult  = MEM_JOB_FAILED;
                        Mem_InstanceRuntime[i].JobPending = FALSE;

                        Mem_PendingRequest[i].Operation = MEM_OP_NONE;
                    }
                }

                if ((Mem_PendingRequest[i].Started == TRUE) && (Mem_InstanceRuntime[i].JobPending == TRUE))
                {
                    Mem_Ipw_MainFunction(i);

                    Mem_InstanceRuntime[i].JobResult = Mem_Ipw_GetJobResult(i);

                    if (Mem_InstanceRuntime[i].JobResult != MEM_JOB_PENDING)
                    {
                        /* [SWS_Mem_00067][SWS_Mem_00031][SWS_Mem_00076][SWS_Mem_00077][SWS_Mem_00078]
                         * job finished (successfully, failed, inconsistent or ECC (un)corrected) */
                        Mem_InstanceRuntime[i].JobPending = FALSE;
                        Mem_PendingRequest[i].Started     = FALSE;
                        Mem_PendingRequest[i].Operation   = MEM_OP_NONE;
                    }
                }
            }
        }
    }
}