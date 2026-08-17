# Tệp Test Được Sinh

- `Mem_Test_Traceability.md`: ma trận coverage theo Requirement cho bộ test AUTOSAR MemoryDriver mức thanh ghi trên STM32, gồm Requirement được kiểm tra runtime, chỉ review và không áp dụng.
- `include/Mem_Cfg.h`, `include/Flash_IP_Cfg.h`: header cấu hình pre-compile do AUTOSAR Mem Configurator sinh.
- `src/Mem_Cfg.c`, `src/Flash_IP_Cfg.c`: dữ liệu cấu hình được STM32CubeIDE compile cùng integration test.

Sinh lại từ thư mục gốc repository:

    python AUTOSAR_MEM/Driver/Code_Generator/GUI/main_gui.py \
      --generate AUTOSAR_MEM/Driver/Code_Generator/GUI/examples/STM32F401.epc \
      --output AUTOSAR_MEM/Test/Generated_File
