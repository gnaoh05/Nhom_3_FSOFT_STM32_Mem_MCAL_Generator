#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Mem_Types.h"

/* --- Định nghĩa trạng thái riêng của phần cứng (IPW) --- */
typedef enum {
    MEM_IPW_IDLE = 0, /* Phần cứng đang rảnh */
    MEM_IPW_BUSY,     /* Phần cứng đang bận ghi/xóa/đọc */
    MEM_IPW_ERROR     /* Phần cứng gặp lỗi */
} Mem_Ipw_StatusType;

/* Hàm lấy trạng thái phần cứng (Dùng cho giao thức Handshake) */
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType instanceId);
/* ----------------------------------------------------------------- */

/* Các nguyên mẫu hàm gọi từ tầng Core xuống */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);

/* [SWS_Mem_00061, SWS_Mem_00079] Hàm hủy tiến trình phần cứng khẩn cấp */
void Mem_Ipw_Cancel(Mem_InstanceIdType instanceId);

/* Hàm mô phỏng phần cứng thực thi ngầm */
void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId);

#endif /* MEM_IPW_H */