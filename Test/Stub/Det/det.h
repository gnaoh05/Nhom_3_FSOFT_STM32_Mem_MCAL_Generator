#ifndef DET_H
#define DET_H

#include "Std_Types.h"

/* API báo lỗi cho module Development Error Tracer (DET) */
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

#endif /* DET_H */