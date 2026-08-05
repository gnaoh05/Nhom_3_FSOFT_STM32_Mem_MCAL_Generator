#include "Mem_IPW.h"
#include "Mem_Cfg.h"
#include <stdio.h> 

/* Biến theo dõi trạng thái và thời gian trễ của phần cứng */
static Mem_Ipw_StatusType mock_status[MEM_MAX_INSTANCES] = {MEM_IPW_IDLE};
static uint8 mock_delay[MEM_MAX_INSTANCES] = {0};

/* Triển khai API lấy trạng thái cho tầng Core gọi */
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType instanceId) {
    return mock_status[instanceId];
}

Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length) {
    (void)destinationDataPtr; 
    printf("[IPW] Thuc thi READ chunk tai 0x%08X, do dai %d bytes\n", sourceAddress, length);
    mock_status[instanceId] = MEM_IPW_BUSY; /* Bật cờ bận */
    mock_delay[instanceId] = 1; /* Cần 1 chu kỳ để đọc xong 1 chunk */
    return E_OK; 
}

Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length) {
    (void)sourceDataPtr; 
    printf("[IPW] Thuc thi WRITE chunk tai 0x%08X, do dai %d bytes\n", targetAddress, length);
    mock_status[instanceId] = MEM_IPW_BUSY;
    mock_delay[instanceId] = 2; /* Ghi tốn nhiều thời gian hơn (2 chu kỳ) */
    return E_OK;
}

Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
    (void)targetAddress; (void)length; 
    printf("[IPW] Thuc thi ERASE sector tai 0x%08X\n", targetAddress);
    mock_status[instanceId] = MEM_IPW_BUSY;
    mock_delay[instanceId] = 3; /* Xóa tốn thời gian nhất (3 chu kỳ) */
    return E_OK;
}

Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length) {
    (void)targetAddress; (void)length; 
    mock_status[instanceId] = MEM_IPW_BUSY;
    mock_delay[instanceId] = 1;
    return E_OK;
}

void Mem_Ipw_Cancel(Mem_InstanceIdType instanceId) {
    mock_status[instanceId] = MEM_IPW_IDLE; /* Nhả cờ phần cứng ngay lập tức */
    mock_delay[instanceId] = 0; 
    printf("[IPW] Tien trinh phan cung bi HUY cho Instance %d!\n", instanceId);
}

void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId) {
    if (mock_status[instanceId] == MEM_IPW_BUSY) {
        if (mock_delay[instanceId] > 0) {
            mock_delay[instanceId]--;
        } 
        /* Nếu thời gian trễ đã hết -> Phần cứng nhả cờ rảnh */
        if (mock_delay[instanceId] == 0) {
            mock_status[instanceId] = MEM_IPW_IDLE; 
        }
    }
}