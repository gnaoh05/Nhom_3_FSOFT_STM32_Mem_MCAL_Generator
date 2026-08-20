#ifndef STD_TYPES_H
#define STD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Platform_Types.h"
#include <stdbool.h>

#ifndef __IO
#define __IO volatile
#endif

#define STD_TYPES_AR_RELEASE_MAJOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_MINOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_REVISION_VERSION   0U

typedef uint8 Std_ReturnType;

#define E_OK        ((Std_ReturnType)0U)
#define E_NOT_OK    ((Std_ReturnType)1U)

#define STD_HIGH    0x01U
#define STD_LOW     0x00U
#define STD_ACTIVE  0x01U
#define STD_IDLE    0x00U
#define STD_ON      0x01U
#define STD_OFF     0x00U

typedef bool boolean;

#ifndef TRUE
#define TRUE        ((boolean)true)
#endif

#ifndef FALSE
#define FALSE       ((boolean)false)
#endif

#ifndef NULL_PTR
#define NULL_PTR    ((void*)0)
#endif

#ifndef NULL
#define NULL        ((void*)0)
#endif

typedef struct
{
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

#ifdef __cplusplus
}
#endif

#endif /* STD_TYPES_H */
