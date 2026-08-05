#include <stdio.h>
#include "Mem.h"
#include "SchM_Mem.h"

int main() {
    printf("--- BAT DAU TEST AUTOSAR MEMORY DRIVER ---\n\n");

    /* 1. KHỞI TẠO */
    printf("1. Goi Mem_Init...\n");
    Mem_Init(NULL);
    printf("Trang thai Job hien tai: %d (0 = OK)\n\n", Mem_GetJobResult(0));

    /* 2. GỌI LỆNH ĐỌC BẤT ĐỒNG BỘ */
    printf("2. Goi Mem_Read...\n");
    Mem_DataType dummyBuffer[10];
    
    /* API trả về ngay lập tức, IPW chưa thực sự được gọi */
    Std_ReturnType status = Mem_Read(0, 0x08000000, dummyBuffer, 10);
    
    if (status == E_OK) {
        printf("Lenh Read duoc tiep nhan (Vao context queue)!\n");
        printf("Trang thai Job ngay luc nay: %d (1 = PENDING)\n\n", Mem_GetJobResult(0));
    }

    /* 3. MÔ PHỎNG SCHEDULER CỦA HỆ ĐIỀU HÀNH */
    printf("3. Mo phong he dieu hanh goi Mem_MainFunction lien tuc...\n");
    int tick = 0;
    while (Mem_GetJobResult(0) == MEM_JOB_PENDING) {
        printf("--- Tick %d ---\n", ++tick);
        Mem_MainFunction(); /* Quét trạng thái, IPW thực sự bắt đầu ở Tick 1 */
    }

    printf("\n4. TEST THANH CONG!\n");
    return 0;
}