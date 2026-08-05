#include "Mem_IPW.h"
#include "Flash_IP.h"

/**
 * @brief Khởi tạo phần cứng Flash và Mở khóa thanh ghi CR
 */
void Mem_IPW_Init(const Mem_ConfigType* ConfigPtr) 
{
    (void)ConfigPtr; /* Trong cấu hình Pre-Compile, tham số này là NULL */
    
    Flash_IP_Init();
    Flash_IP_Unlock();
}

/**
 * @brief Hỏi trạng thái phần cứng từ Flash_IP
 */
Mem_Ipw_StatusType Mem_IPW_GetStatus(Mem_InstanceIdType InstanceId) 
{
    (void)InstanceId;
    
    Flash_IP_JobResultType hwStatus = Flash_IP_GetStatus();
    
    if (hwStatus == FLASH_IP_JOB_BUSY) 
    {
        return MEM_IPW_BUSY;
    }
    
    return MEM_IPW_IDLE;
}

/**
 * @brief Thực hiện đọc dữ liệu tuyến tính
 */
Std_ReturnType Mem_IPW_Read(Mem_InstanceIdType InstanceId, 
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
Std_ReturnType Mem_IPW_Write(Mem_InstanceIdType InstanceId, 
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
Std_ReturnType Mem_IPW_Erase(Mem_InstanceIdType InstanceId, 
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
Std_ReturnType Mem_IPW_BlankCheck(Mem_InstanceIdType InstanceId, 
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
void Mem_IPW_Cancel(Mem_InstanceIdType InstanceId) 
{
    (void)InstanceId;
    
    /* Khóa thanh ghi điều khiển Flash để bảo vệ bộ nhớ */
    Flash_IP_Lock();
    Flash_IP_Unlock(); /* Reset lại trạng thái sẵn sàng */
}

/**
 * @brief Hàm chu kỳ nội bộ của IPW
 */
void Mem_IPW_MainFunction(Mem_InstanceIdType InstanceId) 
{
    (void)InstanceId;
    /* Dành cho mở rộng các tác vụ quét phần cứng theo chu kỳ nếu cần */
}