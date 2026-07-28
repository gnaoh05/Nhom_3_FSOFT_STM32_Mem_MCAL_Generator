#include "Mem_IPW.h"

/**
 * @brief  Chuyển đổi kết quả từ Flash_IP sang chuẩn của tầng Mem_IPW
 */
static Mem_IPW_JobResultType Mem_IPW_ConvertResult(Flash_IP_JobResultType flashResult) {
    Mem_IPW_JobResultType ipwResult;

    switch (flashResult) {
        case FLASH_IP_JOB_OK:
            ipwResult = MEM_IPW_JOB_OK;
            break;

        case FLASH_IP_WRITE_PROTECT_ERROR:
            ipwResult = MEM_IPW_WRITE_PROTECT_ERR;
            break;

        case FLASH_IP_ALIGNMENT_ERROR:
            ipwResult = MEM_IPW_ALIGNMENT_ERR;
            break;

        case FLASH_IP_JOB_FAILED:
        default:
            ipwResult = MEM_IPW_JOB_FAILED;
            break;
    }

    return ipwResult;
}


void Mem_IPW_Init(void) {
    /* Ủy quyền khởi tạo phần cứng cho driver Flash IP */
    Flash_IP_Init();
}

Mem_IPW_JobResultType Mem_IPW_Read(uint32 address, uint8 *targetPtr, uint32 length) {
    Flash_IP_JobResultType flashResult;

    if (targetPtr == NULL_PTR) {
        return MEM_IPW_JOB_FAILED;
    }

    /* Gọi API đọc bên dưới Flash IP */
    flashResult = Flash_IP_Read(address, targetPtr, length);

    /* Chuyển đổi và trả về kết quả */
    return Mem_IPW_ConvertResult(flashResult);
}

Mem_IPW_JobResultType Mem_IPW_Write(uint32 address, const uint8 *sourcePtr, uint32 length) {
    Flash_IP_JobResultType flashResult;

    if (sourcePtr == NULL_PTR) {
        return MEM_IPW_JOB_FAILED;
    }

    /* Gọi API ghi bên dưới Flash IP */
    flashResult = Flash_IP_Write(address, sourcePtr, length);

    /* Chuyển đổi và trả về kết quả */
    return Mem_IPW_ConvertResult(flashResult);
}

Mem_IPW_JobResultType Mem_IPW_Erase(uint8 sectorNum) {
    Flash_IP_JobResultType flashResult;

    /* Gọi API xóa Sector bên dưới Flash IP */
    flashResult = Flash_IP_Erase(sectorNum);

    /* Chuyển đổi và trả về kết quả */
    return Mem_IPW_ConvertResult(flashResult);
}