#ifndef FLASH_IP_TYPES_H
#define FLASH_IP_TYPES_H

#include "Std_Types.h"
/* Định nghĩa trạng thái hoạt động thuần túy phần cứng */
typedef enum 
{
    FLASH_IP_UNINITIALIZED = 0x00U,
    FLASH_IP_INITIALIZED   = 0x01U
} Flash_IP_StatusType;

/* Kết quả thực thi tác vụ phần cứng vật lý */
typedef enum 
{
    FLASH_IP_JOB_OK = 0U,
    FLASH_IP_JOB_BUSY,             /* <-- Bổ sung dòng này */
    FLASH_IP_JOB_FAILED,
    FLASH_IP_WRITE_PROTECT_ERROR,
    FLASH_IP_ALIGNMENT_ERROR
} Flash_IP_JobResultType;

#endif /* FLASH_IP_TYPES_H */