#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Flash_IP_Cfg.h"
#include "Flash_IP_Types.h"
#include "Std_Types.h"

/* Quản lý trạng thái khởi tạo và khóa/mở khóa Flash */
void Flash_IP_Init(void);
void Flash_IP_DeInit(void);

void Flash_IP_Unlock(void);
void Flash_IP_Lock(void);

/* API kiểm tra trạng thái phần cứng và tự động dọn lỗi */
Flash_IP_JobResultType Flash_IP_GetStatus(void);

/* Các hàm thực thi kích hoạt tác vụ Flash phần cứng */
Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum);
Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length);
Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length);
Flash_IP_JobResultType Flash_IP_BlankCheck(uint32 address, uint32 length);

/* Hàm hỗ trợ map địa chỉ vật lý ra Sector ID */
uint8 Flash_IP_GetSectorFromAddress(uint32 address);

#endif /* FLASH_IP_H */