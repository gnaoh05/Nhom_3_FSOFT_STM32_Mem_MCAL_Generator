/**********************************************************************************************************************
 *  FILE:         Mem.h
 *  MODULE:       Mem (Memory Driver)
 *  MÔ TẢ:        Public API của AUTOSAR Classic Platform Memory Driver, hiện thực theo
 *                AUTOSAR_CP_SWS_MemoryDriver, Document ID 1018, AUTOSAR CP R25-11.
 *
 *                Bao gồm:
 *                  8.1 Imported Types
 *                  8.2 Type Definitions
 *                  8.3 Function Definitions (Synchronous / Asynchronous)
 *                  8.5 Scheduled Functions
 *
 *  TRACEABILITY: Mã Requirement (ví dụ [SWS_Mem_xxxxx]) được viện dẫn trong chú thích tại tệp này và
 *                Mem.c để có thể trace hiện thực ngược về specification.
 *********************************************************************************************************************/

#ifndef MEM_H
#define MEM_H

/*======================================================================================================================
 *  INCLUDE
 *====================================================================================================================*/
#include "Std_Types.h"              /* Std_ReturnType, Std_VersionInfoType, NULL_PTR, TRUE/FALSE */
#include "MemAcc_GeneralTypes.h"    /* MemAcc_AddressType, MemAcc_MemJobResultType                 */
#include "Mem_Cfg.h"                /* cấu hình pre-compile (chapter 10)                         */

/*======================================================================================================================
 *  TYPE IMPORT TỪ MemAcc  [SWS_Mem_10020]
 *  Integration test dùng Test/Stub/MemAcc/MemAcc_GeneralTypes.h. Khi tích hợp vào
 *  AUTOSAR stack hoàn chỉnh, include path sẽ trỏ tới header do MemAcc thực cung cấp.
 *====================================================================================================================*/

/*======================================================================================================================
 *  NHẬN DẠNG MODULE / VENDOR  [SWS_Mem_00074] / SWS_BSW_00101..00103, SWS_BSW_00171
 *====================================================================================================================*/
#define MEM_MODULE_ID                       255u   /* ví dụ / phụ thuộc vendor */
#define MEM_VENDOR_ID                       1u     /* ví dụ / phụ thuộc vendor */

#define MEM_AR_RELEASE_MAJOR_VERSION         25u
#define MEM_AR_RELEASE_MINOR_VERSION         11u
#define MEM_AR_RELEASE_REVISION_VERSION       0u

#define MEM_SW_MAJOR_VERSION                  1u
#define MEM_SW_MINOR_VERSION                  0u
#define MEM_SW_PATCH_VERSION                  0u

/*======================================================================================================================
 *  SERVICE ID  (cột Service ID [hex] của từng API tại chapter 8.3/8.5)
 *====================================================================================================================*/
#define MEM_SID_INIT                        0x01u  /* Mem_Init               [SWS_Mem_10008] */
#define MEM_SID_GET_VERSION_INFO            0x02u  /* Mem_GetVersionInfo     [SWS_Mem_10009] */
#define MEM_SID_MAIN_FUNCTION                0x03u  /* Mem_MainFunction       [SWS_Mem_10010] */
#define MEM_SID_GET_JOB_RESULT              0x04u  /* Mem_GetJobResult       [SWS_Mem_10011] */
#define MEM_SID_READ                        0x05u  /* Mem_Read               [SWS_Mem_10012] */
#define MEM_SID_WRITE                       0x06u  /* Mem_Write              [SWS_Mem_10013] */
#define MEM_SID_ERASE                       0x07u  /* Mem_Erase              [SWS_Mem_10014] */
#define MEM_SID_PROPAGATE_ERROR             0x08u  /* Mem_PropagateError     [SWS_Mem_10015] */
#define MEM_SID_BLANK_CHECK                 0x09u  /* Mem_BlankCheck         [SWS_Mem_10016] */
#define MEM_SID_HW_SPECIFIC_SERVICE         0x0Au  /* Mem_HwSpecificService  [SWS_Mem_10017] */
#define MEM_SID_DEINIT                      0x0Bu  /* Mem_DeInit             [SWS_Mem_10018] */
#define MEM_SID_SUSPEND                     0x0Cu  /* Mem_Suspend            [SWS_Mem_10024] */
#define MEM_SID_RESUME                      0x0Du  /* Mem_Resume             [SWS_Mem_10025] */

/*======================================================================================================================
 *  MÃ DEVELOPMENT ERROR  [SWS_Mem_00052]
 *====================================================================================================================*/
#define MEM_E_UNINIT                        0x01u  /* gọi API khi module chưa được khởi tạo                  */
#define MEM_E_PARAM_POINTER                 0x02u  /* gọi API với NULL pointer                              */
#define MEM_E_PARAM_ADDRESS                 0x03u  /* gọi API với địa chỉ không hợp lệ                      */
#define MEM_E_PARAM_LENGTH                  0x04u  /* gọi API với độ dài không hợp lệ                       */
#define MEM_E_PARAM_INSTANCE_ID             0x05u  /* gọi API với driver instance ID không hợp lệ           */
#define MEM_E_JOB_PENDING                   0x06u  /* gọi API khi đang có job request pending               */

/*======================================================================================================================
 *  GIÁ TRỊ TRẢ VỀ BỔ SUNG
 *  Được một số dịch vụ dùng để báo rằng chức năng Mem driver bên dưới chưa được hiện thực cho công nghệ
 *  bộ nhớ đang dùng, xem [SWS_Mem_00070].
 *====================================================================================================================*/
#ifndef E_MEM_SERVICE_NOT_AVAIL
#define E_MEM_SERVICE_NOT_AVAIL             ((Std_ReturnType)2U)
#endif

/*======================================================================================================================
 *  8.2  ĐỊNH NGHĨA TYPE
 *====================================================================================================================*/

/* [SWS_Mem_10002] Mem_AddressType: type địa chỉ thiết bị bộ nhớ vật lý, suy ra từ MemAcc_AddressType */
typedef MemAcc_AddressType Mem_AddressType;

/* [SWS_Mem_10000] Mem_ConfigType: type cấu trúc cấu hình postbuild.
 * Theo [SWS_Mem_00087], configPtr truyền vào Mem_Init() hiện chưa dùng và phải là NULL pointer; type vẫn
 * được khai báo để đáp ứng SRS_BSW_00414. */
typedef struct
{
    uint8 Mem_ConfigType_Reserved; /* không dùng, chỉ giữ để đủ interface */
} Mem_ConfigType;

/* [SWS_Mem_10003] Mem_DataType: type dữ liệu cho user buffer read/write */
typedef uint8 Mem_DataType;

/* [SWS_Mem_10004] Mem_InstanceIdType: type Memory driver instance ID */
typedef uint32 Mem_InstanceIdType;

/* [SWS_Mem_10007] Mem_LengthType: type độ dài của thiết bị bộ nhớ vật lý */
typedef uint32 Mem_LengthType;

/* [SWS_Mem_10026] Mem_HwServiceIdType: type định danh yêu cầu hardware specific service */
typedef uint32 Mem_HwServiceIdType;

/*======================================================================================================================
 *  8.3.1  HÀM ĐỒNG BỘ
 *====================================================================================================================*/

/* [SWS_Mem_10008] Mem_Init: hàm khởi tạo */
extern void Mem_Init(const Mem_ConfigType* configPtr);

/* [SWS_Mem_10018] Mem_DeInit: hàm hủy khởi tạo */
extern void Mem_DeInit(void);

#if (MEM_VERSION_INFO_API == STD_ON)
/* [SWS_Mem_10009] Mem_GetVersionInfo: trả về thông tin phiên bản module Mem */
extern void Mem_GetVersionInfo(Std_VersionInfoType* versionInfoPtr);
#endif

/* [SWS_Mem_10011] Mem_GetJobResult: trả về kết quả job gần nhất */
extern MemAcc_MemJobResultType Mem_GetJobResult(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10024] Mem_Suspend: tạm dừng thao tác bộ nhớ đang chạy bằng cơ chế phần cứng */
extern Std_ReturnType Mem_Suspend(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10025] Mem_Resume: tiếp tục thao tác bộ nhớ đã tạm dừng bằng cơ chế phần cứng */
extern Std_ReturnType Mem_Resume(Mem_InstanceIdType instanceId);

/* [SWS_Mem_10015] Mem_PropagateError: báo lỗi truy cập (ví dụ ECC) từ system ECC handler */
extern void Mem_PropagateError(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  8.3.2  HÀM BẤT ĐỒNG BỘ
 *====================================================================================================================*/

/* [SWS_Mem_10012] Mem_Read: kích hoạt read job */
extern Std_ReturnType Mem_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length);

/* [SWS_Mem_10013] Mem_Write: kích hoạt write job */
extern Std_ReturnType Mem_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length);

/* [SWS_Mem_10014] Mem_Erase: kích hoạt erase job */
extern Std_ReturnType Mem_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

/* [SWS_Mem_10016] Mem_BlankCheck: kích hoạt job kiểm tra trạng thái đã xóa của vùng nhớ */
extern Std_ReturnType Mem_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

/* [SWS_Mem_10017] Mem_HwSpecificService: điều phối hardware specific memory driver job */
extern Std_ReturnType Mem_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr);

/*======================================================================================================================
 *  8.5  HÀM ĐƯỢC LẬP LỊCH
 *  [SWS_Mem_10010] Mem_MainFunction: xử lý job được yêu cầu và thao tác quản lý nội bộ.
 *  Có thể truy cập qua SchM_Mem.h khi tích hợp đầy đủ RTE/SchM; cũng khai báo tại đây để module dùng độc lập.
 *  Phải được gọi tuần hoàn, xem [SWS_Mem_00066]. Không yêu cầu chu kỳ cố định.
 *====================================================================================================================*/
extern void Mem_MainFunction(void);

#endif /* MEM_H */
