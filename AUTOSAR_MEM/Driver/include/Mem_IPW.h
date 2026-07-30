/**********************************************************************************************************************
 *  FILE:         Mem_IPW.h
 *  MODULE:       Mem_IPW (Memory Driver - IP Wrapper Layer)
 *  MÔ TẢ:        Lớp wrapper phần cứng được Mem.c sử dụng nội bộ.
 *
 *                AUTOSAR_CP_SWS_MemoryDriver không định nghĩa trực tiếp lớp "IPW"; nó chỉ yêu cầu public API
 *                của Mem driver (Mem.c) không phụ thuộc thiết bị nhớ ([SWS_Mem_00035]) và khi build thành binary
 *                riêng thì phải tự chứa ([SWS_Mem_00039]). Theo quy ước AUTOSAR MCAL, ví dụ Fls_Ipw, Adc_Ipw,
 *                Mem_IPW là lớp mỏng giữa Mem.c tuân thủ SWS và driver thanh ghi/IP của thiết bị nhớ thực tế.
 *                Nhờ đó, port sang phần cứng mới chỉ cần hiện thực lại Mem_IPW.c.
 *
 *                Trong dự án này, Mem_IPW.c wrapper Flash_IP (Flash_IP.h/.c), driver thanh ghi bare-metal
 *                cho Flash nội STM32F401RE. Mọi chi tiết riêng chip như thanh ghi, hình học sector và IRQ
 *                nằm trong Flash_IP; Mem_IPW.c chỉ chuyển đổi giữa API generic theo instance của Mem driver
 *                và API một thiết bị Flash của Flash_IP.
 *********************************************************************************************************************/

#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Mem.h"

/*======================================================================================================================
 *  Vòng đời
 *====================================================================================================================*/

/* Khởi tạo trạng thái riêng phần cứng cho một Mem driver instance. Được gọi từ Mem_Init() [SWS_Mem_00001]. */
extern void Mem_Ipw_Init(Mem_InstanceIdType instanceId);

/* Hủy thao tác phần cứng đang chạy và hủy khởi tạo trạng thái phần cứng cho một instance.
 * Được gọi từ Mem_DeInit() [SWS_Mem_00079]. */
extern void Mem_Ipw_DeInit(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Truy vấn khả năng dịch vụ tùy chọn
 *  Mem.c dùng để từ chối đồng bộ dịch vụ không khả dụng bằng E_MEM_SERVICE_NOT_AVAIL theo
 *  [SWS_Mem_00070] và quy tắc Suspend/Resume tại [SWS_Mem_00082].
 *====================================================================================================================*/

extern boolean Mem_Ipw_IsHwSpecificServiceSupported(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId);

extern boolean Mem_Ipw_IsSuspendResumeSupported(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Thao tác bộ nhớ bất đồng bộ
 *  Mỗi hàm chỉ kích hoạt thao tác phần cứng và trả về ngay. Tiến trình được xử lý bởi
 *  Mem_Ipw_MainFunction() và kết quả lấy bằng Mem_Ipw_GetJobResult().
 *====================================================================================================================*/

extern Std_ReturnType Mem_Ipw_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length);

extern Std_ReturnType Mem_Ipw_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr);

/*======================================================================================================================
 *  Suspend / Resume  [SRS_MemHwAb_14031] / [SWS_Mem_00082]
 *====================================================================================================================*/

extern Std_ReturnType Mem_Ipw_Suspend(Mem_InstanceIdType instanceId);
extern Std_ReturnType Mem_Ipw_Resume(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Lập lịch / lấy kết quả job
 *====================================================================================================================*/

/* Tiến thao tác phần cứng đang chạy, nếu có, của instance một bước.
 * Được gọi từ Mem_MainFunction() [SWS_Mem_00066]. */
extern void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId);

/* Trả về kết quả job phần cứng hiện tại của instance (MEM_JOB_OK / MEM_JOB_PENDING /
 * MEM_JOB_FAILED / MEM_INCONSISTENT / MEM_ECC_CORRECTED / MEM_ECC_UNCORRECTED). */
extern MemAcc_MemJobResultType Mem_Ipw_GetJobResult(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Hàm hỗ trợ kiểm tra địa chỉ / độ dài
 *  Mem.c dùng để hiện thực kiểm tra development error [SWS_Mem_00006][SWS_Mem_00072][SWS_Mem_00011]
 *  [SWS_Mem_00012][SWS_Mem_00016][SWS_Mem_00017][SWS_Mem_00023][SWS_Mem_00024].
 *====================================================================================================================*/

extern boolean Mem_Ipw_IsAddressValid(Mem_InstanceIdType instanceId, Mem_AddressType address);

extern boolean Mem_Ipw_IsLengthValid(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length);

/* [SWS_Mem_00035]: Mem.c dùng trong Mem_CheckEraseAlignment() để từ chối đồng bộ Mem_Erase() không khớp
 * chính xác một sector vật lý TRƯỚC KHI job được chấp nhận theo [SWS_Mem_00059], tức trước khi nó được xếp
 * hàng để Mem_MainFunction() kích hoạt phần cứng sau đó. */
extern boolean Mem_Ipw_IsEraseAligned(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length);

#endif /* MEM_IPW_H */
