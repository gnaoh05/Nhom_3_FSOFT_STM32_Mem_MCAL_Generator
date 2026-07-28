#ifndef MEM_IPW_TYPES_H
#define MEM_IPW_TYPES_H

#include "Std_Types.h"

/* Kết quả thực thi công việc của tầng Wrapper */
typedef enum {
    MEM_IPW_JOB_OK            = 0x00U,  /* Tác vụ thành công */
    MEM_IPW_JOB_FAILED        = 0x01U,  /* Lỗi thực thi / Timeout */
    MEM_IPW_WRITE_PROTECT_ERR = 0x02U,  /* Lỗi phân vùng chống ghi */
    MEM_IPW_ALIGNMENT_ERR     = 0x03U   /* Lỗi địa chỉ/độ dài không căn lề */
} Mem_IPW_JobResultType;

#endif /* MEM_IPW_TYPES_H */