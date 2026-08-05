#include "Det.h"
#include <stdio.h>

/* Stub DET: In lỗi ra console để debug trên PC */
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    printf("[DET] ModuleId=%u, InstanceId=%u, ApiId=0x%02X, ErrorId=0x%02X\n",
           ModuleId, InstanceId, ApiId, ErrorId);
}
