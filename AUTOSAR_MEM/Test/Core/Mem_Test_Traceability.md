# AUTOSAR MEM Requirement Traceability

## Phạm vi

- Dự án: `AUTOSAR_MEM`
- Target: `STM32F401RE`
- Phong cách test: mức thanh ghi trên target, không dùng HAL flash service
- Tài liệu tham chiếu:
  - `AUTOSAR_CP_SWS_MemoryDriver.pdf` (AUTOSAR CP R25-11, Doc ID 1018)
  - `Refenrence manual Stm32f401re.pdf`

Tệp này theo dõi các Requirement được bộ test target STM32 trong `Test/Testcase` kiểm tra.
Bộ test hướng đến các Requirement vừa:

- được dự án này hiện thực, và
- có thể kiểm tra có ý nghĩa tại runtime trên port Flash nội STM32F401 hiện tại.

Requirement chỉ liên quan cấu hình, chỉ compile-time, chỉ review hoặc không áp dụng cho công nghệ bộ nhớ này được liệt kê riêng.

## Tóm Tắt Runtime Coverage

- Số testcase runtime: `49`
- Target execution bao phủ:
  - API definition
  - DET
  - trạng thái job
  - thực thi bất đồng bộ
  - write/erase/blank-check Flash có phá hủy dữ liệu
  - optional service không hỗ trợ

## Danh Sách Testcase

- `0x0101` `JobResultAfterInit`
- `0x0102` `VersionInfoMatches`
- `0x0103` `InitRejectsNonNullConfig`
- `0x0104` `VersionInfoRejectsNullPointer`
- `0x0105` `DeInitTransitionsToUninit`
- `0x0201` `ReadBeforeInit`
- `0x0202` `ReadNullPointer`
- `0x0203` `ReadInvalidAddress`
- `0x0204` `ReadInvalidLength`
- `0x0205` `ReadInvalidInstance`
- `0x0301` `InitialJobResultOk`
- `0x0302` `AcceptedReadSetsPending`
- `0x0303` `ReadCompletesWithOk`
- `0x0304` `GetJobResultInvalidInstance`
- `0x0305` `RejectedReadKeepsJobResultOk`
- `0x0401` `ReadValidAddress`
- `0x0402` `ReadRejectsInvalidAddress`
- `0x0403` `ReadRejectsInvalidLength`
- `0x0404` `ReadRejectsInvalidInstance`
- `0x0405` `ReadRejectsPendingRequest`
- `0x0501` `WriteValidPattern`
- `0x0502` `WriteRejectsInvalidAddress`
- `0x0503` `WriteRejectsInvalidInstance`
- `0x0504` `WriteRejectsNullPointer`
- `0x0505` `WriteRejectsInvalidLength`
- `0x0506` `WriteRejectsPendingJob`
- `0x0601` `EraseValidSector`
- `0x0602` `EraseRejectsMisalignedRequest`
- `0x0603` `EraseRejectsInvalidAddress`
- `0x0604` `EraseRejectsInvalidInstance`
- `0x0605` `EraseRejectsInvalidLength`
- `0x0606` `EraseRejectsPendingJob`
- `0x0701` `BlankSectorReturnsOk`
- `0x0702` `NonBlankSectorReturnsInconsistent`
- `0x0703` `BlankCheckRejectsInvalidInstance`
- `0x0704` `BlankCheckRejectsInvalidAddress`
- `0x0705` `BlankCheckRejectsInvalidLength`
- `0x0706` `BlankCheckRejectsPendingJob`
- `0x0801` `RejectSecondPendingJob`
- `0x0802` `PropagateErrorCancelsJob`
- `0x0803` `HwSpecificServiceNotAvailable`
- `0x0804` `SuspendNotAvailable`
- `0x0805` `ResumeNotAvailable`
- `0x0806` `PropagateErrorInvalidInstance`
- `0x0807` `HwSpecificRejectsInvalidInstance`
- `0x0808` `HwSpecificRejectsNullDataPtr`
- `0x0809` `HwSpecificRejectsNullLengthPtr`
- `0x0810` `SuspendRejectsInvalidInstance`
- `0x0811` `ResumeRejectsInvalidInstance`

## Requirement Coverage Matrix

| Requirement | Coverage | Testcases | Ghi chú |
| --- | --- | --- | --- |
| `SWS_Mem_10008` | Runtime | `0x0101`, `0x0103` | Hành vi API `Mem_Init()` |
| `SWS_Mem_00001` | Runtime | `0x0101`, `0x0301` | Kết quả job ban đầu trở thành `MEM_JOB_OK` |
| `SWS_Mem_00087` | Runtime | `0x0103` | `configPtr` phải là `NULL_PTR` |
| `SWS_Mem_10009` | Runtime | `0x0102`, `0x0104` | Hành vi API `Mem_GetVersionInfo()` |
| `SWS_Mem_00002` | Runtime | `0x0104` | DET của `Mem_GetVersionInfo(NULL_PTR)` |
| `SWS_Mem_10018` | Runtime | `0x0105` | Hành vi API `Mem_DeInit()` |
| `SWS_Mem_00079` | Runtime và review | `0x0105` | Runtime kiểm tra trạng thái đã hủy khởi tạo; việc hủy hoàn toàn Flash đang chạy vẫn bị giới hạn bởi Flash single-bank STM32F401 |
| `Section7.3.1` | Runtime | `0x0105`, `0x0201` | API, ngoài ngoại lệ được cho phép, yêu cầu khởi tạo |
| `SWS_Mem_00052` | Runtime | `0x0105`, `0x0201`, `0x0202`, `0x0203`, `0x0204`, `0x0205`, `0x0304`, `0x0402`, `0x0403`, `0x0404`, `0x0502`, `0x0503`, `0x0504`, `0x0505`, `0x0602`, `0x0603`, `0x0604`, `0x0605`, `0x0703`, `0x0704`, `0x0705`, `0x0806`, `0x0807`, `0x0808`, `0x0809`, `0x0810`, `0x0811` | Phân loại development error được target test thực thi |
| `SWS_Mem_10011` | Runtime | `0x0301`, `0x0304` | Hành vi API `Mem_GetJobResult()` |
| `SWS_Mem_00029` | Runtime | `0x0301`, `0x0302`, `0x0305` | Driver theo dõi kết quả job hiện tại |
| `SWS_Mem_00090` | Runtime | `0x0304` | Instance ID không hợp lệ trong `Mem_GetJobResult()` |
| `SWS_Mem_00059` | Runtime | `0x0305`, `0x0801` | Job request không thể xử lý bị từ chối bằng `E_NOT_OK` |
| `SWS_Mem_10012` | Runtime | `0x0202`, `0x0203`, `0x0204`, `0x0205`, `0x0302`, `0x0303`, `0x0305`, `0x0401`, `0x0402`, `0x0403`, `0x0404`, `0x0405`, `0x0801` | Hành vi API `Mem_Read()` |
| `SWS_Mem_00004` | Runtime | `0x0205`, `0x0404` | Instance ID không hợp lệ trong `Mem_Read()` |
| `SWS_Mem_00005` | Runtime | `0x0202` | DET của `Mem_Read(NULL_PTR)` |
| `SWS_Mem_00006` | Runtime | `0x0203`, `0x0402` | Địa chỉ không hợp lệ trong `Mem_Read()` |
| `SWS_Mem_00072` | Runtime | `0x0204`, `0x0305`, `0x0403` | Độ dài không hợp lệ trong `Mem_Read()` |
| `SWS_Mem_00007` | Runtime | `0x0405`, `0x0801` | `Mem_Read()` khi job trước đang pending |
| `SWS_Mem_00030` | Runtime | `0x0302` | Yêu cầu đã chấp nhận đổi trạng thái thành `MEM_JOB_PENDING` |
| `SWS_Mem_10010` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | Xử lý được lập lịch bởi `Mem_MainFunction()` |
| `SWS_Mem_00066` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | Dịch vụ bất đồng bộ được thực thi trong `Mem_MainFunction()` |
| `SWS_Mem_00067` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | Job thành công kết thúc với `MEM_JOB_OK` |
| `SWS_Mem_10013` | Runtime | `0x0501`, `0x0502`, `0x0503`, `0x0504`, `0x0505`, `0x0506` | Hành vi API `Mem_Write()` |
| `SWS_Mem_00009` | Runtime | `0x0503` | Instance ID không hợp lệ trong `Mem_Write()` |
| `SWS_Mem_00010` | Runtime | `0x0504` | DET của `Mem_Write(NULL_PTR)` |
| `SWS_Mem_00011` | Runtime | `0x0502` | Địa chỉ không hợp lệ trong `Mem_Write()` |
| `SWS_Mem_00012` | Runtime | `0x0505` | Độ dài không hợp lệ trong `Mem_Write()` |
| `SWS_Mem_00013` | Runtime | `0x0506` | `Mem_Write()` khi job trước đang pending |
| `SWS_Mem_10014` | Runtime | `0x0601`, `0x0602`, `0x0603`, `0x0604`, `0x0605`, `0x0606` | Hành vi API `Mem_Erase()` |
| `SWS_Mem_00015` | Runtime | `0x0604` | Instance ID không hợp lệ trong `Mem_Erase()` |
| `SWS_Mem_00016` | Runtime | `0x0602`, `0x0603` | Địa chỉ không hợp lệ trong `Mem_Erase()` |
| `SWS_Mem_00017` | Runtime | `0x0605` | Độ dài không hợp lệ trong `Mem_Erase()` |
| `SWS_Mem_00018` | Runtime | `0x0606` | `Mem_Erase()` khi job trước đang pending |
| `SWS_Mem_00035` | Runtime | `0x0602` | Không tự điều chỉnh căn chỉnh xóa; yêu cầu sector không căn phải bị từ chối |
| `SWS_Mem_10016` | Runtime | `0x0701`, `0x0702`, `0x0703`, `0x0704`, `0x0705`, `0x0706` | Hành vi API `Mem_BlankCheck()` |
| `SWS_Mem_00022` | Runtime | `0x0703` | Instance ID không hợp lệ trong `Mem_BlankCheck()` |
| `SWS_Mem_00023` | Runtime | `0x0704` | Địa chỉ không hợp lệ trong `Mem_BlankCheck()` |
| `SWS_Mem_00024` | Runtime | `0x0705` | Độ dài không hợp lệ trong `Mem_BlankCheck()` |
| `SWS_Mem_00025` | Runtime | `0x0706` | `Mem_BlankCheck()` khi job trước đang pending |
| `SWS_Mem_00076` | Runtime | `0x0702` | Job hoàn tất có kết quả không như kỳ vọng báo `MEM_INCONSISTENT` |
| `SWS_Mem_00057` | Runtime | `0x0801` | Mỗi driver instance chỉ có một job tại một thời điểm |
| `SWS_Mem_10015` | Runtime | `0x0802`, `0x0806` | Hành vi API `Mem_PropagateError()` |
| `SWS_Mem_00061` | Runtime | `0x0802` | `Mem_PropagateError()` đặt `MEM_ECC_UNCORRECTED` và hủy job hiện tại |
| `SWS_Mem_00020` | Runtime | `0x0806` | Instance ID không hợp lệ trong `Mem_PropagateError()` |
| `SWS_Mem_10017` | Runtime | `0x0803`, `0x0807`, `0x0808`, `0x0809` | Hành vi API `Mem_HwSpecificService()` |
| `SWS_Mem_00070` | Runtime | `0x0803` | Optional service không hỗ trợ trả `E_MEM_SERVICE_NOT_AVAIL` |
| `SWS_Mem_00026` | Runtime | `0x0807` | Instance ID không hợp lệ trong `Mem_HwSpecificService()` |
| `SWS_Mem_00027` | Runtime | `0x0808`, `0x0809` | Xử lý null pointer trong `Mem_HwSpecificService()` |
| `SWS_Mem_10024` | Runtime | `0x0804`, `0x0810` | Hành vi API `Mem_Suspend()` |
| `SWS_Mem_00082` | Runtime | `0x0804`, `0x0805` | Suspend/resume không hỗ trợ trả `E_MEM_SERVICE_NOT_AVAIL` |
| `SWS_Mem_00091` | Runtime | `0x0810` | Instance ID không hợp lệ trong `Mem_Suspend()` |
| `SWS_Mem_10025` | Runtime | `0x0805`, `0x0811` | Hành vi API `Mem_Resume()` |
| `SWS_Mem_00092` | Runtime | `0x0811` | Instance ID không hợp lệ trong `Mem_Resume()` |
| `SWS_Mem_00088` | Runtime và review | `0x0501`, `0x0601`, `0x0701`, `0x0702` | Runtime test xác nhận chiến lược đọc lại và blank-check của upper layer; việc chứng minh driver không tự xác minh là hoạt động inspection |

## Requirement Chỉ Review Hoặc Không Áp Dụng

| Requirement | Trạng thái | Lý do |
| --- | --- | --- |
| `SWS_Mem_00031` | Chỉ review | Bộ test tham chiếu STM32F401 chưa có đường fault injection runtime xác định cho trường hợp yêu cầu được chấp nhận nhưng sau đó lỗi phần cứng. |
| `SWS_Mem_00080` | Không áp dụng cho port này | Flash nội STM32F401 không được reference driver hỗ trợ suspend; `SWS_Mem_00082` điều khiển hành vi runtime quan sát được. |
| `SWS_Mem_00081` | Không áp dụng cho port này | Flash nội STM32F401 không được reference driver hỗ trợ resume; `SWS_Mem_00082` điều khiển hành vi runtime quan sát được. |
| `SWS_Mem_00083` | Không áp dụng cho port này | Requirement chỉ áp dụng khi có hỗ trợ suspend. |
| `SWS_Mem_00084` | Không áp dụng cho port này | Requirement chỉ áp dụng khi có hỗ trợ resume. |

## Ghi Chú Thực Thi STM32F401

- Test Flash phá hủy dữ liệu chỉ dùng một sector dành riêng:
  - `TEST_FLASH_SECTOR_ADDRESS = 0x08060000`
  - `TEST_FLASH_SECTOR_LENGTH = 0x00020000`
- Khung test kiểm tra điểm cuối image sau liên kết phải nằm dưới sector đó trước khi cho phép test write/erase/blank-check.
- Test DET và pending-job không phá hủy dữ liệu chủ ý không gọi `Mem_MainFunction()` sau khi job được chấp nhận, để kiểm tra hành vi xếp hàng mà không thay đổi nội dung Flash.
