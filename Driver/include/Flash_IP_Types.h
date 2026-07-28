#ifndef FLASH_IP_TYPES_H
#define FLASH_IP_TYPES_H

#include "Std_Types.h"

/* Định nghĩa trạng thái hoạt động thuần túy phần cứng */
typedef enum {
    FLASH_IP_UNINITIALIZED = 0x00U,
    FLASH_IP_INITIALIZED   = 0x01U
} Flash_IP_StatusType;

/* Kết quả thực thi tác vụ phần cứng vật lý */
typedef enum {
    FLASH_IP_JOB_OK            = 0x00U, /* Thao tác thành công */
    FLASH_IP_JOB_FAILED        = 0x01U, /* Lỗi phần cứng chung / Timeout */
    FLASH_IP_WRITE_PROTECT_ERROR = 0x02U, /* Lỗi phân vùng bị khóa chống ghi */
    FLASH_IP_ALIGNMENT_ERROR   = 0x03U  /* Lỗi địa chỉ không căn lề Word / Lỗi nạp phần cứng */
} Flash_IP_JobResultType;

#endif /* FLASH_IP_TYPES_H */