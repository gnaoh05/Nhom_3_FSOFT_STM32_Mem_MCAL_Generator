#ifndef MEMACC_H
#define MEMACC_H

#include "MemAcc_GeneralTypes.h"
#include "MemAcc_Cfg.h"

/* [SWS_MemAcc_00001] Module ID chuẩn (41 = Memory Access) */
#define MEMACC_MODULE_ID 41u

/* --- Định danh API (Service IDs) dùng cho module DET --- */
#define MEMACC_INIT_ID                  0x01u
#define MEMACC_GETVERSIONINFO_ID        0x02u
#define MEMACC_MAINFUNCTION_ID          0x03u
#define MEMACC_CANCEL_ID                0x04u
#define MEMACC_GETJOBRESULT_ID          0x05u
#define MEMACC_GETMEMORYINFO_ID         0x06u
#define MEMACC_GETPROCESSEDLENGTH_ID    0x07u
#define MEMACC_GETJOBINFO_ID            0x08u
#define MEMACC_READ_ID                  0x09u
#define MEMACC_WRITE_ID                 0x0Au
#define MEMACC_ERASE_ID                 0x0Bu
#define MEMACC_COMPARE_ID               0x0Cu
#define MEMACC_BLANKCHECK_ID            0x0Du
#define MEMACC_HWSPECIFICSERVICE_ID     0x0Eu
#define MEMACC_GETJOBSTATUS_ID          0x10u

/* --- [SWS_MemAcc_10038] Mã lỗi báo cáo cho DET (Error Codes) --- */
#define MEMACC_E_UNINIT                       0x01u
#define MEMACC_E_PARAM_POINTER                0x02u
#define MEMACC_E_PARAM_ADDRESS_AREA_ID        0x03u
#define MEMACC_E_PARAM_ADDRESS_LENGTH         0x04u
#define MEMACC_E_PARAM_HW_ID                  0x05u
#define MEMACC_E_BUSY                         0x06u
#define MEMACC_E_MEM_INIT_FAILED              0x07u

/* --- Cấu trúc cấu hình MemAcc_ConfigType --- */
/* (Thường là một cấu trúc rỗng đối với VARIANT-PRE-COMPILE nếu tất cả thông số đều cấu hình tĩnh) */
typedef struct {
    uint8 Dummy;
} MemAcc_ConfigType;

/* --- HÀM ĐỒNG BỘ (Thực thi & trả kết quả ngay) --- */
void MemAcc_Init(const MemAcc_ConfigType* configPtr);
void MemAcc_DeInit(void);
void MemAcc_GetVersionInfo(Std_VersionInfoType* versionInfoPtr);

MemAcc_JobResultType MemAcc_GetJobResult(MemAcc_AddressAreaIdType addressAreaId);
MemAcc_JobStatusType MemAcc_GetJobStatus(MemAcc_AddressAreaIdType addressAreaId);

/* --- HÀM BẤT ĐỒNG BỘ (Đẩy lệnh vào hàng đợi, xử lý tại MainFunction) --- */
Std_ReturnType MemAcc_Read(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType sourceAddress, MemAcc_DataType* destinationDataPtr, MemAcc_LengthType length);
Std_ReturnType MemAcc_Write(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType targetAddress, const MemAcc_DataType* sourceDataPtr, MemAcc_LengthType length);
Std_ReturnType MemAcc_Erase(MemAcc_AddressAreaIdType addressAreaId, MemAcc_AddressType targetAddress, MemAcc_LengthType length);

/* Hàm hủy job */
void MemAcc_Cancel(MemAcc_AddressAreaIdType addressAreaId);

#endif /* MEMACC_H */
