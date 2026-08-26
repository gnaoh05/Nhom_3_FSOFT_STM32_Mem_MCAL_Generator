# Debug bộ test AUTOSAR MEM

## UART khi debugger kết nối

Việc bật/tắt UART không phụ thuộc cấu hình build **Debug/Release**. Khi firmware
khởi động, TestManager đọc bit `C_DEBUGEN` trong thanh ghi `DHCSR` của Cortex-M.
Nút **Run** cũng dùng GDB server trong thời gian ngắn để nạp ELF, vì vậy
TestManager chờ GDB server ngắt kết nối trước khi phân loại chế độ chạy:

- nhấn **Run** hoặc reset/chạy bình thường: GDB server ngắt kết nối, USART2 và
  menu UART hoạt động ở 115200 baud;
- nhấn biểu tượng con bọ và debugger kết nối: TestManager không khởi tạo USART2,
  bỏ qua menu UART và chạy nhóm testcase được chọn bởi `ACTIVE_TEST_GROUP`.

Vì vậy cùng một file ELF build bằng cấu hình Debug vẫn có UART khi được nạp/chạy
không kèm debugger. Chỉ phiên chạy có debugger thực sự kết nối mới tạm dừng UART.

Framework không tạo chế độ debugger, breakpoint hay cơ chế tự tạm dừng riêng.
Việc Run, Pause, Step và sửa biến được thực hiện hoàn toàn bằng debugger của
STM32CubeIDE.

## Theo dõi kết quả

Thêm `g_TestReport` vào cửa sổ **Expressions/Watch**. Các trường quan trọng:

- `OverallTotal`, `OverallPassed`, `OverallFailed`;
- `FirstFailedCaseIndex`, `LastFailedCaseIndex`;
- `CaseReports[index].CaseId`, `Expected`, `Actual`;
- `CaseReports[index].SourceFile`, `SourceLine`;
- snapshot DET trong `DetValid`, `DetModuleId`, `DetInstanceId`, `DetApiId`,
  `DetErrorId`;
- `LastFault`, `FaultCfsr`, `FaultHfsr`, `FaultMmfar`, `FaultBfar` khi CPU fault.

Bạn có thể đặt breakpoint trực tiếp trong `Init_Test.c`, `Read_Test.c`,… rồi
sửa input hoặc biến local trước khi lời gọi API được thực thi. Framework không
tự dừng khi một case fail; nó tiếp tục chạy các testcase còn lại.

## Giám sát bộ nhớ song song với STM32CubeProgrammer (HotPlug Mode)

Để đối chứng dữ liệu Flash vật lý mà không làm gián đoạn quá trình Debug/UART:
1. Mở **STM32CubeProgrammer**.
2. Chọn kết nối **Mode: `Hot plug`** và bấm **Connect**.
3. Tab **Memory & File edition**: Nhập Address `0x0800C000` (Sector 3) hoặc `0x08060000` (Sector 7), Size `0x100`.
4. Bấm **Read** để xem dữ liệu thay đổi trực tiếp trên chip sau các lệnh Erase / Write.
