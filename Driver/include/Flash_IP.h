#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Flash_IP_Cfg.h"
#include "Flash_IP_Types.h"
#include "Std_Types.h"

/**
 * @brief  Khởi tạo phần cứng Flash IP (Cấu hình Latency ACR & Prefetch/Cache)
 */
void Flash_IP_Init(void);

/**
 * @brief  Hủy khởi tạo (Khóa thanh ghi Flash & Reset ACR về mặc định)
 */
void Flash_IP_DeInit(void);

/**
 * @brief  Đọc chuỗi Byte từ địa chỉ bộ nhớ Flash vật lý
 * @param  address   Địa chỉ Flash cần đọc
 * @param  targetPtr Con trỏ chứa dữ liệu đọc ra
 * @param  length    Số lượng Byte cần đọc
 * @return Flash_IP_JobResultType Kết quả thực thi
 */
Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length);

/**
 * @brief  Ghi dữ liệu vào bộ nhớ Flash vật lý (Xử lý căn lề Word & byte dư)
 * @param  address   Địa chỉ Flash cần ghi (Yêu cầu căn lề 4-byte)
 * @param  sourcePtr Con trỏ chứa dữ liệu cần ghi
 * @param  length    Số lượng Byte cần ghi
 * @return Flash_IP_JobResultType Kết quả thực thi
 */
Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length);

/**
 * @brief  Xóa một Sector bộ nhớ Flash
 * @param  sectorNum Chỉ số Sector cần xóa (0 -> 7)
 * @return Flash_IP_JobResultType Kết quả thực thi
 */
Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum);

#endif /* FLASH_IP_H */