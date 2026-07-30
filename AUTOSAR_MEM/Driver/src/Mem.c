/**********************************************************************************************************************
 *  FILE:         Mem.c
 *  MODULE:       Mem (Memory Driver)
 *  MÔ TẢ:        Hiện thực Memory Driver của AUTOSAR Classic Platform theo
 *                AUTOSAR_CP_SWS_MemoryDriver, Document ID 1018, AUTOSAR CP R25-11.
 *
 *  GHI CHÚ HIỆU CHỈNH (v2): Các dịch vụ bất đồng bộ (Read/Write/Erase/BlankCheck/
 *                HwSpecificService) chỉ kiểm tra tham số và lưu yêu cầu một cách đồng bộ;
 *                thao tác kích hoạt phần cứng thực sự diễn ra trong Mem_MainFunction(), theo [SWS_Mem_00066]
 *                ("All job requests triggered by asynchronous Mem driver services shall be executed
 *                within the Mem_MainFunction"). Phiên bản v1 kích hoạt phần cứng trực tiếp từ lời gọi
 *                API; khi diễn giải nghiêm ngặt 00066, đây là một sai lệch, xem phần traceability của dự án.
 *
 *  TRACEABILITY: Mỗi kiểm tra DET/hành vi bên dưới đều ghi đúng Requirement [SWS_Mem_xxxxx]
 *                tương ứng. Khi driver có kiểm tra không được một Requirement đánh số còn hiệu lực
 *                trong phiên bản tài liệu này (R25-11) yêu cầu, điều đó được nêu rõ thay vì viện dẫn
 *                một mã không có thật, xem ghi chú MEM_E_UNINIT bên dưới.
 *********************************************************************************************************************/

#include "Mem.h"
#include "Mem_IPW.h"

#if (MEM_DEV_ERROR_DETECT == STD_ON)
#include "det.h"
#endif

/*======================================================================================================================
 *  TRẠNG THÁI MODULE
 *====================================================================================================================*/
typedef enum
{
    MEM_UNINIT = 0,
    MEM_INIT
} Mem_StateType;

typedef struct
{
    MemAcc_MemJobResultType JobResult;      /* kết quả của job đã xử lý gần nhất, xem 7.2.1.1         */
    boolean                 JobPending;     /* TRUE từ lúc job được chấp nhận đến khi hoàn tất         */
    boolean                 SuspendActive;  /* TRUE khi job hiện tại đang tạm dừng                     */
} Mem_InstanceRuntimeType;

/* Dịch vụ bất đồng bộ nào được xếp hàng cho instance và các tham số của nó. Theo
 * [SWS_Mem_00066], lời gọi kích hoạt Mem_Ipw_xxx() phải diễn ra trong Mem_MainFunction(), không phải
 * trực tiếp trong API. Mỗi instance chỉ dùng một phần tử, phù hợp với [SWS_Mem_00057]. */
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
    Mem_DataType*         DestPtr;     /* bộ đệm đích của Mem_Read()                         */
    const Mem_DataType*   SrcPtr;      /* bộ đệm nguồn của Mem_Write()                       */
    Mem_HwServiceIdType   HwServiceId; /* bộ chọn dịch vụ Mem_HwSpecificService()            */
    Mem_DataType*         HwDataPtr;   /* bộ đệm dữ liệu Mem_HwSpecificService()             */
    Mem_LengthType*       HwLengthPtr; /* con trỏ độ dài Mem_HwSpecificService()             */
    boolean                Started;    /* TRUE khi Mem_Ipw_xxx() đã được gọi thực sự          */
} Mem_PendingRequestType;

static Mem_StateType           Mem_ModuleState = MEM_UNINIT;
static Mem_InstanceRuntimeType Mem_InstanceRuntime[MEM_INSTANCE_COUNT];
static Mem_PendingRequestType  Mem_PendingRequest[MEM_INSTANCE_COUNT];

/*======================================================================================================================
 *  MACRO BÁO LỖI DET
 *====================================================================================================================*/
#if (MEM_DEV_ERROR_DETECT == STD_ON)
#define MEM_DET_REPORT_ERROR(ApiId, ErrorId) \
            ((void)Det_ReportError(MEM_MODULE_ID, MEM_INDEX, (ApiId), (ErrorId)))
#else
#define MEM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*======================================================================================================================
 *  HÀM HỖ TRỢ KIỂM TRA THAM SỐ CỤC BỘ
 *  Mỗi hàm thực hiện kiểm tra theo Requirement [SWS_Mem_xxxxx] được viện dẫn và, nếu
 *  MEM_DEV_ERROR_DETECT == STD_ON, báo development error tương ứng bằng Det_ReportError().
 *  Trả về TRUE nếu tham số hợp lệ, ngược lại trả về FALSE.
 *====================================================================================================================*/

/* GHI CHÚ VỀ MEM_E_UNINIT: [SWS_Mem_00052] liệt kê "API service called without module initialization" là
 * development error hợp lệ (MEM_E_UNINIT). Tuy nhiên, phiên bản tài liệu này (R25-11) không có Requirement
 * đánh số, còn hiệu lực và riêng cho từng hàm để bắt buộc kiểm tra này với Mem_Read/Write/Erase/BlankCheck/
 * HwSpecificService/Suspend/Resume (các mã dự kiến như [SWS_Mem_00003]/[SWS_Mem_00008] đã bị DELETED trong
 * R23-11, xem Appendix A.3.3). Kiểm tra bên dưới vẫn được giữ lại như một biện pháp phòng vệ, phù hợp bảng lỗi
 * [SWS_Mem_00052] còn hiệu lực và quy ước chung SRS_BSW, nhưng không trace được tới một SWS_Mem cụ thể còn hiệu lực. */
static boolean Mem_CheckModuleInit(uint8 apiId)
{
    boolean valid = (boolean)(Mem_ModuleState == MEM_INIT);

    if (valid == FALSE)
    {
        MEM_DET_REPORT_ERROR(apiId, MEM_E_UNINIT); /* bảng lỗi [SWS_Mem_00052], xem ghi chú bên trên */
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
         * [SWS_Mem_00027](HwSpecificService dataPtr/lengthPtr) [SWS_Mem_00087](Init configPtr - đảo
         * điều kiện, xem Mem_Init()) [SWS_Mem_00002](GetVersionInfo versionInfoPtr) */
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
 * physical segmentation needs to be considered". Nghĩa là yêu cầu Erase không khớp chính xác một sector
 * vật lý (xem Flash_IP_Cfg.c) phải bị REJECTED, không được tự động làm tròn hoặc đệm. Tài liệu không định
 * nghĩa mã lỗi riêng cho trường hợp không căn theo phân đoạn vật lý, nên dùng MEM_E_PARAM_ADDRESS như kiểm tra
 * địa chỉ tổng quát ([SWS_Mem_00016]); với Mem_Erase, địa chỉ không căn sector là một dạng địa chỉ không hợp lệ. */
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
    /* [SRS_MemHwAb_14050] / [SWS_Mem_00057]: mỗi instance chỉ có một job tại một thời điểm */
    boolean valid = (boolean)(Mem_InstanceRuntime[instanceId].JobPending == FALSE);

    if (valid == FALSE)
    {
        /* [SWS_Mem_00007](Read) [SWS_Mem_00013](Write) [SWS_Mem_00018](Erase) [SWS_Mem_00025](BlankCheck).
         * Không có SWS_Mem đánh số riêng cho HwSpecificService; kiểm tra vẫn áp dụng để phù hợp
         * [SRS_MemHwAb_14050] (mỗi instance một job, không ngoại lệ theo từng dịch vụ). */
        MEM_DET_REPORT_ERROR(apiId, MEM_E_JOB_PENDING);
    }

    return valid;
}

/*======================================================================================================================
 *  8.3.1  HÀM ĐỒNG BỘ
 *====================================================================================================================*/

/* [SWS_Mem_10008] */
void Mem_Init(const Mem_ConfigType* configPtr)
{
    Mem_InstanceIdType i;

    /* [SWS_Mem_00087]: configPtr hiện chưa dùng và phải là NULL pointer */
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

    if (Mem_CheckModuleInit(MEM_SID_DEINIT) == TRUE)
    {
        for (i = 0u; i < (Mem_InstanceIdType)MEM_INSTANCE_COUNT; i++)
        {
            /* [SWS_Mem_00079]: hủy thao tác phần cứng đang diễn ra theo khả năng tốt nhất. Xem Flash_IP.c về
             * giới hạn single Flash bank: không thể hủy tuyệt đối thao tác đang có BSY. */
            Mem_Ipw_DeInit(i);

            Mem_InstanceRuntime[i].JobPending    = FALSE;
            Mem_InstanceRuntime[i].SuspendActive = FALSE;

            Mem_PendingRequest[i].Operation = MEM_OP_NONE;
            Mem_PendingRequest[i].Started   = FALSE;
        }

        Mem_ModuleState = MEM_UNINIT;
    }
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

    if ((Mem_CheckModuleInit(MEM_SID_GET_JOB_RESULT) == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_GET_JOB_RESULT) == TRUE)) /* [SWS_Mem_00090] */
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
        if (Mem_Ipw_IsSuspendResumeSupported(instanceId) == FALSE)
        {
            retVal = E_MEM_SERVICE_NOT_AVAIL; /* [SWS_Mem_00082] */
        }
        else if (Mem_InstanceRuntime[instanceId].SuspendActive == TRUE)
        {
            retVal = E_NOT_OK; /* [SWS_Mem_00083]: đã tạm dừng, từ chối mà không thực hiện thêm */
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
        if (Mem_Ipw_IsSuspendResumeSupported(instanceId) == FALSE)
        {
            retVal = E_MEM_SERVICE_NOT_AVAIL; /* [SWS_Mem_00082] */
        }
        else if (Mem_InstanceRuntime[instanceId].SuspendActive == FALSE)
        {
            retVal = E_NOT_OK; /* [SWS_Mem_00084]: không có suspend chờ, từ chối mà không thực hiện thêm */
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
    if ((Mem_CheckModuleInit(MEM_SID_PROPAGATE_ERROR) == TRUE) &&
        (Mem_CheckInstanceId(instanceId, MEM_SID_PROPAGATE_ERROR) == TRUE)) /* [SWS_Mem_00020] */
    {
        /* [SWS_Mem_00061]: đặt kết quả job là MEM_ECC_UNCORRECTED và hủy xử lý job hiện tại */
        Mem_InstanceRuntime[instanceId].JobResult  = MEM_ECC_UNCORRECTED;
        Mem_InstanceRuntime[instanceId].JobPending = FALSE;

        Mem_PendingRequest[instanceId].Operation = MEM_OP_NONE;
        Mem_PendingRequest[instanceId].Started   = FALSE;
    }
}

/*======================================================================================================================
 *  8.3.2  HÀM BẤT ĐỒNG BỘ
 *  Theo [SWS_Mem_00066], các hàm này kiểm tra tham số và lưu yêu cầu đã chấp nhận; việc kích hoạt phần cứng
 *  Mem_Ipw_xxx() diễn ra sau đó trong Mem_MainFunction(). Dịch vụ tùy chọn không được hiện thực cho công nghệ
 *  bộ nhớ sẽ bị từ chối đồng bộ bằng E_MEM_SERVICE_NOT_AVAIL theo [SWS_Mem_00070].
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

        retVal = E_OK; /* job được chấp nhận; kích hoạt phần cứng hoãn đến Mem_MainFunction() [SWS_Mem_00066] */
    }
    /* else: [SWS_Mem_00059]: job bị từ chối đồng bộ bằng E_NOT_OK, không đổi trạng thái */

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
        (Mem_CheckPointer(lengthPtr, MEM_SID_HW_SPECIFIC_SERVICE)     == TRUE))   /* 00027 */
    {
        if (Mem_Ipw_IsHwSpecificServiceSupported(instanceId, hwServiceId) == FALSE)
        {
            retVal = E_MEM_SERVICE_NOT_AVAIL; /* [SWS_Mem_00070] / API return contract of [SWS_Mem_10017] */
        }
        else if (Mem_CheckJobPending(instanceId, MEM_SID_HW_SPECIFIC_SERVICE) == TRUE) /* see
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
    }

    return retVal;
}

/*======================================================================================================================
 *  8.5  HÀM ĐƯỢC LẬP LỊCH
 *====================================================================================================================*/

/* [SWS_Mem_10010] Mem_MainFunction
 * [SWS_Mem_00066]: tại đây mỗi job bất đồng bộ thực sự kích hoạt phần cứng (lần đầu,
 * Started == FALSE -> Started = TRUE) và sau đó được kiểm tra hoàn tất ở mỗi lần gọi qua
 * Mem_Ipw_MainFunction()/Mem_Ipw_GetJobResult(). */
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
                            /* Không được xảy ra khi JobPending == TRUE; xử lý phòng vệ là lỗi. */
                            break;
                    }

                    if (startResult == E_OK)
                    {
                        Mem_PendingRequest[i].Started = TRUE;
                    }
                    else
                    {
                        /* Phần cứng từ chối kích hoạt dù Mem_xxx() đã kiểm tra tham số đồng bộ.
                         * Đây chỉ là nhánh phòng vệ, hiếm khi xảy ra.
                         * [SWS_Mem_00031]: pending job not able to complete -> MEM_JOB_FAILED. */
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
                         * job đã kết thúc: thành công, lỗi, không nhất quán hoặc ECC có/không thể sửa. */
                        Mem_InstanceRuntime[i].JobPending = FALSE;
                        Mem_PendingRequest[i].Started     = FALSE;
                        Mem_PendingRequest[i].Operation   = MEM_OP_NONE;
                    }
                }
            }
        }
    }
}
