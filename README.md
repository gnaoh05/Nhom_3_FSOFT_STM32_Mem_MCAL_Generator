# AUTOSAR CP MEMORY DRIVER (MCAL) GENERATOR & RUNTIME INTEGRATION

Hệ thống sinh cấu hình (MCAL Configuration Generator) và Khung kiểm thử toàn diện cho **AUTOSAR CP Memory Driver (Mem)** trên vi điều khiển **STM32F401RE**.

Xem tài liệu kỹ thuật chi tiết tại: [AUTOSAR_MEM/README.md](AUTOSAR_MEM/README.md)

---

## TỔNG QUAN HỆ THỐNG

1. **MCAL Configuration Generator (Python GUI)**:
   - Đường dẫn: `AUTOSAR_MEM/Driver/Code_Generator/GUI/main_gui.py`.
   - Sinh các tệp cấu hình `Mem_Cfg.h`, `Mem_Cfg.c`, `Flash_IP_Cfg.h`, `Flash_IP_Cfg.c`.
   - Tích hợp Parameter Range Validation theo tiêu chuẩn AUTOSAR SWS Memory Driver.

2. **Kiến trúc Driver 3 Tầng chuẩn AUTOSAR**:
   - `Mem.c`: AUTOSAR Core Layer điều phối Job Context và Job State Machine bất đồng bộ.
   - `Mem_IPW.c`: IP Wrapper Layer phân giải địa chỉ tuyến tính thành Physical Sector ID.
   - `Flash_IP.c`: Hardware IP Layer điều khiển trực tiếp thanh ghi Flash STM32 (`Flash_IP_HwAccess.h`).

3. **Khung Kiểm thử 54 Testcases & Interactive UART CLI**:
   - 54 Runtime Testcases bao phủ 9 nhóm: `INIT`, `DET`, `STATUS`, `READ`, `WRITE`, `ERASE`, `BLANKCHECK`, `JOB`, `BOUNDARY`.
   - Interactive Manual CLI (Phím `M`) hỗ trợ Hex Dump 16 cột + ASCII View và Read-Back Verification.
   - Hardware Memory Inspection qua STM32CubeProgrammer (HotPlug mode).
