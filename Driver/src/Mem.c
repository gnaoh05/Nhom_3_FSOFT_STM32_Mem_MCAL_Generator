#include "Mem.h"
#include "Mem_IPW.h"   /* Khai báo hàm tầng IPW */
#include "SchM_Mem.h"
#include <stdio.h>     /* Dùng printf cho mock log trên PC */
#if (MEM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h" /* Gắn module dò lỗi DET nếu cờ cấu hình bật */
#endif

/* Biến trạng thái toàn cục: Khóa các API khác nếu chưa gọi Mem_Init */
static uint8 Mem_InitState = MEM_UNINIT;

/* [SWS_Mem_00029] Mảng trạng thái lưu kết quả Job độc lập cho từng bộ nhớ */
static MemAcc_MemJobResultType Mem_JobResults[MEM_MAX_INSTANCES];

/* Định nghĩa nội bộ các loại thao tác để tính toán Alignment */
typedef enum {
    MEM_OP_READ,
    MEM_OP_WRITE,
    MEM_OP_ERASE
} Mem_OperationType;

/* 
 * Hàm Helper: Kiểm tra Bounds (Giới hạn) và Alignment (Căn lề)
 * Trả về 0 nếu hợp lệ, trả về mã lỗi DET nếu vi phạm.
 */
static uint8 Mem_ValidateAddressAndLength(Mem_AddressType address, Mem_LengthType length, Mem_OperationType opType) {
    uint16 i;
    uint32 batchStart, batchSize, batchEnd;
    uint32 requiredAlignment = 1u;
    
    if (length == 0u) {
        return MEM_E_PARAM_LENGTH;
    }
    
    /* Quét qua cấu hình vật lý của STM32F401RE do GUI sinh ra */
    for (i = 0; i < Mem_ConfigData.MemInstances[0].MemNumberOfBatches; i++) {
        batchStart = Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemStartAddress;
        batchSize  = Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemNumberOfSectors * 
                     Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemEraseSectorSize;
        batchEnd   = batchStart + batchSize;
        
        /* Xác định xem địa chỉ bắt đầu có thuộc Sector Batch này không */
        if ((address >= batchStart) && (address < batchEnd)) {
            
            /* Lấy kích thước căn lề quy định từ file cấu hình dựa trên loại lệnh */
            if (opType == MEM_OP_READ) {
                requiredAlignment = Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemMinReadSize;
            } else if (opType == MEM_OP_WRITE) {
                requiredAlignment = Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemWritePageSize;
            } else if (opType == MEM_OP_ERASE) {
                requiredAlignment = Mem_ConfigData.MemInstances[0].MemSectorBatches[i].MemEraseSectorSize;
            }
            
            /* [SWS_Mem_00006, SWS_Mem_00011, SWS_Mem_00016] Kiểm tra lỗi căn lề địa chỉ */
            if ((address - batchStart) % requiredAlignment != 0u) {
                return MEM_E_PARAM_ADDRESS;
            }
            
            /* [SWS_Mem_00072, SWS_Mem_00012, SWS_Mem_00017] 
             * Kiểm tra lỗi: 
             * 1. Lệnh thao tác vượt quá biên của Batch (Tầng trên MemAcc phải tự chia nhỏ lệnh)
             * 2. Chiều dài không chia hết cho Alignment
             */
            if (((address + length) > batchEnd) || (length % requiredAlignment != 0u)) {
                return MEM_E_PARAM_LENGTH;
            }
            
            return 0u; /* Trả về 0 tức là Hợp lệ (Không có lỗi) */
        }
    }
    
    /* Không thuộc bất kỳ vùng Flash nào được định nghĩa */
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
    }
    
    Mem_InitState = MEM_INIT; /* Mở khóa máy trạng thái toàn cục */
    
    /* 
     * TODO: Gọi API tầng IPW để cấu hình chân xung nhịp, Unlock Flash STM32 
     * Lúc này, thay vì dùng configPtr, bạn truyền địa chỉ biến cấu hình tự sinh Mem_ConfigData
     * xuống cho tầng IPW. Ví dụ:
     * 
     * Mem_Ipw_Init(&Mem_ConfigData); 
     */
}

void Mem_DeInit(void) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    /* Chặn API nếu module chưa được khởi tạo */
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, 0, MEM_DEINIT_ID, MEM_E_UNINIT);
        return;
    }
#endif
    Mem_InitState = MEM_UNINIT; /* Khóa máy trạng thái */
    
    /* [SWS_Mem_00079] TODO: Gọi API tầng IPW để hủy tiến trình ngầm, ngắt clock Flash */
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
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_GETJOBRESULT_ID, MEM_E_UNINIT);
        return MEM_JOB_FAILED;
    }
    /* [SWS_Mem_00090] Kiểm tra chéo (Cross-check) Instance ID */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_GETJOBRESULT_ID, MEM_E_PARAM_INSTANCE_ID);
        return MEM_JOB_FAILED;
    }
#endif
    /* Trả về kết quả Job mới nhất của bản thể được yêu cầu */
    return Mem_JobResults[instanceId];
}

Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_SUSPEND_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_SUSPEND_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
#endif
    /* [SWS_Mem_00082] Flash Cortex-M4/STM32F401 không hỗ trợ ngắt lệnh phần cứng */
    return E_MEM_SERVICE_NOT_AVAIL; 
}

Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_RESUME_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_RESUME_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
#endif
    /* Giống Suspend, từ chối do phần cứng không hỗ trợ */
    return E_MEM_SERVICE_NOT_AVAIL;
}

void Mem_PropagateError(Mem_InstanceIdType instanceId) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_PROPAGATEERROR_ID, MEM_E_UNINIT);
        return;
    }
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_PROPAGATEERROR_ID, MEM_E_PARAM_INSTANCE_ID);
        return;
    }
#endif
    /* [SWS_Mem_00061] Đánh dấu luồng lỗi ECC nghiêm trọng được kích hoạt bởi hàm ngắt (ISR) */
    Mem_JobResults[instanceId] = MEM_ECC_UNCORRECTED;
}

/* -------------------------------------------------------------------------
 * NHÓM HÀM ASYNCHRONOUS & SCHEDULED
 * ------------------------------------------------------------------------- */

/* Khai báo hàm IPW đã nằm trong Mem_IPW.h */

Std_ReturnType Mem_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_READ_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00004] Lỗi vượt biên ID */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_READ_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00005] Lỗi con trỏ trỏ tới vùng dữ liệu rỗng */
    if (destinationDataPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_READ_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00072] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    valErr = Mem_ValidateAddressAndLength(sourceAddress, length, MEM_OP_READ);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_READ_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00007] Lỗi xung đột: Mỗi bản thể chỉ xử lý 1 Job tại 1 thời điểm */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_READ_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    /* Ép sang Pending để khóa bản thể, ngăn các Request khác đánh xen ngang */
    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Std_ReturnType retVal = Mem_Ipw_Read(instanceId, sourceAddress, destinationDataPtr, length);
    
    /* Giải phóng cờ ngay lập tức nếu tầng IPW từ chối lệnh */
    if (retVal != E_OK) {
        Mem_JobResults[instanceId] = MEM_JOB_FAILED;
    }
    return retVal;
}

Std_ReturnType Mem_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00009] Lỗi ID bản thể sai */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00010] Con trỏ dữ liệu đầu vào NULL */
    if (sourceDataPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00012] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    valErr = Mem_ValidateAddressAndLength(targetAddress, length, MEM_OP_WRITE);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00013] Đảm bảo tính độc quyền tiến trình */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_WRITE_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Std_ReturnType retVal = Mem_Ipw_Write(instanceId, targetAddress, sourceDataPtr, length);
    
    if (retVal != E_OK) {
        Mem_JobResults[instanceId] = MEM_JOB_FAILED;
    }
    return retVal;
}

Std_ReturnType Mem_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_ERASE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00015] Kiểm tra ID bản thể */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_ERASE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00017] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề */
    valErr = Mem_ValidateAddressAndLength(targetAddress, length, MEM_OP_ERASE);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_ERASE_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00018] Ngăn xung đột tác vụ */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_ERASE_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Std_ReturnType retVal = Mem_Ipw_Erase(instanceId, targetAddress, length);
    
    if (retVal != E_OK) {
        Mem_JobResults[instanceId] = MEM_JOB_FAILED;
    }
    return retVal;
}

Std_ReturnType Mem_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    uint8 valErr;
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_BLANKCHECK_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00022] Kiểm tra ID bản thể */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_BLANKCHECK_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    
    /* [SWS_Mem_00024] Kiểm tra lỗi địa chỉ, độ dài vô nghĩa hoặc sai căn lề (Dùng quy tắc READ) */
    valErr = Mem_ValidateAddressAndLength(targetAddress, length, MEM_OP_READ);
    if (valErr != 0u) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_BLANKCHECK_ID, valErr);
        return E_NOT_OK;
    }

    /* [SWS_Mem_00025] Block tiến trình mới nếu đang bận */
    if (Mem_JobResults[instanceId] == MEM_JOB_PENDING) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_BLANKCHECK_ID, MEM_E_JOB_PENDING);
        return E_NOT_OK;
    }
#endif

    Mem_JobResults[instanceId] = MEM_JOB_PENDING;
    Std_ReturnType retVal = Mem_Ipw_BlankCheck(instanceId, targetAddress, length);
    
    if (retVal != E_OK) {
        Mem_JobResults[instanceId] = MEM_JOB_FAILED;
    }
    return retVal;
}

Std_ReturnType Mem_HwSpecificService(Mem_InstanceIdType instanceId, Mem_HwServiceIdType hwServiceId, Mem_DataType* dataPtr, Mem_LengthType* lengthPtr) {
#if (MEM_DEV_ERROR_DETECT == STD_ON)
    if (Mem_InitState == MEM_UNINIT) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_HWSPECIFICSERVICE_ID, MEM_E_UNINIT);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00026] Báo lỗi nếu sai ID phần cứng */
    if (instanceId >= MEM_MAX_INSTANCES) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_HWSPECIFICSERVICE_ID, MEM_E_PARAM_INSTANCE_ID);
        return E_NOT_OK;
    }
    /* [SWS_Mem_00027] Cấm truyền NULL cho con trỏ mang tham số đặc thù */
    if (dataPtr == NULL || lengthPtr == NULL) {
        Det_ReportError(MEM_MODULE_ID, instanceId, MEM_HWSPECIFICSERVICE_ID, MEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    /* Tạm thời từ chối, do chưa triển khai hàm custom trên tầng IPW */
    return E_MEM_SERVICE_NOT_AVAIL;
}

void Mem_MainFunction(void) {
    if (Mem_InitState == MEM_UNINIT) {
        return; 
    }

    uint8 i;
    for (i = 0; i < MEM_MAX_INSTANCES; i++) {
        if (Mem_JobResults[i] == MEM_JOB_PENDING) {
            
            /* Gọi xuống tầng IPW để hối thúc phần cứng chạy */
            Mem_Ipw_MainFunction(i);
            
            /* (Mock logic): Mở file Mem_IPW.c để xem, nếu mock_delay đếm về 0 nghĩa là giả lập xong */
            extern uint8 mock_delay[]; 
            if (mock_delay[i] == 0) {
                Mem_JobResults[i] = MEM_JOB_OK; /* Cập nhật trạng thái Job hoàn thành */
                printf("[CORE] Job %d hoan thanh (Da chuyen tu PENDING sang OK)\n", i);
            }
            
        }
    }
}