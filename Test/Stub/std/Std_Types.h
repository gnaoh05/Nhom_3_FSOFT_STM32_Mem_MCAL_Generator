#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifndef STATUSTYPE_DEFINED
#define STATUSTYPE_DEFINED
#define E_OK                    ((Std_ReturnType)0x00U)
#endif

#define E_NOT_OK                ((Std_ReturnType)0x01U)

/* Mức điện áp logic chuẩn */
#define STD_LOW                 (0x00U)  /* Mức logic 0 (0V) */
#define STD_HIGH                (0x01U)  /* Mức logic 1 (3.3V/5V) */

/* Trạng thái kích hoạt (Active States) */
#define STD_IDLE                (0x00U)  /* Trạng thái rảnh */
#define STD_ACTIVE              (0x01U)  /* Trạng thái đang hoạt động */

/* Bật/Tắt tính năng (Enable/Disable) */
#define STD_OFF                 (0x00U)
#define STD_ON                  (0x01U)

/* Định nghĩa con trỏ NULL an toàn */
#ifndef NULL_PTR
#define NULL_PTR                ((void *)0)
#endif

/* Kiểu dữ liệu nguyên cơ bản AUTOSAR */
typedef uint8_t                 uint8;
typedef uint16_t                uint16;
typedef uint32_t                uint32;
typedef uint64_t                uint64;

typedef int8_t                  sint8;
typedef int16_t                 sint16;
typedef int32_t                 sint32;
typedef int64_t                 sint64;

typedef float                   float32;
typedef double                  float64;

typedef uint8_t                 boolean;

/* Kiểu dữ liệu trả về chuẩn cho các hàm API AUTOSAR */
typedef uint8_t                 Std_ReturnType;

/* Cấu trúc lưu trữ thông tin phiên bản của Module MCAL */
typedef struct {
    uint16 vendorID;           /* ID của nhà sản xuất (VD: FPT Software / ST) */
    uint16 moduleID;           /* ID của Module (VD: Mem = 91, Flash = 92) */
    uint8  sw_major_version;   /* Phiên bản phần mềm Major */
    uint8  sw_minor_version;   /* Phiên bản phần mềm Minor */
    uint8  sw_patch_version;   /* Phiên bản phần mềm Patch */
} Std_VersionInfoType;

/* Macro volatile truy xuất thanh ghi cứng */
#ifndef __IO
#define __IO                    volatile
#endif

#endif /* STD_TYPES_H */