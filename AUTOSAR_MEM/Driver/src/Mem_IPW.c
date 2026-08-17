/**********************************************************************************************************************
 *  FILE:         Mem_IPW.c
 *  MODULE:       Mem_IPW (Memory Driver - IP Wrapper Layer)
 *  MÔ TẢ:        Hiện thực lớp IP Wrapper cho STM32F401RE. Mọi thao tác phần cứng được chuyển đến
 *                Flash_IP (Flash_IP.h/.c), driver thanh ghi bare-metal của Flash nội. Cách này giúp Mem.c
 *                không phụ thuộc phần cứng theo [SWS_Mem_00035] và gom chi tiết STM32F401RE (thanh ghi,
 *                hình học sector, xử lý IRQ) vào Flash_IP.
 *
 *                Chỉ có Mem driver instance 0 (xem Mem_Cfg.h - MEM_INSTANCE_COUNT == 1), ánh xạ 1:1
 *                tới Flash bank nội duy nhất của STM32F401RE.
 *
 *                Quản lý kết quả job:
 *                Flash_IP chỉ theo dõi trạng thái Program/Erase (thao tác bất đồng bộ qua polling).
 *                Read và BlankCheck hoàn tất đồng bộ tại lớp này vì Flash memory-mapped, nên kết quả được
 *                tính và lưu cục bộ thay vì đọc từ Flash_IP_GetStatus(). Mem_Ipw_GetJobResult() chọn đúng
 *                nguồn theo loại thao tác được kích hoạt gần nhất.
 *********************************************************************************************************************/

#include "Mem_IPW.h"
#include "Flash_IP.h"

/*======================================================================================================================
 *  [SWS_Mem_00033] kiểm tra cấu hình tĩnh / [SWS_Mem_00060] ghi chú multi-instance
 *  Flash_IP.c mô hình đúng một thiết bị vật lý: Flash bank nội duy nhất của STM32F401RE (một bộ thanh ghi
 *  FLASH_CR/FLASH_SR, một bộ biến trạng thái tĩnh của module). [SWS_Mem_00060] yêu cầu
 *  the Mem driver to "support multiple instances of the same memory device" - that requirement is
 *  Requirement này chỉ đáp ứng được với bộ nhớ EXTERNAL có nhiều chip vật lý giống nhau (ví dụ hai SPI Flash
 *  IC giống nhau), không thể áp dụng cho Flash controller nội duy nhất của MCU. Nếu MEM_INSTANCE_COUNT > 1
 *  nhưng mọi instance vẫn dùng cùng Flash_IP, chúng sẽ trỏ vào cùng bộ nhớ vật lý và làm hỏng job của nhau.
 *  Vì vậy cần lỗi ở compile time thay vì runtime. Để thêm Mem driver instance thứ hai, hãy dùng cặp
 *  Mem_IPW/xxx_IP KHÁC (ví dụ driver SPI Flash ngoài), không phải bản sao thứ hai của driver này. */
#if (MEM_INSTANCE_COUNT > 1u)
#error "Mem_IPW.c backs every Mem driver instance with the single STM32F401RE internal Flash bank (Flash_IP.c). MEM_INSTANCE_COUNT must be 1 - see the SWS_Mem_00060 note in Mem_IPW.c."
#endif

/*======================================================================================================================
 *  Trạng thái cục bộ
 *====================================================================================================================*/
typedef enum
{
    MEM_IPW_RESULT_SRC_NONE = 0,
    MEM_IPW_RESULT_SRC_FLASH_IP,  /* kết quả từ Flash_IP_GetStatus() - Program / Erase (bất đồng bộ)     */
    MEM_IPW_RESULT_SRC_LOCAL      /* kết quả đã tính đồng bộ - Read / BlankCheck                           */
} Mem_Ipw_ResultSourceType;

static Mem_Ipw_ResultSourceType Mem_Ipw_ResultSource[MEM_INSTANCE_COUNT];
static MemAcc_MemJobResultType  Mem_Ipw_LocalResult[MEM_INSTANCE_COUNT];

/*======================================================================================================================
 *  Hàm hỗ trợ cục bộ
 *====================================================================================================================*/

/* IPW này hiện chỉ hỗ trợ một instance (instanceId == 0, Flash nội).
 * Dùng hàm thay vì macro để dễ mở rộng nếu sau này thêm Mem driver instance (ví dụ cặp
 * Mem_IPW/Flash_IP cho SPI Flash ngoài). */
static boolean Mem_Ipw_IsInstanceSupported(Mem_InstanceIdType instanceId)
{
    return (boolean)(instanceId < (Mem_InstanceIdType)MEM_INSTANCE_COUNT);
}

boolean Mem_Ipw_IsHwSpecificServiceSupported(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId)
{
    (void)hwServiceId;

    return (boolean)((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) && FALSE);
}

boolean Mem_Ipw_IsSuspendResumeSupported(Mem_InstanceIdType instanceId)
{
    return (boolean)((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) && FALSE);
}

static MemAcc_MemJobResultType Mem_Ipw_MapFlashIpStatus(Flash_IP_StatusType status)
{
    MemAcc_MemJobResultType result;

    switch (status)
    {
        case FLASH_IP_BUSY:
            result = MEM_JOB_PENDING;
            break;
        case FLASH_IP_OK:
            result = MEM_JOB_OK;
            break;
        case FLASH_IP_ERROR:
            result = MEM_JOB_FAILED;
            break;
        case FLASH_IP_IDLE:
        default:
            result = MEM_JOB_OK;
            break;
    }

    return result;
}

/*======================================================================================================================
 *  Vòng đời
 *====================================================================================================================*/

void Mem_Ipw_Init(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_Init();

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_NONE;
        Mem_Ipw_LocalResult[instanceId]  = MEM_JOB_OK;
    }
}

void Mem_Ipw_DeInit(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_DeInit(); /* [SWS_Mem_00079] */

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_NONE;
    }
}

void Mem_Ipw_Cancel(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_Cancel();
        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_NONE;
        Mem_Ipw_LocalResult[instanceId]  = MEM_JOB_FAILED;
    }
}

/*======================================================================================================================
 *  Thao tác bộ nhớ bất đồng bộ
 *====================================================================================================================*/

Std_ReturnType Mem_Ipw_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* Flash là memory-mapped nên thao tác đọc hoàn tất đồng bộ. Kết quả vẫn chỉ được Mem.c/
         * Mem_MainFunction() sử dụng ở chu kỳ sau, phù hợp API bất đồng bộ [SWS_Mem_10012]. */
        retVal = Flash_IP_Read((uint32)sourceAddress, destinationDataPtr, (uint32)length);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_LOCAL;
            Mem_Ipw_LocalResult[instanceId]  = MEM_JOB_OK; /* [SWS_Mem_00067] */
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        retVal = Flash_IP_ProgramStart((uint32)targetAddress, sourceDataPtr, (uint32)length);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_FLASH_IP;
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    /* Căn chỉnh đã được Mem.c kiểm tra đồng bộ bằng Mem_CheckEraseAlignment(), hàm này gọi
     * Mem_Ipw_IsEraseAligned(), trước khi trigger này được gọi từ Mem_MainFunction(). Kiểm tra lặp lại ở đây
     * để phòng vệ với chi phí thấp và bảo vệ các nơi gọi khác của lớp IPW. */
    if ((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) &&
        (Mem_Ipw_IsEraseAligned(instanceId, targetAddress, length) == TRUE))
    {
        uint8 sector = Flash_IP_GetSectorFromAddress((uint32)targetAddress);

        retVal = Flash_IP_EraseSectorStart(sector);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_FLASH_IP;
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* Flash là memory-mapped và MCU này không có phần cứng blank-check chuyên dụng, nên kiểm tra
         * được thực hiện đồng bộ bằng cách đọc lại và so sánh với giá trị đã xóa. */
        uint32  i;
        uint8   byte;
        boolean isBlank = TRUE;

        for (i = 0u; i < (uint32)length; i++)
        {
            (void)Flash_IP_Read((uint32)targetAddress + i, &byte, 1u);

            if (byte != (uint8)FLASH_IP_ERASED_VALUE)
            {
                isBlank = FALSE;
                break;
            }
        }

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_LOCAL;
        Mem_Ipw_LocalResult[instanceId]  = (isBlank == TRUE) ? MEM_JOB_OK : MEM_INCONSISTENT; /* [SWS_Mem_00076] */

        retVal = E_OK;
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr)
{
    /* Reference port này chưa hiện thực hardware-specific service nào, ví dụ read protection level,
     * option byte hay unique ID 96-bit tại 0x1FFF7A10; xem [SWS_Mem_00070]. Khi cần, hãy thêm trường hợp
     * theo hwServiceId trong Flash_IP.c và điều phối tại đây. */
    (void)dataPtr;
    (void)lengthPtr;

    if (Mem_Ipw_IsHwSpecificServiceSupported(instanceId, hwServiceId) == FALSE)
    {
        return E_MEM_SERVICE_NOT_AVAIL;
    }

    return E_NOT_OK;
}

/*======================================================================================================================
 *  Suspend / Resume
 *  STM32F401 (single Flash bank) không có cơ chế phần cứng suspend/resume cho thao tác program/erase.
 *  Khả năng này chỉ có trên dòng STM32 dual-bank hoặc mới hơn; xem [SWS_Mem_00082].
 *====================================================================================================================*/

Std_ReturnType Mem_Ipw_Suspend(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsSuspendResumeSupported(instanceId) == FALSE)
    {
        return E_MEM_SERVICE_NOT_AVAIL;
    }

    return E_NOT_OK;
}

Std_ReturnType Mem_Ipw_Resume(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsSuspendResumeSupported(instanceId) == FALSE)
    {
        return E_MEM_SERVICE_NOT_AVAIL;
    }

    return E_NOT_OK;
}

/*======================================================================================================================
 *  Lập lịch / lấy kết quả job
 *====================================================================================================================*/

void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_MainFunction();
    }
}

MemAcc_MemJobResultType Mem_Ipw_GetJobResult(Mem_InstanceIdType instanceId)
{
    MemAcc_MemJobResultType result = MEM_JOB_FAILED;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        if (Mem_Ipw_ResultSource[instanceId] == MEM_IPW_RESULT_SRC_LOCAL)
        {
            result = Mem_Ipw_LocalResult[instanceId];
        }
        else
        {
            result = Mem_Ipw_MapFlashIpStatus(Flash_IP_GetStatus());
        }
    }

    return result;
}

/*======================================================================================================================
 *  Hàm hỗ trợ kiểm tra địa chỉ / độ dài
 *====================================================================================================================*/

boolean Mem_Ipw_IsAddressValid(Mem_InstanceIdType instanceId, Mem_AddressType address)
{
    boolean valid = FALSE;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        valid = (boolean)((address >= (Mem_AddressType)FLASH_IP_BASE_ADDRESS) &&
                           (address <  (Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE)));
    }

    return valid;
}

boolean Mem_Ipw_IsLengthValid(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length)
{
    boolean valid = FALSE;

    if ((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) &&
        (length > 0u) &&
        (Mem_Ipw_IsAddressValid(instanceId, address) == TRUE))
    {
        const Mem_AddressType flashEnd =
                (Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE);

        /* Subtraction form avoids address + length wraparound. */
        valid = (boolean)((Mem_AddressType)length <= (flashEnd - address));
    }

    return valid;
}

boolean Mem_Ipw_IsEraseAligned(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length)
{
    boolean valid = FALSE;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* Flash controller STM32F401RE chỉ xóa được cả sector. Vì vậy, Mem_Erase() hợp lệ phải có cặp
         * địa chỉ/độ dài khớp chính xác một sector đã cấu hình (xem Flash_IP_Cfg.c). Theo [SWS_Mem_00035],
         * MemAcc thường tách yêu cầu xóa logic lớn thành các yêu cầu theo sector. Dự án này không có MemAcc,
         * do đó nơi gọi Mem_Erase() phải gửi một yêu cầu đúng kích thước cho từng sector. */
        uint8 sector = Flash_IP_GetSectorFromAddress((uint32)address);

        valid = (boolean)((sector != 0xFFu) &&
                           (address == (Mem_AddressType)Flash_IP_GetSectorStartAddress(sector)) &&
                           (length  == (Mem_LengthType)Flash_IP_GetSectorSize(sector)));
    }

    return valid;
}

boolean Mem_Ipw_IsWriteAligned(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length,
        uint8*              errorId)
{
    const Mem_AddressType alignment = (Mem_AddressType)FLASH_IP_WRITE_ALIGNMENT;

    if ((Mem_Ipw_IsInstanceSupported(instanceId) == FALSE) ||
        (errorId == NULL_PTR) ||
        (alignment == 0u))
    {
        return FALSE;
    }

    if ((address % alignment) != 0u)
    {
        *errorId = MEM_E_PARAM_ADDRESS;
        return FALSE;
    }

    if ((length % (Mem_LengthType)alignment) != 0u)
    {
        *errorId = MEM_E_PARAM_LENGTH;
        return FALSE;
    }

    *errorId = 0u;
    return TRUE;
}
