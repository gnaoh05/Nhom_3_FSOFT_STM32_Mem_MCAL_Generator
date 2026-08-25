#include "Fee.h"
#include "MemAcc.h"

#if (FEE_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

typedef enum
{
    FEE_JOB_NONE = 0U,
    FEE_JOB_READ,
    FEE_JOB_WRITE
} Fee_JobType;

static MemIf_StatusType    Fee_ModuleStatus = MEMIF_UNINIT;
static MemIf_JobResultType Fee_JobResult    = MEMIF_JOB_OK;
static Fee_JobType         Fee_CurrentJob   = FEE_JOB_NONE;

/**
 * @brief Tra cứu vị trí index trong bảng cấu hình dựa trên Logical Block Number.
 */
static boolean Fee_FindBlockConfig(uint16 BlockNumber, uint16 *ConfigIndex)
{
    boolean isFound = FALSE;
    uint16 i = 0U;

    for (i = 0U; i < FEE_NUM_BLOCKS; i++)
    {
        if (Fee_BlockConfigTable[i].blockNumber == BlockNumber)
        {
            if (ConfigIndex != NULL_PTR)
            {
                *ConfigIndex = i;
            }
            isFound = TRUE;
            break;
        }
    }

    return isFound;
}

/**
 * @brief Service to initialize the FEE module.
 */
void Fee_Init(const Fee_ConfigType *ConfigPtr)
{
    (void)ConfigPtr; /* [SWS_Fee_00189]: ConfigPtr is not used and shall be NULL_PTR */

    /* [SWS_Fee_00120]: Chuyển sang trạng thái bận nội bộ */
    Fee_ModuleStatus = MEMIF_BUSY_INTERNAL;

    Fee_JobResult    = MEMIF_JOB_OK;
    Fee_CurrentJob   = FEE_JOB_NONE;

    /* [SWS_Fee_00168]: Chuyển sang IDLE khi khởi tạo thành công */
    Fee_ModuleStatus = MEMIF_IDLE;
}

/**
 * @brief Service to de-initialize the FEE module.
 */
void Fee_DeInit(void)
{
    /* Kiểm tra nếu module chưa Init thì báo DET (nếu bật DET) */
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_DEINIT, (uint8)FEE_E_UNINIT);
#endif
    }
    else
    {
        /* Nếu đang có job bận ngầm bên dưới thì hủy tác vụ trên MemAcc */
        if (Fee_ModuleStatus == MEMIF_BUSY)
        {
            MemAcc_Cancel((MemAcc_AddressAreaIdType)FEE_MEMACC_ADDRESS_AREA_ID);
        }

        /* Đưa toàn bộ trạng thái nội bộ về mặc định */
        Fee_JobResult    = MEMIF_JOB_OK;
        Fee_CurrentJob   = FEE_JOB_NONE;
        Fee_ModuleStatus = MEMIF_UNINIT;
    }
}

/**
 * @brief Service to switch operational mode (Fast/Slow) of underlying driver.
 */
void Fee_SetMode(MemIf_ModeType Mode)
{
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_SET_MODE, (uint8)FEE_E_UNINIT);
#endif
    }
    else if (Fee_ModuleStatus == MEMIF_BUSY)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_SET_MODE, (uint8)FEE_E_BUSY);
#endif
    }
    else
    {
        /* [SWS_Fee_00020]: Chuyển mode cho driver nếu phần cứng hỗ trợ */
        (void)Mode;
    }
}

/**
 * @brief Service to initiate an asynchronous read job.
 */
Std_ReturnType Fee_Read(uint16 BlockNumber, 
                        uint16 BlockOffset, 
                        uint8 *DataBufferPtr, 
                        uint16 Length)
{
    Std_ReturnType retVal = E_NOT_OK;
    uint16 blockIdx = 0U;
    boolean isConfigured = FALSE;
    MemAcc_AddressType targetAddr = 0U;

    /* [SWS_Fee_00122]: Kiểm tra UNINIT */
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_UNINIT);
#endif
    }
    /* [SWS_Fee_00133]: Kiểm tra BUSY */
    else if (Fee_ModuleStatus == MEMIF_BUSY)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_BUSY);
#endif
    }
    /* [SWS_Fee_00136]: Kiểm tra NULL Pointer */
    else if (DataBufferPtr == NULL_PTR)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_PARAM_POINTER);
#endif
    }
    else
    {
        isConfigured = Fee_FindBlockConfig(BlockNumber, &blockIdx);

        /* [SWS_Fee_00134]: Kiểm tra Block Number hợp lệ */
        if (isConfigured == FALSE)
        {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_INVALID_BLOCK_NO);
#endif
        }
        /* [SWS_Fee_00135]: Kiểm tra Offset (Offset < configured block length) */
        else if (BlockOffset >= Fee_BlockConfigTable[blockIdx].blockSize)
        {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_INVALID_BLOCK_OFS);
#endif
        }
        /* [SWS_Fee_00137]: Kiểm tra Length (Offset + Length <= configured block length) */
        else if (((uint32)BlockOffset + (uint32)Length) > (uint32)Fee_BlockConfigTable[blockIdx].blockSize)
        {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_READ, (uint8)FEE_E_INVALID_BLOCK_LEN);
#endif
        }
        /* [SWS_Fee_00022]: Module ở trạng thái IDLE hoặc BUSY_INTERNAL -> Tiếp nhận Job */
        else
        {
            /* [SWS_Fee_00021]: Tính toán địa chỉ logic đẩy xuống MemAcc */
            targetAddr = Fee_BlockConfigTable[blockIdx].logicalAddress + (MemAcc_AddressType)BlockOffset;

            /* Chuyển trạng thái sang BUSY & PENDING */
            Fee_ModuleStatus = MEMIF_BUSY;
            Fee_JobResult    = MEMIF_JOB_PENDING;
            Fee_CurrentJob   = FEE_JOB_READ;

            /* Gọi xuống MemAcc */
            retVal = MemAcc_Read((MemAcc_AddressAreaIdType)FEE_MEMACC_ADDRESS_AREA_ID, 
                                 targetAddr, 
                                 DataBufferPtr, 
                                 (MemAcc_LengthType)Length);
            
            if (retVal != E_OK)
            {
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_JobResult    = MEMIF_JOB_FAILED;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
        }
    }

    return retVal;
}

/**
 * @brief Service to initiate an asynchronous write job.
 */
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8 *DataBufferPtr)
{
    Std_ReturnType retVal = E_NOT_OK;
    uint16 blockIdx = 0U;
    boolean isConfigured = FALSE;
    MemAcc_AddressType targetAddr = 0U;
    MemAcc_LengthType blockLen = 0U;

    /* [SWS_Fee_00123]: Kiểm tra UNINIT */
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_WRITE, (uint8)FEE_E_UNINIT);
#endif
    }
    /* [SWS_Fee_00144]: Kiểm tra BUSY */
    else if (Fee_ModuleStatus == MEMIF_BUSY)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_WRITE, (uint8)FEE_E_BUSY);
#endif
    }
    /* [SWS_Fee_00139]: Kiểm tra con trỏ NULL */
    else if (DataBufferPtr == NULL_PTR)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_WRITE, (uint8)FEE_E_PARAM_POINTER);
#endif
    }
    else
    {
        isConfigured = Fee_FindBlockConfig(BlockNumber, &blockIdx);

        /* [SWS_Fee_00138]: Kiểm tra Block Number hợp lệ */
        if (isConfigured == FALSE)
        {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_WRITE, (uint8)FEE_E_INVALID_BLOCK_NO);
#endif
        }
        /* [SWS_Fee_00025]: Module ở trạng thái IDLE hoặc BUSY_INTERNAL -> Tiếp nhận Job */
        else
        {
            /* [SWS_Fee_00024]: Offset ghi luôn cố định bằng 0 */
            targetAddr = Fee_BlockConfigTable[blockIdx].logicalAddress;
            blockLen   = (MemAcc_LengthType)Fee_BlockConfigTable[blockIdx].blockSize;

            Fee_ModuleStatus = MEMIF_BUSY;
            Fee_JobResult    = MEMIF_JOB_PENDING;
            Fee_CurrentJob   = FEE_JOB_WRITE;

            /* Gọi xuống MemAcc */
            retVal = MemAcc_Write((MemAcc_AddressAreaIdType)FEE_MEMACC_ADDRESS_AREA_ID, 
                                  targetAddr, 
                                  DataBufferPtr, 
                                  blockLen);
            
            if (retVal != E_OK)
            {
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_JobResult    = MEMIF_JOB_FAILED;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
        }
    }

    return retVal;
}

/**
 * @brief Service to return the current status of the FEE module.
 */
MemIf_StatusType Fee_GetStatus(void)
{
    /* [SWS_Fee_00034], [SWS_Fee_00128], [SWS_Fee_00129], [SWS_Fee_00074] */
    return Fee_ModuleStatus;
}

/**
 * @brief Service to query the result of the last accepted job issued by upper layer.
 */
MemIf_JobResultType Fee_GetJobResult(void)
{
    MemIf_JobResultType retResult = MEMIF_JOB_FAILED;

    /* [SWS_Fee_00125]: Kiểm tra UNINIT */
    if (Fee_ModuleStatus == MEMIF_UNINIT)
    {
#if (FEE_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError((uint16)FEE_MODULE_ID, (uint8)FEE_INSTANCE_ID, (uint8)FEE_SID_GET_JOB_RESULT, (uint8)FEE_E_UNINIT);
#endif
        retResult = MEMIF_JOB_FAILED;
    }
    else
    {
        /* [SWS_Fee_00155]: Trả về kết quả Job gần nhất của Upper layer */
        retResult = Fee_JobResult;
    }

    return retResult;
}

/**
 * @brief Cyclic main function handling the Fee asynchronous state machine.
 */
void Fee_MainFunction(void)
{
    MemAcc_JobStatusType accStatus = MEMACC_JOB_IDLE;
    MemAcc_JobResultType accResult = MEMACC_OK;

    if (Fee_ModuleStatus == MEMIF_BUSY)
    {
        /* Kiểm tra trạng thái xử lý ngầm từ MemAcc */
        accStatus = MemAcc_GetJobStatus((MemAcc_AddressAreaIdType)FEE_MEMACC_ADDRESS_AREA_ID);

        if (accStatus == MEMACC_JOB_IDLE)
        {
            accResult = MemAcc_GetJobResult((MemAcc_AddressAreaIdType)FEE_MEMACC_ADDRESS_AREA_ID);

            if (accResult == MEMACC_OK)
            {
                /* [SWS_Fee_00035] */
                Fee_JobResult    = MEMIF_JOB_OK;
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
            else if (accResult == MEMACC_INCONSISTENT)
            {
                /* [SWS_Fee_00159] */
                Fee_JobResult    = MEMIF_BLOCK_INCONSISTENT;
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
            else if (accResult == MEMACC_CANCELED)
            {
                /* [SWS_Fee_00157] */
                Fee_JobResult    = MEMIF_JOB_CANCELED;
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
            else
            {
                /* [SWS_Fee_00158] */
                Fee_JobResult    = MEMIF_JOB_FAILED;
                Fee_ModuleStatus = MEMIF_IDLE;
                Fee_CurrentJob   = FEE_JOB_NONE;
            }
        }
        else
        {
            /* [SWS_Fee_00156]: MemAcc vẫn đang bận xử lý (MEMACC_JOB_PENDING) */
        }
    }
}