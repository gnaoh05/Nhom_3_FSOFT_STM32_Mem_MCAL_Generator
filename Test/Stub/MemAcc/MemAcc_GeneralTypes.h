#ifndef MEMACC_GENERALTYPES_H
#define MEMACC_GENERALTYPES_H

#include "Std_Types.h"

/* [SWS_Mem_10002] Định dạng địa chỉ vật lý (kế thừa từ tầng MemAcc) */
typedef uint32 MemAcc_AddressType; 

/* Trạng thái kết quả xử lý của một Job (Đọc, Ghi, Xóa, BlankCheck) */
typedef enum {
    MEM_JOB_OK,           /* Hoàn thành tốt */
    MEM_JOB_PENDING,      /* Đang xử lý ngầm */
    MEM_JOB_FAILED,       /* Lỗi phần cứng/logic */
    MEM_INCONSISTENT,     /* Dữ liệu không nhất quán (Vd: Xóa chưa sạch) */
    MEM_ECC_CORRECTED,    /* Đã tự động sửa lỗi ECC */
    MEM_ECC_UNCORRECTED   /* Lỗi ECC nghiêm trọng, không thể sửa */
} MemAcc_MemJobResultType;

#endif /* MEMACC_GENERALTYPES_H */