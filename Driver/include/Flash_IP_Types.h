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
    FLASH_IP_JOB_BUSY,             /* Trạng thái đang xử lý */
    FLASH_IP_JOB_FAILED,           /* Lỗi chung */
    FLASH_IP_INCONSISTENT,         /* Bổ sung: Lỗi Blank Check dữ liệu không nhất quán */
    FLASH_IP_WRITE_PROTECT_ERROR,  /* Lỗi bảo vệ ghi */
    FLASH_IP_ALIGNMENT_ERROR       /* Lỗi căn chỉnh địa chỉ/dữ liệu */
} Flash_IP_JobResultType;

#endif /* FLASH_IP_TYPES_H */