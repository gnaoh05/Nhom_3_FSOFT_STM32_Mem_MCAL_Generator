#include "Det.h"
#include <stdio.h>

/* Hàm thực thi giả lập (Stub) của DET để in log lỗi ra Console */
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    printf("[DET ERROR] Module: %d | Instance: %d | API ID: 0x%02X | Error Code: 0x%02X\n", 
           ModuleId, InstanceId, ApiId, ErrorId);
}