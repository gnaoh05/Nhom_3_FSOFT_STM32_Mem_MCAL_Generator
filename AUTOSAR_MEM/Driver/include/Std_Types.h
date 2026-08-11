#ifndef STD_TYPES_H
#define STD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Kiểu số nguyên chuẩn
 ******************************************************************************/
#include "Platform_Types.h"
#include <stdbool.h>

/******************************************************************************
 * Phiên bản AUTOSAR
 ******************************************************************************/

#define STD_TYPES_AR_RELEASE_MAJOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_MINOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_REVISION_VERSION   0U

/******************************************************************************
 * Kiểu trả về chuẩn
 ******************************************************************************/

typedef uint8_t Std_ReturnType;

#define E_OK        ((Std_ReturnType)0U)
#define E_NOT_OK    ((Std_ReturnType)1U)

/******************************************************************************
 * Mức logic và trạng thái chuẩn
 ******************************************************************************/

#define STD_HIGH    0x01U
#define STD_LOW     0x00U
#define STD_ACTIVE  0x01U
#define STD_IDLE    0x00U
#define STD_ON      0x01U
#define STD_OFF     0x00U

/******************************************************************************
 * Kiểu Boolean
 ******************************************************************************/

#ifndef TRUE
#define TRUE    ((boolean)true)
#endif

#ifndef FALSE
#define FALSE   ((boolean)false)
#endif

typedef bool boolean;

/******************************************************************************
 * Null Pointer
 ******************************************************************************/

#ifndef NULL_PTR
#define NULL_PTR    ((void*)0)
#endif

/******************************************************************************
 * Thông tin phiên bản
 ******************************************************************************/

typedef struct
{
    uint16_t vendorID;
    uint16_t moduleID;
    uint8_t sw_major_version;
    uint8_t sw_minor_version;
    uint8_t sw_patch_version;
} Std_VersionInfoType;

#ifdef __cplusplus
}
#endif

#endif
