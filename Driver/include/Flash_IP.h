#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Flash_IP_Cfg.h"

/* Định nghĩa trạng thái hoạt động thuần túy phần cứng */
typedef enum {
    FLASH_IP_UNINITIALIZED = 0x00,
    FLASH_IP_INITIALIZED   = 0x01
} Flash_IP_StatusType;

/* Kết quả thực thi tác vụ phần cứng vật lý */
typedef enum {
    FLASH_IP_JOB_OK               = 0x00, /* Thao tác thành công */
    FLASH_IP_JOB_FAILED           = 0x01, /* Lỗi phần cứng chung */
    FLASH_IP_WRITE_PROTECT_ERROR  = 0x02, /* Lỗi phân vùng bị khóa chống ghi */
    FLASH_IP_ALIGNMENT_ERROR      = 0x03  /* Lỗi địa chỉ không căn lề Word */
} Flash_IP_JobResultType;

/* Khai báo các API lớp IP - Không chứa bất kỳ kiểu dữ liệu nào của tầng trên */
void Flash_IP_Init(void);
void Flash_IP_DeInit(void);
Flash_IP_JobResultType Flash_IP_Read(uint32_t address, uint8_t *targetPtr, uint32_t length);
Flash_IP_JobResultType Flash_IP_Write(uint32_t address, const uint8_t *sourcePtr, uint32_t length);
Flash_IP_JobResultType Flash_IP_Erase(uint8_t sectorNum);

#endif /* FLASH_IP_H */