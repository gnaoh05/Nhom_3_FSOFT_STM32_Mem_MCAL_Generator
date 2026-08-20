#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include "Std_Types.h"
#include "MemAcc_GeneralTypes.h"

/* --- [SWS_Mem_10002 -> 10026] Định nghĩa kiểu đặc thù của Driver --- */
typedef MemAcc_AddressType Mem_AddressType;     /* Địa chỉ vật lý */
typedef uint8 Mem_DataType;                     /* Kiểu dữ liệu mảng Buffer */
typedef uint32 Mem_InstanceIdType;              /* Định danh thiết bị (Instance) */
typedef uint32 Mem_LengthType;                  /* Độ dài dữ liệu (Bytes) */
typedef uint32 Mem_HwServiceIdType;             /* ID dịch vụ phần cứng đặc thù */

/* Trạng thái máy trạng thái chung của toàn module */
#define MEM_UNINIT (0x00u) /* Chưa khởi tạo */
#define MEM_INIT   (0x01u) /* Đã khởi tạo */

/* [SWS_Mem_00070] Mã lỗi khi gọi hàm phần cứng không được hỗ trợ */
#define E_MEM_SERVICE_NOT_AVAIL ((Std_ReturnType)0x02u)

/* Cấu trúc mô tả 1 Sector Batch (Nhóm các sector có cùng kích thước liên kề nhau) */
typedef struct {
    uint32  MemStartAddress;         /* Địa chỉ bắt đầu của Sector Batch */
    uint32  MemNumberOfSectors;      /* Số lượng Sector trong Batch này */
    uint32  MemEraseSectorSize;      /* Kích thước của 1 Sector (vd: 16KB, 64KB) */
    uint32  MemMinReadSize;          /* Kích thước đọc nhỏ nhất (1 byte) */
    uint32  MemWritePageSize;        /* Kích thước ghi nhỏ nhất */
    uint32  MemSpecifiedEraseCycles; /* Số chu kỳ xóa tối đa của phần cứng */
} Mem_SectorBatchConfigType;

/* Cấu trúc tham số cấu hình cho TỪNG THIẾT BỊ (Instance) */
typedef struct {
    uint8                             MemInstanceId;      /* ID thiết bị (0) */
    uint16                            MemNumberOfBatches; /* Số lượng Batch */
    const Mem_SectorBatchConfigType*  MemSectorBatches;   /* Con trỏ trỏ tới mảng cấu hình Sector */
} Mem_InstanceConfigType;

/* [SWS_Mem_10000] Cấu trúc tham số cấu hình TỔNG THỂ của toàn bộ Mem driver */
typedef struct {
    const Mem_InstanceConfigType* MemInstances; /* Mảng cấu hình các Instance */

} Mem_ConfigType;

#endif /* MEM_TYPES_H */
