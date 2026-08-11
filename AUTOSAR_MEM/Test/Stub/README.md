# AUTOSAR integration stubs

Thư mục này chứa các interface tối thiểu dùng để build và integration-test
Memory Driver khi project chưa có đầy đủ AUTOSAR BSW stack.

| Stub | Vai trò trong integration test |
| --- | --- |
| `std/Std_Types.h` | Thay thế module kiểu dữ liệu chuẩn AUTOSAR. |
| `Det/det.h`, `Det/det.c` | Ghi nhận development error gần nhất để testcase kiểm tra. |
| `MemAcc/MemAcc_GeneralTypes.h` | Cung cấp các type mà `Mem` bắt buộc import từ MemAcc. |
| `SchM/SchM_Mem.h` | Cung cấp interface scheduled function của Mem cho scheduler. |

Các file này không phải output của code generator và không phải hiện thực production
đầy đủ. Khi tích hợp với một AUTOSAR stack thực, thay các include path Stub bằng
header/module tương ứng do stack cung cấp; không copy implementation Stub vào target
production.

Trong STM32CubeIDE, include path của Stub phải đứng trước `Driver/include` để các
dependency giả lập được chọn một cách nhất quán trong cấu hình Debug và Release.
