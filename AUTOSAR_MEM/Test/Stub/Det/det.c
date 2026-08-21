#include "det.h"
volatile Det_LastErrorType Det_LastError =
{
    0u,
    0u,
    0u,
    0u,
    FALSE
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
    Det_LastError.Valid      = TRUE;

    return E_OK;
}

void Det_ResetLastError(void)
{
    Det_LastError.ModuleId   = 0u;
    Det_LastError.InstanceId = 0u;
    Det_LastError.ApiId      = 0u;
    Det_LastError.ErrorId    = 0u;
    Det_LastError.Valid      = FALSE;
}

Det_LastErrorType Det_GetLastError(void)
{
    Det_LastErrorType lastError;

    lastError.ModuleId   = Det_LastError.ModuleId;
    lastError.InstanceId = Det_LastError.InstanceId;
    lastError.ApiId      = Det_LastError.ApiId;
    lastError.ErrorId    = Det_LastError.ErrorId;
    lastError.Valid      = Det_LastError.Valid;

    return lastError;
}

boolean Det_HasError(void)
{
    return Det_LastError.Valid;
}
