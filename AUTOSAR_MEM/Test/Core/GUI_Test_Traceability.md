# AUTOSAR MEM Configurator Tool (GUI & Generator) Requirement Traceability

## 1. Phạm vi

- Dự án: `AUTOSAR_MEM_Configurator`
- Mục tiêu: Kiểm thử toàn diện công cụ giao diện người dùng (GUI) và bộ sinh mã nguồn cấu hình (Code Generator).
- Tài liệu & Tiêu chuẩn tham chiếu:
  - `AUTOSAR_CP_SWS_MemoryDriver.pdf` (AUTOSAR CP R25-11, Doc ID 1018)
  - `AUTOSAR_TPS_ECUConfigurationParameters.pdf` (AUTOSAR Schema R4.0/R4.4)
  - `MemDriver.epd` (ECUC Module Definition)
  - `MemDriver.epc` (ECUC Description Value Configuration)

---

## 2. Tóm Tắt Phân Loại Kiểm Thử GUI & Generator

Bộ kiểm thử GUI được chia thành **6 Nhóm Kiểm Thử (18 Testcases)**:

| Mã Nhóm | Tên Nhóm | Số Lượng TC | Mục Tiêu Kiểm Thử |
| :---: | :--- | :---: | :--- |
| **EPD** | EPD Schema Parsing & Metamodel | 3 | Kiểm tra khả năng đọc hiểu và trích xuất Schema chuẩn ARXML |
| **VAL** | Input Validation & Constraints | 4 | Kiểm tra ràng buộc kiểu dữ liệu, miền giá trị MIN/MAX, địa chỉ Hex |
| **GRP** | Sector Batch Grouping Algorithm | 3 | Kiểm tra thuật toán gộp sector liên tiếp có cùng kích thước |
| **EPC** | EPC Serialization & Round-Trip | 3 | Kiểm tra tính toàn vẹn dữ liệu khi Lưu/Mở file cấu hình EPC XML |
| **GEN** | Template Rendering & Generation | 3 | Kiểm tra sinh đúng cú pháp C cho 4 file cấu hình (`.h` và `.c`) |
| **E2E** | End-to-End Compile Compatibility| 2 | Kiểm tra mã sinh ra từ GUI biên dịch thành công cùng Driver C |

---

## 3. Danh Sách Chi Tiết 18 Testcases GUI & Generator

### Nhóm 1: EPD Group (ARXML EPD Schema Parsing)
- **`GUI_EPD_001`**: `ParseEpdRootModule` - Đọc đúng thẻ `ECUC-MODULE-DEF`, ShortName `Mem`, phiên bản Schema 4.4.0.
- **`GUI_EPD_002`**: `ExtractContainerDefinitions` - Trích xuất đầy đủ các Container: `MemGeneral`, `MemInstance`, `MemSectorBatch`, `FlashIPConfig`.
- **`GUI_EPD_003`**: `ExtractParameterMetadata` - Trích xuất đúng kiểu (BOOLEAN, INTEGER, ENUM), giá trị mặc định, miền MIN/MAX và ToolTip Description.

### Nhóm 2: VAL Group (Input Validation & Error Handling)
- **`GUI_VAL_001`**: `IntegerRangeValidation` - Kiểm tra giá trị hợp lệ trong khoảng `[MIN, MAX]`; từ chối giá trị < MIN hoặc > MAX.
- **`GUI_VAL_002`**: `NonNumericRejection` - Từ chối các chuỗi không phải số (ví dụ: `"abc"`, `"--"`) nhập vào ô số nguyên.
- **`GUI_VAL_003`**: `HexAddressFormatValidation` - Kiểm tra tính hợp lệ của địa chỉ Hex (ví dụ: `0x08000000` hợp lệ, `0xXYZ` không hợp lệ).
- **`GUI_VAL_004`**: `SectorParameterValidation` - Kiểm tra tính hợp lệ của từng tham số cấu hình Sector trong bảng dữ liệu.

### Nhóm 3: GRP Group (Sector Batch Grouping Algorithm)
- **`GUI_GRP_001`**: `GroupContiguousSectors` - Kiểm tra gộp 4 sector 16KB liên tiếp (`SECTOR_0` đến `SECTOR_3`) thành 1 Batch có `MemNumberOfSectors = 4`.
- **`GUI_GRP_002`**: `SplitOnDifferentSize` - Tách thành Batch mới khi gặp sector có kích thước khác (ví dụ: chuyển từ 16KB sang 64KB `SECTOR_4`).
- **`GUI_GRP_003`**: `FullSTM32F401Grouping` - Gộp toàn bộ 8 sector của STM32F401RE thành đúng 3 Batch (`4x16KB`, `1x64KB`, `3x128KB`).

### Nhóm 4: EPC Group (EPC File Save & Load Round-Trip)
- **`GUI_EPC_001`**: `SaveEpcStructure` - Xuất file `.epc` đúng cấu trúc XML AUTOSAR với các thẻ `Container`, `Parameter`, `List`, `Item`.
- **`GUI_EPC_002`**: `LoadEpcDataIntegrity` - Nạp file `.epc` và phục hồi chính xác 100% giá trị các ô nhập liệu và danh sách sector.
- **`GUI_EPC_003`**: `CorruptedEpcHandling` - Xử lý an toàn khi nạp file XML không hợp lệ (không crash ứng dụng).

### Nhóm 5: GEN Group (Jinja2 Code Generation)
- **`GUI_GEN_001`**: `GenerateMemCfgH` - Sinh file `Mem_Cfg.h` chứa đúng các macro cấu hình (`MEM_DEV_ERROR_DETECT`, `MEM_MAX_INSTANCES`).
- **`GUI_GEN_002`**: `GenerateMemCfgC` - Sinh file `Mem_Cfg.c` chứa mảng cấu hình `Mem_SectorBatchConfigType` và `Mem_ConfigData`.
- **`GUI_GEN_003`**: `GenerateFlashIpCfg` - Sinh file `Flash_IP_Cfg.h` và `Flash_IP_Cfg.c` tương thích với tầng phần cứng Flash IP.

### Nhóm 6: E2E Group (End-to-End Build Verification)
- **`GUI_E2E_001`**: `CompileGeneratedCodeStdOn` - Biên dịch mã nguồn sinh từ GUI ở chế độ `MEM_DEV_ERROR_DETECT = STD_ON` với GCC (0 errors, 0 warnings).
- **`GUI_E2E_002`**: `CompileGeneratedCodeStdOff` - Biên dịch mã nguồn sinh từ GUI ở chế độ `MEM_DEV_ERROR_DETECT = STD_OFF` với GCC (0 errors, 0 warnings).

---

## 4. Hướng Dẫn Chạy Kiểm Thử Tự Động

Chạy toàn bộ bộ test GUI tự động bằng lệnh:
```bash
python AUTOSAR_MEM/Test/Testcase/test_gui_generator.py
```
