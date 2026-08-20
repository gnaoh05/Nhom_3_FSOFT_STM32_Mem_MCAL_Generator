#include "MemAcc.h"
#include "Mem.h" /* To call Mem_Read, Mem_Write, Mem_Erase, Mem_GetJobResult */

/* Tối giản: Biến trạng thái quản lý các Address Area */
typedef struct {
    MemAcc_JobStatusType Status;
    MemAcc_JobResultType Result;
    MemAcc_JobType       CurrentJob;

    /* Lưu thông số của request hiện tại */
    MemAcc_AddressType   TargetAddress;
    MemAcc_DataType*     DataPtr;
    MemAcc_LengthType    Length;
} MemAcc_AddressAreaStateType;

static MemAcc_AddressAreaStateType MemAcc_State[MEMACC_MAX_ADDRESS_AREAS];
static uint8 MemAcc_InitStatus = MEMACC_E_UNINIT;

/* [SWS_MemAcc_10015] */
void MemAcc_Init(const MemAcc_ConfigType* configPtr)
{
    (void)configPtr;
    uint8 i;
    for (i = 0; i < MEMACC_MAX_ADDRESS_AREAS; i++) {
        MemAcc_State[i].Status = MEMACC_JOB_IDLE;
        MemAcc_State[i].Result = MEMACC_OK; /* Theo SWS_MemAcc_00112 */
        MemAcc_State[i].CurrentJob = MEMACC_NO_JOB;
    }
    MemAcc_InitStatus = 0; /* Module is initialized */
}

/* [SWS_MemAcc_10041] */
void MemAcc_DeInit(void)
{
    MemAcc_InitStatus = MEMACC_E_UNINIT;
}

/* [SWS_MemAcc_10016] */
void MemAcc_GetVersionInfo(Std_VersionInfoType* versionInfoPtr)
{
    if (versionInfoPtr != NULL_PTR) {
        versionInfoPtr->vendorID = 0;
        versionInfoPtr->moduleID = MEMACC_MODULE_ID;
        versionInfoPtr->sw_major_version = 1;
        versionInfoPtr->sw_minor_version = 0;
        versionInfoPtr->sw_patch_version = 0;
    }
}

/* [SWS_MemAcc_10019] */
MemAcc_JobResultType MemAcc_GetJobResult(MemAcc_AddressAreaIdType addressAreaId)
{
    if (addressAreaId < MEMACC_MAX_ADDRESS_AREAS) {
        return MemAcc_State[addressAreaId].Result;
    }
    return MEMACC_FAILED;
}

/* [SWS_MemAcc_10040] */
MemAcc_JobStatusType MemAcc_GetJobStatus(MemAcc_AddressAreaIdType addressAreaId)
{
    if (addressAreaId < MEMACC_MAX_ADDRESS_AREAS) {
        return MemAcc_State[addressAreaId].Status;
    }
    return MEMACC_JOB_IDLE;
}

/* Helper function để ánh xạ Address Area -> Mem Driver Instance */
static Std_ReturnType MemAcc_MapAddress(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType logicalAddr, Mem_InstanceIdType* instId, Mem_AddressType* physicalAddr)
{
    if (addressAreaId >= MEMACC_MAX_ADDRESS_AREAS) return E_NOT_OK;
    const MemAcc_AddressAreaConfigType* config = &MemAcc_AddressAreaConfig[addressAreaId];

    if (logicalAddr < config->LogicalStartAddress || 
        logicalAddr >= (config->LogicalStartAddress + config->Length)) {
        return E_NOT_OK; /* Out of bounds */
    }

    *instId = config->MemInstanceId;
    /* Ánh xạ địa chỉ logic sang vật lý */
    *physicalAddr = config->PhysicalStartAddress + (logicalAddr - config->LogicalStartAddress);
    return E_OK;
}

/* [SWS_MemAcc_10023] */
Std_ReturnType MemAcc_Read(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType sourceAddress, MemAcc_DataType* destinationDataPtr, MemAcc_LengthType length)
{
    if (MemAcc_InitStatus == MEMACC_E_UNINIT) return E_NOT_OK;
    if (addressAreaId >= MEMACC_MAX_ADDRESS_AREAS) return E_NOT_OK;
    if (MemAcc_State[addressAreaId].Status == MEMACC_JOB_PENDING) return E_NOT_OK;

    Mem_InstanceIdType instId;
    Mem_AddressType physAddr;
    if (MemAcc_MapAddress(addressAreaId, sourceAddress, &instId, &physAddr) != E_OK) return E_NOT_OK;
    /* Có thể check bounds cho cả length ở đây */

    /* Dispatch to Mem Driver */
    Std_ReturnType ret = Mem_Read(instId, physAddr, destinationDataPtr, length);
    if (ret == E_OK) {
        MemAcc_State[addressAreaId].Status = MEMACC_JOB_PENDING;
        MemAcc_State[addressAreaId].CurrentJob = MEMACC_READ_JOB;
    }
    return ret;
}

/* [SWS_MemAcc_10024] */
Std_ReturnType MemAcc_Write(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType targetAddress, const MemAcc_DataType* sourceDataPtr, MemAcc_LengthType length)
{
    if (MemAcc_InitStatus == MEMACC_E_UNINIT) return E_NOT_OK;
    if (addressAreaId >= MEMACC_MAX_ADDRESS_AREAS) return E_NOT_OK;
    if (MemAcc_State[addressAreaId].Status == MEMACC_JOB_PENDING) return E_NOT_OK;

    Mem_InstanceIdType instId;
    Mem_AddressType physAddr;
    if (MemAcc_MapAddress(addressAreaId, targetAddress, &instId, &physAddr) != E_OK) return E_NOT_OK;

    Std_ReturnType ret = Mem_Write(instId, physAddr, sourceDataPtr, length);
    if (ret == E_OK) {
        MemAcc_State[addressAreaId].Status = MEMACC_JOB_PENDING;
        MemAcc_State[addressAreaId].CurrentJob = MEMACC_WRITE_JOB;
    }
    return ret;
}

/* [SWS_MemAcc_10025] */
Std_ReturnType MemAcc_Erase(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType targetAddress, MemAcc_LengthType length)
{
    if (MemAcc_InitStatus == MEMACC_E_UNINIT) return E_NOT_OK;
    if (addressAreaId >= MEMACC_MAX_ADDRESS_AREAS) return E_NOT_OK;
    if (MemAcc_State[addressAreaId].Status == MEMACC_JOB_PENDING) return E_NOT_OK;

    Mem_InstanceIdType instId;
    Mem_AddressType physAddr;
    if (MemAcc_MapAddress(addressAreaId, targetAddress, &instId, &physAddr) != E_OK) return E_NOT_OK;

    Std_ReturnType ret = Mem_Erase(instId, physAddr, length);
    if (ret == E_OK) {
        MemAcc_State[addressAreaId].Status = MEMACC_JOB_PENDING;
        MemAcc_State[addressAreaId].CurrentJob = MEMACC_ERASE_JOB;
    }
    return ret;
}

/* [SWS_MemAcc_10018] */
void MemAcc_Cancel(MemAcc_AddressAreaIdType addressAreaId)
{
    if (addressAreaId < MEMACC_MAX_ADDRESS_AREAS && MemAcc_State[addressAreaId].Status == MEMACC_JOB_PENDING) {
        MemAcc_State[addressAreaId].Status = MEMACC_JOB_IDLE;
        MemAcc_State[addressAreaId].Result = MEMACC_CANCELED;
        MemAcc_State[addressAreaId].CurrentJob = MEMACC_NO_JOB;
        /* Nếu phần cứng có hỗ trợ hủy, gọi Mem_Cancel/Mem_Suspend ở đây (tùy hardware) */
    }
}

/* [SWS_MemAcc_10017] */
void MemAcc_MainFunction(void)
{
    if (MemAcc_InitStatus == MEMACC_E_UNINIT) return;

    uint8 i;
    for (i = 0; i < MEMACC_MAX_ADDRESS_AREAS; i++) {
        if (MemAcc_State[i].Status == MEMACC_JOB_PENDING) {
            /* Lấy kết quả từ Mem Driver để xem phần cứng làm xong chưa */
            Mem_InstanceIdType instId = MemAcc_AddressAreaConfig[i].MemInstanceId;
            MemAcc_MemJobResultType memResult = Mem_GetJobResult(instId);
            
            if (memResult == MEM_JOB_OK) {
                MemAcc_State[i].Status = MEMACC_JOB_IDLE;
                MemAcc_State[i].Result = MEMACC_OK;
                MemAcc_State[i].CurrentJob = MEMACC_NO_JOB;
                /* Gọi callback JobEndNotification tại đây nếu có cấu hình */
            } 
            else if (memResult == MEM_JOB_FAILED || memResult == MEM_INCONSISTENT) {
                MemAcc_State[i].Status = MEMACC_JOB_IDLE;
                MemAcc_State[i].Result = MEMACC_FAILED;
                if (memResult == MEM_INCONSISTENT) MemAcc_State[i].Result = MEMACC_INCONSISTENT;
                MemAcc_State[i].CurrentJob = MEMACC_NO_JOB;
            }
        }
    }
}
