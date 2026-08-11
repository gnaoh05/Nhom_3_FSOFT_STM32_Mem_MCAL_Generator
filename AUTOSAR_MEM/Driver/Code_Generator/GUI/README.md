# AUTOSAR Mem Configurator

GUI và CLI cấu hình Mem/Flash cho dự án AUTOSAR_MEM. Engine Python chỉ dùng
standard library; metadata, cấu hình project và code template được tách thành
các artifact độc lập theo flow EPD -> EPC -> templates -> generated C/H.

## Cấu trúc

    GUI/
      mem_configurator.py
      Mem.epd                         # BSW module definition ổn định
      examples/STM32F401.epc          # cấu hình project mẫu
    templates/
      include/*.template              # template C header
      src/*.template                  # template C source
    ../../../Test/Generated_File/
      include/*.h                     # output được STM32CubeIDE compile
      src/*.c                         # output được STM32CubeIDE compile

Mem.epd định nghĩa container, parameter, type, range, enum và description.
EPC chứa các giá trị cụ thể của từng project. GUI đọc metadata từ EPD để hiển
thị mô tả/ràng buộc và validator kiểm tra cả rule của driver lẫn EPD.

## Chạy GUI

Tại thư mục này:

    python mem_configurator.py

Hoặc từ thư mục gốc repository:

    python AUTOSAR_MEM/Driver/Code_Generator/GUI/mem_configurator.py

Yêu cầu: Python 3.9 trở lên có Tkinter. Bản Python chính thức trên Windows đã
bao gồm Tkinter theo mặc định.

## Chức năng

- Cấu hình MemGeneral và Flash STM32F401RE.
- Đọc canonical Mem.epd để lấy metadata, range và enum cho GUI/validator.
- Thêm, sửa, xóa và kiểm tra bảng sector.
- Lưu/mở project dạng .memcfg.json.
- Import file `.epd`/`.epc` dạng cũ từ nhánh `gui-code-generation` và file
  AUTOSAR ECUC `.epc` chuẩn do tool này xuất.
- Xuất bản sao canonical Mem.epd, không trộn project values vào definition.
- Xuất Mem.epc: ECUC Configuration Values dạng XML AUTOSAR.
- Render bốn template ngoài bằng engine strict-token, không phụ thuộc Jinja2.
- Sinh include/Mem_Cfg.h, src/Mem_Cfg.c, include/Flash_IP_Cfg.h và
  src/Flash_IP_Cfg.c.
- Phát hiện sector chồng lấn, khoảng trống, sai tổng kích thước, địa chỉ ngoài
  vùng Flash và các giá trị không tương thích driver.

## Kiểm tra không cần mở GUI

    python mem_configurator.py --self-test

Xuất toàn bộ bộ file mặc định:

    python mem_configurator.py --export-default generated

Sinh lại từ một project đã lưu:

    python mem_configurator.py --generate Mem.memcfg.json --output generated

Chạy toàn bộ pipeline từ EPC mẫu và cập nhật đúng bốn file mà STM32CubeIDE
hiện đang compile:

    python mem_configurator.py \
      --generate examples/STM32F401.epc \
      --output ../../../Test/Generated_File

Có thể chỉ định artifact khác:

    python mem_configurator.py \
      --generate Project.epc \
      --definition Mem.epd \
      --templates ../templates \
      --output generated

Có thể thay `Mem.memcfg.json` bằng `MemDriver.epd`, `MemDriver.epc` hoặc một
file AUTOSAR EPC chuẩn. Trên GUI, dùng **File > Open / import...**. Sau khi
import file XML cũ, hãy lưu thành `.memcfg.json` để tiếp tục chỉnh sửa bằng
định dạng tích hợp.

## Tích hợp với nhánh gui-code-generation

Phần import tương thích được xây dựng từ code/config mới nhất đã fetch ở
`origin/gui-code-generation` (commit `f5fe4ab`). Các giá trị dùng được với
driver hiện tại được ánh xạ gồm DET, VersionInfo API, MemIndex, invocation,
chu kỳ MainFunction, erased value, instance và bảng sector.

Ý tưởng EPD-driven GUI và external templates đã được port, nhưng template
Jinja2 cũ không được dùng nguyên trạng vì nó phụ thuộc cấu trúc
Mem_ConfigType/sector batch đã không còn tồn tại. Bốn template mới sinh đúng
interface pre-compile của driver hiện tại. Template chỉ là đầu vào; output C/H
được tách riêng tại `Test/Generated_File` để STM32CubeIDE không compile nhầm
template hoặc tạo trùng symbol.

Các tham số FlashLatency, cache/prefetch, PSIZE và timeout của tool cũ chưa
được đưa vào canonical EPD vì driver hiện tại không tiêu thụ chúng. Nếu bổ sung
sau này, cần thay đổi đồng thời EPD, MemConfig, template, Flash_IP và test.

## Giới hạn tương thích EPD/EPC

EPD/EPC được xuất theo mô hình AUTOSAR ECUC và namespace AUTOSAR R4. Tên release
trên GUI được ghi vào metadata/comment để truy vết. Trước khi dùng trong dự án
sản xuất, cần kiểm tra file bằng đúng XSD và plugin của EB tresos hoặc công cụ
AUTOSAR đích. Mỗi vendor/release có thể yêu cầu thêm metadata hoặc ràng buộc
ngoài phần chuẩn.
