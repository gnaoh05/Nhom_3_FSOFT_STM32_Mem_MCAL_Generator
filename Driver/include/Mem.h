#ifndef MEM_H
#define MEM_H

#include "Mem_Types.h"
#include "Mem_Cfg.h"

/* [SWS_Mem_10009] Module ID chuẩn (91 = Memory Driver) */
#define MEM_MODULE_ID 91u

/* --- Định danh API (Service IDs) dùng cho module DET --- */
#define MEM_INIT_ID              0x01u
#define MEM_GETVERSIONINFO_ID    0x02u
#define MEM_MAINFUNCTION_ID      0x03u
#define MEM_GETJOBRESULT_ID      0x04u
#define MEM_READ_ID              0x05u
#define MEM_WRITE_ID             0x06u
#define MEM_ERASE_ID             0x07u
#define MEM_PROPAGATEERROR_ID    0x08u
#define MEM_BLANKCHECK_ID        0x09u
#define MEM_HWSPECIFICSERVICE_ID 0x0Au
#define MEM_DEINIT_ID            0x0Bu
#define MEM_SUSPEND_ID           0x0Cu
#define MEM_RESUME_ID            0x0Du

/* --- [SWS_Mem_00052] Mã lỗi báo cáo cho DET (Error Codes) --- */
#define MEM_E_UNINIT             0x01u /* API được gọi khi module chưa INIT */
#define MEM_E_PARAM_POINTER      0x02u /* Lỗi con trỏ dữ liệu NULL */
#define MEM_E_PARAM_ADDRESS      0x03u /* Lỗi địa chỉ (Target/Source) không hợp lệ */
#define MEM_E_PARAM_LENGTH       0x04u /* Độ dài thao tác = 0 hoặc sai kích thước sector/page */
#define MEM_E_PARAM_INSTANCE_ID  0x05u /* Instance ID vượt quá MEM_MAX_INSTANCES */
#define MEM_E_JOB_PENDING        0x06u /* Thiết bị đang bận xử lý một Job khác */

/* --- HÀM ĐỒNG BỘ (Thực thi & trả kết quả ngay) --- */
void Mem_Init(const Mem_ConfigType* configPtr);
void Mem_DeInit(void);
void Mem_GetVersionInfo(Std_VersionInfoType* versionInfoPtr);
MemAcc_MemJobResultType Mem_GetJobResult(Mem_InstanceIdType instanceId);
Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId);
Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId);
void Mem_PropagateError(Mem_InstanceIdType instanceId);

/* --- HÀM BẤT ĐỒNG BỘ (Đẩy lệnh vào hàng đợi, xử lý tại MainFunction) --- */
Std_ReturnType Mem_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);
Std_ReturnType Mem_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);
Std_ReturnType Mem_HwSpecificService(Mem_InstanceIdType instanceId, Mem_HwServiceIdType hwServiceId, Mem_DataType* dataPtr, Mem_LengthType* lengthPtr);

#endif /* MEM_H */