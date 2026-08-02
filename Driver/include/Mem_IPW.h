#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Mem_Types.h"

/* Các nguyên mẫu hàm gọi từ tầng Core xuống */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType instanceId, Mem_AddressType sourceAddress, Mem_DataType* destinationDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, const Mem_DataType* sourceDataPtr, Mem_LengthType length);
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType instanceId, Mem_AddressType targetAddress, Mem_LengthType length);

/* Hàm mô phỏng phần cứng thực thi ngầm */
void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId);

#endif /* MEM_IPW_H */