#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Std_Types.h"
#include "Mem_Types.h"
#include "Mem_Cfg.h"

/* Trạng thái nội bộ của tầng IPW */
typedef enum {
    MEM_IPW_IDLE = 0U,
    MEM_IPW_BUSY = 1U,
    MEM_IPW_ERROR = 2U  /* Đã thêm dấu phẩy ở dòng trên và giá trị 2U */
} Mem_Ipw_StatusType;

/* ============================================================================
 * NGUYÊN MẪU HÀM TẦNG MEM_IPW
 * ============================================================================ */

/**
 * @brief Khởi tạo phần cứng Flash thông qua Flash_IP Driver
 */
void Mem_Ipw_Init(const Mem_ConfigType* ConfigPtr);

/**
 * @brief Lấy trạng thái bận/rảnh của phần cứng Flash
 */
Mem_Ipw_StatusType Mem_Ipw_GetStatus(Mem_InstanceIdType InstanceId);

/**
 * @brief Đọc dữ liệu trực tiếp từ bus bộ nhớ Flash
 */
Std_ReturnType Mem_Ipw_Read(Mem_InstanceIdType InstanceId, 
                            Mem_AddressType Address, 
                            Mem_DataType* DataPtr, 
                            Mem_LengthType Length);

/**
 * @brief Đẩy lệnh ghi khối dữ liệu xuống phần cứng Flash
 */
Std_ReturnType Mem_Ipw_Write(Mem_InstanceIdType InstanceId, 
                             Mem_AddressType Address, 
                             const Mem_DataType* DataPtr, 
                             Mem_LengthType Length);

/**
 * @brief Chuyển đổi Địa chỉ sang Sector ID và kích hoạt xóa Sector phần cứng
 */
Std_ReturnType Mem_Ipw_Erase(Mem_InstanceIdType InstanceId, 
                             Mem_AddressType Address, 
                             Mem_LengthType Length);

/**
 * @brief Kiểm tra xem vùng nhớ Flash có hoàn toàn rỗng (0xFF) hay không
 */
Std_ReturnType Mem_Ipw_BlankCheck(Mem_InstanceIdType InstanceId, 
                                  Mem_AddressType Address, 
                                  Mem_LengthType Length);

/**
 * @brief Hủy tác vụ phần cứng nếu đang thực thi
 */
void Mem_Ipw_Cancel(Mem_InstanceIdType InstanceId);

/**
 * @brief Hàm chu kỳ ngầm của IPW
 */
void Mem_Ipw_MainFunction(Mem_InstanceIdType InstanceId);


#endif /* MEM_IPW_H */