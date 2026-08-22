#include "Mem.h"
#include "Mem_IPW.h"   /* Khai báo hàm tầng IPW */
#include "SchM_Mem.h"

#if (MEM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h" /* Gắn module dò lỗi DET nếu cờ cấu hình bật */
#endif

/* Biến trạng thái toàn cục: Khóa các API khác nếu chưa gọi Mem_Init */
static uint8 Mem_InitState = MEM_UNINIT;

/* [SWS_Mem_00029] Mảng trạng thái lưu kết quả Job độc lập cho từng bộ nhớ */
static MemAcc_MemJobResultType Mem_JobResults[MEM_MAX_INSTANCES];

/* --- [SWS_Mem_00066] Cấu trúc Context Tracker (State Machine) --- */
typedef enum {
    MEM_JOB_ACTION_IDLE = 0,
    MEM_JOB_ACTION_READ,
    MEM_JOB_ACTION_WRITE,
    MEM_JOB_ACTION_ERASE,
    MEM_JOB_ACTION_BLANKCHECK
} Mem_JobActionType;
/*
 * Hàm Helper: Kiểm tra Bounds, Alignment VÀ trả về ChunkSize
 */
typedef struct {
    Mem_JobActionType   Action;
    Mem_AddressType     CurrentAddress;      /* Địa chỉ đang tiến hành thao tác */
    Mem_DataType*       CurrentDataPtr;      /* Con trỏ mảng đọc */
    const Mem_DataType* CurrentWriteDataPtr; /* Con trỏ mảng ghi */
    Mem_LengthType      RemainingLength;     /* Chiều dài CÒN LẠI cần đẩy xuống */
    Mem_LengthType      ChunkSize;           /* Kích thước mỗi khối chia nhỏ (Vd: Page Size) */
} Mem_JobContextType;

static Mem_JobContextType Mem_JobContext[MEM_MAX_INSTANCES];
/* --------------------------------------------------------------- */

/* Định nghĩa nội bộ các loại thao tác để tính toán Alignment */
typedef enum {
    MEM_OP_READ,
    MEM_OP_WRITE,
    MEM_OP_ERASE,
    MEM_OP_BLANKCHECK
} Mem_OperationType;

/*
 * Hàm Helper: Kiểm tra Bounds, Alignment VÀ trả về ChunkSize
 */
static uint8 Mem_ValidateAddressAndLength(Mem_InstanceIdType instanceId, Mem_AddressType address, Mem_LengthType length, Mem_OperationType opType, uint32* outChunkSize) {
    uint16 i;
    uint32 batchStart, batchSize, batchEnd;
    uint32 requiredAlignment = 1u;
    
    if (length == 0u) {
        return MEM_E_PARAM_LENGTH;
    }
    
    for (i = 0; i < Mem_ConfigData.MemInstances[instanceId].MemNumberOfBatches; i++) {
        batchStart = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemStartAddress;
        batchSize  = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemNumberOfSectors * 
                     Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemEraseSectorSize;

        batchEnd   = batchStart + batchSize;
        
        if ((address >= batchStart) && (address < batchEnd)) {
            
            if (opType == MEM_OP_READ) {
                requiredAlignment = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemMinReadSize;
            } else if (opType == MEM_OP_WRITE) {
                requiredAlignment = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemWritePageSize;
            } else if (opType == MEM_OP_ERASE) {
                requiredAlignment = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemEraseSectorSize;
            } else if (opType == MEM_OP_BLANKCHECK) {
                requiredAlignment = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemMinReadSize;
            }
            
            if ((address - batchStart) % requiredAlignment != 0u) {
                return MEM_E_PARAM_ADDRESS;
            }
            
            if (((address + length) > batchEnd) || (length % requiredAlignment != 0u)) {
                return MEM_E_PARAM_LENGTH;
            }
            
            if (outChunkSize != NULL) {
                if (opType == MEM_OP_BLANKCHECK) {
                    *outChunkSize = Mem_ConfigData.MemInstances[instanceId].MemSectorBatches[i].MemEraseSectorSize;
                } else {
                    *outChunkSize = requiredAlignment; 
                }
            }
            return 0u; 
        }
    }
    return MEM_E_PARAM_ADDRESS; 
}
/* -------------------------------------------------------------------------
 * NHÓM HÀM SYNCHRONOUS
 * ------------------------------------------------------------------------- */

/* 
 * Khai báo biến cấu hình toàn cục. 
 * Biến này SẼ ĐƯỢC SINH TỰ ĐỘNG trong file Mem_Cfg.c bởi Tool GUI.
 */
extern const Mem_ConfigType Mem_ConfigData; 

void Mem_Init(const Mem_ConfigType* configPtr) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    /* 
     * [SWS_Mem_00087] Module Mem dùng VARIANT-PRE-COMPILE, configPtr TỪ TẦNG TRÊN TRUYỀN XUỐNG 
     * BẮT BUỘC phải là NULL. Nếu truyền địa chỉ vào đây là vi phạm chuẩn.
     */
    if (configPtr != NULL) {
        Det_ReportError(MEM_MODULE_ID, 0, MEM_INIT_ID, MEM_E_PARAM_POINTER);
        return;
    }
#else
    (void)configPtr; /* Ép kiểu void tránh cảnh báo unused parameter */
#endif

    uint8 i;
    /* [SWS_Mem_00001] Reset toàn bộ trạng thái thiết bị về OK khi khởi tạo */
    for (i = 0; i < MEM_MAX_INSTANCES; i++) {
        Mem_JobResults[i] = MEM_JOB_OK;
        Mem_JobContext[i].Action = MEM_JOB_ACTION_IDLE; /* Đặt lại Context */
    }
    
    Mem_InitState = MEM_INIT; /* Mở khóa máy trạng thái toàn cục */
    
    /* Mở khóa và khởi tạo tầng phần cứng Flash_IP thông qua IPW */
    Mem_Ipw_Init(&Mem_ConfigData); 
}

void Mem_DeInit(void) {
    uint8 i;
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    /* Chặn API nếu module chưa được khởi tạo */
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, 0, MEM_DEINIT_ID, MEM_E_UNINIT);
        return;
    }
#endif

    /* [SWS_Mem_00079] Hủy mọi tiến trình đang chạy và de-init trạng thái nội bộ */
    for (i = 0; i < MEM_MAX_INSTANCES; i++) {
        if (Mem_JobResults[i] == MEM_JOB_PENDING) {
            Mem_Ipw_Cancel(i); /* Hủy tiến trình phần cứng */
            Mem_JobResults[i] = MEM_JOB_FAILED; 
            Mem_JobContext[i].Action = MEM_JOB_ACTION_IDLE;
        }
    }

    Mem_InitState = MEM_UNINIT; /* Khóa máy trạng thái */
    
    /* TODO: Gọi API tầng IPW để ngắt clock Flash */
}

void Mem_GetVersionInfo(Std_VersionInfoType* versionInfoPtr) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    /* [SWS_Mem_00002] Bảo vệ con trỏ NULL để tránh Crash do lỗi phân vùng (Segfault) */
    if (versionInfoPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, 0, MEM_GETVERSIONINFO_ID, MEM_E_PARAM_POINTER);
        return;
    }
#endif
    versionInfoPtr->vendorID = 0x0000; 
    versionInfoPtr->moduleID = MEM_MODULE_ID;
    versionInfoPtr->sw_major_version = 1;
    versionInfoPtr->sw_minor_version = 0;
    versionInfoPtr->sw_patch_version = 0;
}

MemAcc_MemJobResultType Mem_GetJobResult(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_GETJOBRESULT_ID, MEM_E_UNINIT);
        return MEM_JOB_FAILED;
    }
    /* [SWS_Mem_00090] Kiểm tra chéo (Cross-check) Instance ID */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_GETJOBRESULT_ID, MEM_E_PARAM_INSTANCE_ID);
        return MEM_JOB_FAILED;
    }
#endif
    /* Trả về kết quả Job mới nhất của bản thể được yêu cầu */
    return Mem_JobResults[instanceId];
}

Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_SUSPEND_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_SUSPEND_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
#endif
    (void)instanceId; /* Triệt tiêu warning */
    /* [SWS_Mem_00082] Flash Cortex-M4/STM32F401 không hỗ trợ ngắt lệnh phần cứng */
    return E_MEM_SERVICE_NOT_AVAIL; 
}

Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_RESUME_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_RESUME_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
#endif
    (void)instanceId; /* Triệt tiêu warning */
    /* Giống Suspend, từ chối do phần cứng không hỗ trợ */
    return E_MEM_SERVICE_NOT_AVAIL;
}

void Mem_PropagateError(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_PROPAGATEERROR_ID, MEM_E_UNINIT);
        return;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_PROPAGATEERROR_ID, MEM_E_PARAM_INSTANCE_ID);
        return;
    }
#endif
    /* [SWS_Mem_00061] Hủy bỏ tiến trình job hiện tại đang chạy và đánh dấu lỗi ECC */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Mem_Ipw_Cancel(instanceId);
        Mem_JobContext[instanceId].Action = MEM_JOB_ACTION_IDLE;
    }
    Mem_JobResults[instanceId] = MEM_ECC_UNCORRECTED;
}

/* -------------------------------------------------------------------------
 * NHÓM HÀM ASYNCHRONOUS & SCHEDULED
 * ------------------------------------------------------------------------- */

Std_ReturnType Mem_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_READ_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00004] Lỗi vượt biên ID */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_READ_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00005] Lỗi con trỏ trỏ tới vùng dữ liệu rỗng */
    if (destinationDataPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_READ_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00072] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    uint32 chunkSz = 0;
    valErr = Mem_ValidateAddressAndLength(instanceId, sourceAddress, length, MEM_OP_READ, &chunkSz);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_READ_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00007] Lỗi xung đột: Mỗi bản thể chỉ xử lý 1 Job tại 1 thời điểm */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_READ_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    /* [SWS_Mem_00066] Lưu context thực thi bất đồng bộ */
    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Mem_JobContext[instanceId].Action = MEM_JOB_ACTION_READ;
    Mem_JobContext[instanceId].CurrentAddress = sourceAddress;
    Mem_JobContext[instanceId].CurrentDataPtr = destinationDataPtr;
    Mem_JobContext[instanceId].RemainingLength = length;
    Mem_JobContext[instanceId].ChunkSize = chunkSz; /* Nạp kích thước chia nhỏ */
    
    return E_OK;
}

Std_ReturnType Mem_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_WRITE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00009] Lỗi ID bản thể sai */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_WRITE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00010] Con trỏ dữ liệu đầu vào NULL */
    if (sourceDataPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_WRITE_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00012] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    uint32 chunkSz = 0;
    valErr = Mem_ValidateAddressAndLength(instanceId, targetAddress, length, MEM_OP_WRITE, &chunkSz);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_WRITE_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00013] Đảm bảo tính độc quyền tiến trình */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    /* [SWS_Mem_00066] Lưu context thực thi bất đồng bộ */
    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Mem_JobContext[instanceId].Action = MEM_JOB_ACTION_WRITE;
    Mem_JobContext[instanceId].CurrentAddress = targetAddress;
    Mem_JobContext[instanceId].CurrentWriteDataPtr = sourceDataPtr;
    Mem_JobContext[instanceId].RemainingLength = length;
    Mem_JobContext[instanceId].ChunkSize = chunkSz;
    
    return E_OK;
}

Std_ReturnType Mem_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_ERASE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00015] Kiểm tra ID bản thể */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_ERASE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00017] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    uint32 chunkSz = 0;
    valErr = Mem_ValidateAddressAndLength(instanceId, targetAddress, length, MEM_OP_ERASE, &chunkSz);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_ERASE_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00018] Ngăn xung đột tác vụ */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_ERASE_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    /* [SWS_Mem_00066] Lưu context thực thi bất đồng bộ */
    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Mem_JobContext[instanceId].Action = MEM_JOB_ACTION_ERASE;
    Mem_JobContext[instanceId].CurrentAddress = targetAddress;
    Mem_JobContext[instanceId].RemainingLength = length;
    Mem_JobContext[instanceId].ChunkSize = chunkSz;
    
    return E_OK;
}

Std_ReturnType Mem_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_BLANKCHECK_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00022] Kiểm tra ID bản thể */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_BLANKCHECK_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00024] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề (Dùng quy tắc READ) */
    uint32 chunkSz = 0;
    valErr = Mem_ValidateAddressAndLength(instanceId, targetAddress, length, MEM_OP_BLANKCHECK, &chunkSz);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_BLANKCHECK_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00025] Block tiến trình mới nếu đang bận */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_BLANKCHECK_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    /* [SWS_Mem_00066] Lưu context thực thi bất đồng bộ */
    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Mem_JobContext[instanceId].Action = MEM_JOB_ACTION_BLANKCHECK;
    Mem_JobContext[instanceId].CurrentAddress = targetAddress;
    Mem_JobContext[instanceId].RemainingLength = length;
    Mem_JobContext[instanceId].ChunkSize = chunkSz;
    
    return E_OK;
}

Std_ReturnType Mem_HwSpecificService(Mem_InstanceIdType instanceId, Mem_HwServiceIdType hwServiceId, Mem_DataType* dataPtr, Mem_LengthType* lengthPtr) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_HWSPECIFICSERVICE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00026] Báo lỗi nếu sai ID phần cứng */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_HWSPECIFICSERVICE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00027] Cấm truyền NULL cho con trỏ mang tham số đặc thù */
    if (dataPtr == NULL || lengthPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, MEM_INDEX, MEM_HWSPECIFICSERVICE_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    (void)instanceId; (void)hwServiceId; (void)dataPtr; (void)lengthPtr; /* Triệt tiêu warning */
    /* Tạm thời từ chối, do chưa triển khai hàm custom trên tầng IPW */
    return E_MEM_SERVICE_NOT_AVAIL;
}

void Mem_MainFunction(void) {
    if (Mem_InitState == MEM_UNINIT) {
        return; 
    }

    uint8 i;
    for (i = 0; i < MEM_MAX_INSTANCES; i++) {
        
        /* Chạy hàm ngầm mô phỏng của phần cứng trước để cập nhật Timer/Cờ */
        Mem_Ipw_MainFunction(i);

        if (Mem_JobResults[i] == MEM_JOB_PENDING) {
            
            Mem_Ipw_StatusType ipwStatus = Mem_Ipw_GetStatus(i);
            
            /* Giao thức Handshake: Chỉ đẩy lệnh xuống khi phần cứng báo RẢNH (IDLE) */
            if (ipwStatus == MEM_IPW_IDLE) {
                
                /* Đã xử lý hết chiều dài yêu cầu -> Báo hoàn thành lên HĐH */
                if (Mem_JobContext[i].RemainingLength == 0) {
                    Mem_JobResults[i] = MEM_JOB_OK;
                    Mem_JobContext[i].Action = MEM_JOB_ACTION_IDLE;
                } 
                /* Vẫn còn chiều dài -> Cắt 1 chunk và đẩy xuống IPW */
                else {
                    Mem_LengthType processLen = Mem_JobContext[i].ChunkSize;
                    /* Chốt khối cuối cùng nếu dung lượng còn lại nhỏ hơn ChunkSize */
                    if (Mem_JobContext[i].RemainingLength < processLen) 
                    {
                        processLen = Mem_JobContext[i].RemainingLength;
                    }

                    Std_ReturnType hwStatus = E_NOT_OK;
                    
                    switch (Mem_JobContext[i].Action) {
                        case MEM_JOB_ACTION_READ:
                            hwStatus = Mem_Ipw_Read(i, Mem_JobContext[i].CurrentAddress, Mem_JobContext[i].CurrentDataPtr, processLen);
                            /* Dịch con trỏ mảng ghi đè */
                            if (hwStatus == E_OK && Mem_JobContext[i].CurrentDataPtr != NULL) {
                                Mem_JobContext[i].CurrentDataPtr += processLen;
                            }
                            break;
                        case MEM_JOB_ACTION_WRITE:
                            hwStatus = Mem_Ipw_Write(i, Mem_JobContext[i].CurrentAddress, Mem_JobContext[i].CurrentWriteDataPtr, processLen);
                            if (hwStatus == E_OK && Mem_JobContext[i].CurrentWriteDataPtr != NULL) {
                                Mem_JobContext[i].CurrentWriteDataPtr += processLen;
                            }
                            break;
                        case MEM_JOB_ACTION_ERASE:
                            hwStatus = Mem_Ipw_Erase(i, Mem_JobContext[i].CurrentAddress, processLen);
                            break;
                        case MEM_JOB_ACTION_BLANKCHECK:
                            hwStatus = Mem_Ipw_BlankCheck(i, Mem_JobContext[i].CurrentAddress, processLen);
                            break;
                        default: break;
                    }

                    if (hwStatus == E_OK) {
                        /* Tiến địa chỉ lên và trừ đi số bytes còn lại */
                        Mem_JobContext[i].CurrentAddress += processLen;
                        Mem_JobContext[i].RemainingLength -= processLen;
                    } 
                    else {
                        /* Tầng IPW trả về E_NOT_OK */
                        if (Mem_JobContext[i].Action == MEM_JOB_ACTION_BLANKCHECK) {
                            /* Nếu là lệnh BlankCheck mà IPW báo E_NOT_OK -> Tức là vùng nhớ KHÔNG rỗng */
                            Mem_JobResults[i] = MEM_INCONSISTENT; 
                        } 
                        else {
                            /* Các lệnh Read/Write/Erase bị từ chối -> Lỗi phần cứng thực sự */
                            Mem_JobResults[i] = MEM_JOB_FAILED;
                        }
                        
                        /* Reset Context Tracker do Job đã kết thúc sớm */
                        Mem_JobContext[i].Action = MEM_JOB_ACTION_IDLE;
                    }
                }
            }
            else if (ipwStatus == MEM_IPW_ERROR) {
                /* Phần cứng báo lỗi (VD: Error, Inconsistent, Write Protect) trong lúc đang xử lý ngầm */
                if (Mem_JobContext[i].Action == MEM_JOB_ACTION_BLANKCHECK) {
                    Mem_JobResults[i] = MEM_INCONSISTENT;
                } else {
                    Mem_JobResults[i] = MEM_JOB_FAILED;
                }
                Mem_JobContext[i].Action = MEM_JOB_ACTION_IDLE; /* Hủy bỏ Job */
            }
            /* Còn nếu ipwStatus == MEM_IPW_BUSY -> CORE KHÔNG LÀM GÌ CẢ (Chờ MainFunction chu kỳ sau) */
        }
    }
}
