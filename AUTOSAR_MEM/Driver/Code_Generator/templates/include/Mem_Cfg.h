/**********************************************************************************************************************
 *  FILE:         Mem_Cfg.h
 *  MODULE:       Mem (Memory Driver) - Pre-Compile Configuration
 *  MÔ TẢ:        Ví dụ cấu hình sinh cho Mem driver, tương ứng chapter 10
 *                "Configuration Specification" của AUTOSAR_CP_SWS_MemoryDriver (Doc ID 1018, R25-11).
 *
 *                Ánh xạ tới:
 *                  MemGeneral            -> [ECUC_Mem_00002]
 *                  MemDevErrorDetect     -> [ECUC_Mem_00004]
 *                  MemIndex              -> [ECUC_Mem_00023]
 *                  MemInvocation         -> [ECUC_Mem_00025]
 *                  MemMainFunctionPeriod -> [ECUC_Mem_00029]
 *                  MemInstance           -> [ECUC_Mem_00003]/[ECUC_Mem_00007]
 *                  MemSectorBatch        -> [ECUC_Mem_00009] .. [ECUC_Mem_00014]
 *                  MemPublishedInformation/MemErasedValue -> [ECUC_Mem_00020]/[ECUC_Mem_00021]
 *
 *                GHI CHÚ: Đây là cấu hình VÍ DỤ viết tay cho một Mem driver instance ánh xạ tới Flash nội
 *                STM32F401RE (512 KB / 8 sector, địa chỉ 0x08000000-0x0807FFFF). Với dự án thực tế, hãy
 *                thay bằng kết quả từ configuration tool.
 *********************************************************************************************************************/

#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"

/*======================================================================================================================
 *  MemGeneral container  [ECUC_Mem_00002]
 *====================================================================================================================*/

/* MemDevErrorDetect [ECUC_Mem_00004]: bật/tắt DET */
#define MEM_DEV_ERROR_DETECT            STD_ON

/* Version info API, xem [SWS_Mem_10009] / SRS_BSW_00003 */
#define MEM_VERSION_INFO_API            STD_ON

/* MemIndex [ECUC_Mem_00023]: InstanceId của Mem driver module instance này, bằng 0 khi chỉ có một driver */
#define MEM_INDEX                       0u

/* MemInvocation [ECUC_Mem_00025]: DIRECT_STATIC | INDIRECT_STATIC | INDIRECT_DYNAMIC */
#define MEM_INVOCATION_DIRECT_STATIC    0u
#define MEM_INVOCATION_INDIRECT_STATIC  1u
#define MEM_INVOCATION_INDIRECT_DYNAMIC 2u
#define MEM_INVOCATION                  MEM_INVOCATION_DIRECT_STATIC

/* [SWS_Mem_00038]/[SRS_MemHwAb_14045]/[SRS_MemHwAb_14049]: invocation INDIRECT_STATIC và INDIRECT_DYNAMIC
 * cần định dạng binary image chuẩn của Mem driver tại chapter 7.2.6: header, bảng service function pointer,
 * delimiter và dynamic driver activation. Reference driver chưa hiện thực phần này do ngoài phạm vi, xem
 * traceability review của dự án: không có use case OTA background update. Hãy báo lỗi compile-time thay vì
 * âm thầm nhận MemInvocation mà driver không thể đáp ứng. */
#if (MEM_INVOCATION != MEM_INVOCATION_DIRECT_STATIC)
#error "Only MEM_INVOCATION_DIRECT_STATIC is implemented by this reference Mem driver. INDIRECT_STATIC/INDIRECT_DYNAMIC require the chapter 7.2.6 binary image format, which is not implemented - see Mem_Cfg.h."
#endif

/* MemMainFunctionPeriod [ECUC_Mem_00029] tính bằng giây, chỉ dùng cho tài liệu / cấu hình SchM */
#define MEM_MAIN_FUNCTION_PERIOD        0.005f

/*======================================================================================================================
 *  MemInstance container  [ECUC_Mem_00003]
 *  STM32F401RE có một thiết bị Flash nội, nên chỉ cấu hình một Mem driver instance ánh xạ tới thiết bị đó.
 *  [SRS_MemHwAb_14043] multi-instance không được dùng tại đây, nhưng driver vẫn generic, xem MEM_INSTANCE_COUNT.
 *====================================================================================================================*/

#define MEM_INSTANCE_COUNT              1u

/* Tên symbolic của MemInstanceId [ECUC_Mem_00007] */
#define MemConf_MemInstance_MemInstance_0   0u

/* Tham số MemSectorBatch [ECUC_Mem_00009]..[ECUC_Mem_00014]. Flash STM32F401RE KHÔNG phân đoạn đồng đều
 * (4x16KB + 1x64KB + 3x128KB), vì vậy hình học từng sector được Flash_IP.c sở hữu qua
 * Flash_IP_GetSectorFromAddress/StartAddress/Size(), không lặp lại ở đây. Mem_Cfg.h chỉ cần dải địa chỉ tổng
 * để làm tài liệu và cross-check; Mem_IPW.c kiểm tra trực tiếp địa chỉ/độ dài với Flash_IP. */

#define MEM_INSTANCE_0_START_ADDRESS     0x08000000UL   /* MemStartAddress [ECUC_Mem_00014]: Flash base */
#define MEM_INSTANCE_0_SIZE              0x00080000UL   /* Tổng kích thước: 512 KB                       */

/*======================================================================================================================
 *  MemPublishedInformation container  [ECUC_Mem_00020]
 *====================================================================================================================*/

/* MemErasedValue [ECUC_Mem_00021]: nội dung của một memory cell đã xóa.
 * Được cung cấp vừa là macro compile-time cho điều kiện preprocessor/static initializer, vừa là const symbol
 * Mem_ErasedValue đọc được tại runtime trong Mem_Cfg.c. Điều này phù hợp cách tham số "Published Information"
 * của AUTOSAR thường được cung cấp như dữ liệu, không chỉ macro, để module/tool khác có thể truy vấn mà không
 * phải biên dịch lại với Mem_Cfg.h. */
#define MEM_ERASED_VALUE                 0xFFu

extern const uint32 Mem_ErasedValue;

#endif /* MEM_CFG_H */
