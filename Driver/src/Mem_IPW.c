#include "Mem_IPW.h"
#include "Mem_Cfg.h"   /* Cần MEM_MAX_INSTANCES */
#include <stdio.h>     /* Chỉ dùng để in log test trên PC */

/* Biến giả lập độ trễ phần cứng (để test cờ PENDING) */
/* Không dùng static vì Mem.c cần truy cập qua extern */
uint8 mock_delay[MEM_MAX_INSTANCES] = {0};

/* Mô phỏng hàm Read */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length) {
    printf("[IPW] Da nhan lenh READ tai dia chi 0x%08X, do dai %d bytes\n", sourceAddress, length);
    
    /* Mô phỏng phần cứng cần 3 chu kỳ MainFunction mới đọc xong */
    mock_delay[instanceId] = 3; 
    
    return E_OK; /* Trả về E_OK nghĩa là IPW đã tiếp nhận lệnh thành công */
}

/* Các hàm Write, Erase làm tương tự (Trả về E_OK) */
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length) {
    return E_OK;
}
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
    return E_OK;
}
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
    return E_OK;
}

/* Hàm giả lập ngắt/xử lý ngầm của IPW */
void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId) {
    if (mock_delay[instanceId] > 0) {
        mock_delay[instanceId]--;
        printf("[IPW] Phan cung dang xu ly... (Delay con %d)\n", mock_delay[instanceId]);
    }
}