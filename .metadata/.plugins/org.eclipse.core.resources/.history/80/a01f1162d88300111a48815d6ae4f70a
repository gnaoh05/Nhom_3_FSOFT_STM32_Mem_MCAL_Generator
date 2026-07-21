#ifndef STD_TYPES_H
#define STD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Standard Integer Types
 ******************************************************************************/
#include "Platform_Types.h"
#include <stdbool.h>

/******************************************************************************
 * AUTOSAR Version
 ******************************************************************************/

#define STD_TYPES_AR_RELEASE_MAJOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_MINOR_VERSION      4U
#define STD_TYPES_AR_RELEASE_REVISION_VERSION   0U

/******************************************************************************
 * Standard Return Type
 ******************************************************************************/

typedef uint8_t Std_ReturnType;

#define E_OK        ((Std_ReturnType)0U)
#define E_NOT_OK    ((Std_ReturnType)1U)

/******************************************************************************
 * Boolean
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
 * Version Info
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
