#include "Det.h"

/*======================================================================================
 * Development Error Tracer
 *
 * This is a lightweight DET implementation intended for AUTOSAR Memory Driver
 * integration on STM32F401RE.
 *
 * The implementation stores the last reported error, allowing the debugger or
 * application to inspect development errors during integration testing.
 *=====================================================================================*/

typedef struct
{
    uint16 ModuleId;
    uint8  InstanceId;
    uint8  ApiId;
    uint8  ErrorId;
} Det_LastErrorType;

static Det_LastErrorType Det_LastError =
{
    0u,
    0u,
    0u,
    0u
};

Std_ReturnType Det_ReportError
(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
)
{
    Det_LastError.ModuleId   = ModuleId;
    Det_LastError.InstanceId = InstanceId;
    Det_LastError.ApiId      = ApiId;
    Det_LastError.ErrorId    = ErrorId;

    return E_OK;
}
