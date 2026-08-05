#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h> /* Hỗ trợ macro NULL */

/* Kiểu dữ liệu cơ sở chuẩn AUTOSAR */
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

/* Kiểu trả về tiêu chuẩn cho các API */
typedef uint8 Std_ReturnType;

#define E_OK       ((Std_ReturnType)0x00u) /* Thành công */
#define E_NOT_OK   ((Std_ReturnType)0x01u) /* Thất bại chung */

/* Macro trạng thái Boolean */
#define STD_ON     1u
#define STD_OFF    0u

/* [SRS_BSW_00003] Cấu trúc thông tin phiên bản module */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

#endif /* STD_TYPES_H */