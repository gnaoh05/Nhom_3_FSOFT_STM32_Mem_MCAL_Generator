#include "Mem_IPW.h"
#include "Flash_IP.h"

#define MEM_IPW_TIMEOUT_TICKS (50000U) /* Số chu kỳ gọi MainFunction tối đa chờ phần cứng */
static uint32 Mem_Ipw_TimeoutCounters[MEM_MAX_INSTANCES];

/**
 * @brief Khởi tạo phần cứng Flash và Mở khóa thanh ghi CR
 */
void Mem_Ipw_Init(const Mem_ConfigType* ConfigPtr) 
{
    uint8 i;
    (void)ConfigPtr; /* Trong cấu hình Pre-Compile, tham số này là NULL */
    
    Flash_IP_Init();
    Flash_IP_Unlock();

    for (i = 0; i < MEM_MAX_INSTANCES; i++) 
    {
        Mem_Ipw_TimeoutCounters[i] = 0U;
    }
}

/**
 * @brief Hỏi trạng thái phần cứng từ Flash_IP
 */
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType InstanceId) 
{
    Flash_IP_JobResultType hwStatus = Flash_IP_GetStatus();
    
    /* Báo lỗi nếu phần cứng treo quá lâu (Timeout) */
    if (Mem_Ipw_TimeoutCounters[InstanceId] >= MEM_IPW_TIMEOUT_TICKS) 
    {
        return MEM_IPW_ERROR;
    }

    if (hwStatus == FLASH_IP_JOB_BUSY) 
    {
        return MEM_IPW_BUSY;
    }
    else if (hwStatus == FLASH_IP_JOB_OK)
    {
        return MEM_IPW_IDLE;
    }
    else
    {
        return MEM_IPW_ERROR;
    }
}

/**
 * @brief Thực hiện đọc dữ liệu tuyến tính
 */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType InstanceId, 
                             Mem_AddressType Address, 
                             Mem_DataType* DataPtr, 
                             Mem_LengthType Length) 
{
    (void)InstanceId;
    
    Flash_IP_JobResultType res = Flash_IP_Read(Address, DataPtr, Length);
    
    if (res == FLASH_IP_JOB_OK) 
    {
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief Đẩy lệnh ghi khối dữ liệu xuống Flash_IP
 */
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType InstanceId, 
                              Mem_AddressType Address, 
                              const Mem_DataType* DataPtr, 
                              Mem_LengthType Length) 
{
    (void)InstanceId;
    
    /* Ghi khối dữ liệu 32-bit Word xuống Flash IP */
    Flash_IP_JobResultType res = Flash_IP_Write(Address, DataPtr, Length);
    
    if (res == FLASH_IP_JOB_BUSY) 
    {
        return E_OK; /* Báo cho tầng Mem ở trên biết lệnh đã kích hoạt thành công */
    }
    
    return E_NOT_OK;
}

/**
 * @brief Chuyển đổi Địa chỉ vật lý (Address) -> SectorNum (0..7) và phát lệnh Xóa
 */
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType InstanceId, 
                              Mem_AddressType Address, 
                              Mem_LengthType Length) 
{
    (void)InstanceId;
    (void)Length; /* Tầng Mem ở trên đã xác thực độ dài hợp lệ bằng Mem_ValidateAddressAndLength */

    /* Map từ địa chỉ dạng byte sang Sector ID của phần cứng */
    uint8 sectorNum = Flash_IP_GetSectorFromAddress(Address);

    if (sectorNum != 0xFFU) 
    {
        Flash_IP_JobResultType res = Flash_IP_Erase(sectorNum);
        
        if (res == FLASH_IP_JOB_BUSY) 
        {
            return E_OK; /* Lệnh xóa đã được phát xuống phần cứng */
        }
    }

    return E_NOT_OK;
}

/**
 * @brief Kiểm tra dữ liệu rỗng (0xFF)
 */
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType InstanceId, 
                                   Mem_AddressType Address, 
                                   Mem_LengthType Length) 
{
    (void)InstanceId;
    
    Flash_IP_JobResultType res = Flash_IP_BlankCheck(Address, Length);
    
    if (res == FLASH_IP_JOB_OK) 
    {
        return E_OK; /* Vùng nhớ hoàn toàn rỗng */
    }
    
    return E_NOT_OK; /* Vùng nhớ chứa dữ liệu hoặc bị lỗi */
}

/**
 * @brief Khóa thanh ghi Flash để hủy bỏ tác vụ nghi vấn
 */
void Mem_Ipw_Cancel(Mem_InstanceIdType InstanceId) 
{
    (void)InstanceId;
    
    /* Ghi chú: STM32 Flash Controller không hỗ trợ Cancel khi đã phát lệnh.
     * Tầng Mem sẽ đánh dấu Job là FAILED/CANCELLED và không đẩy chunk tiếp theo.
     * Do đó, tại IPW chúng ta không lock ngang thanh ghi phần cứng. */
}

/**
 * @brief Hàm chu kỳ nội bộ của IPW
 */
void Mem_Ipw_MainFunction(Mem_InstanceIdType InstanceId) 
{
    Flash_IP_JobResultType hwStatus = Flash_IP_GetStatus();

    /* Nếu phần cứng đang bận xử lý (Ghi/Xóa), ta tăng biến đếm thời gian Timeout */
    if (hwStatus == FLASH_IP_JOB_BUSY) 
    {
        if (Mem_Ipw_TimeoutCounters[InstanceId] < MEM_IPW_TIMEOUT_TICKS) 
        {
            Mem_Ipw_TimeoutCounters[InstanceId]++;
        }
    }
    else
    {
        /* Khi phần cứng rảnh rỗi hoặc xong việc, lập tức reset biến đếm */
        Mem_Ipw_TimeoutCounters[InstanceId] = 0U;
    }
}