#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Mem_IPW_Types.h"
#include "Flash_IP.h"
#include "Std_Types.h"

/**
 * @brief  Khởi tạo tầng IP Wrapper và Driver Flash phần cứng bên dưới
 */
void Mem_IPW_Init(void);

/**
 * @brief  Đọc dữ liệu từ bộ nhớ Flash thông qua Flash_IP
 * @param  address   Địa chỉ vật lý bắt đầu đọc
 * @param  targetPtr Con trỏ đệm chứa dữ liệu đầu ra
 * @param  length    Số lượng Byte cần đọc
 * @return Mem_IPW_JobResultType Kết quả thực thi
 */
Mem_IPW_JobResultType Mem_IPW_Read(uint32 address, uint8 *targetPtr, uint32 length);

/**
 * @brief  Ghi dữ liệu vào bộ nhớ Flash thông qua Flash_IP
 * @param  address   Địa chỉ vật lý bắt đầu ghi
 * @param  sourcePtr Con trỏ chứa dữ liệu cần ghi
 * @param  length    Số lượng Byte cần ghi
 * @return Mem_IPW_JobResultType Kết quả thực thi
 */
Mem_IPW_JobResultType Mem_IPW_Write(uint32 address, const uint8 *sourcePtr, uint32 length);

/**
 * @brief  Xóa Sector bộ nhớ Flash thông qua Flash_IP
 * @param  sectorNum Chỉ số Sector cần xóa
 * @return Mem_IPW_JobResultType Kết quả thực thi
 */
Mem_IPW_JobResultType Mem_IPW_Erase(uint8 sectorNum);

#endif /* MEM_IPW_H */