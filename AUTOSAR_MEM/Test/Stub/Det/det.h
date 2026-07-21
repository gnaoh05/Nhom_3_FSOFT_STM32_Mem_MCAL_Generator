#ifndef DET_H
#define DET_H

#include "Std_Types.h"

/* Module ID */
#define DET_MODULE_ID               (15u)

/* API IDs */
#define DET_SID_REPORT_ERROR        (0x01u)
#define DET_SID_START               (0x02u)

/* Return type */
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8  InstanceId,
    uint8  ApiId,
    uint8  ErrorId
);

#endif
