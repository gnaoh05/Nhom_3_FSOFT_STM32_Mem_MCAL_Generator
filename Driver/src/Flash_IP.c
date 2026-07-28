#include "Flash_IP.h"
#include "Std_Types.h"

/* Cơ sở địa chỉ thanh ghi Flash trên STM32F401RE */
#define FLASH_BASE_ADDR     (0x40023C00UL)

typedef struct {
    __IO uint32 ACR;      
    __IO uint32 KEYR;     
    __IO uint32 OPTKEYR;  
    __IO uint32 SR;       
    __IO uint32 CR;       
    __IO uint32 OPTCR;    
} Flash_RegType;

#define FLASH_REG    ((Flash_RegType*)FLASH_BASE_ADDR)

/* Định nghĩa các bit trong thanh ghi FLASH_CR */
#define FLASH_CR_LOCK       (1U << 31)
#define FLASH_CR_STRT       (1U << 16)
#define FLASH_CR_SER        (1U << 1)
#define FLASH_CR_PG         (1U << 0)

/* Định nghĩa các bit trong thanh ghi FLASH_SR */
#define FLASH_SR_BSY        (1U << 16)
#define FLASH_SR_WRPERR     (1U << 4)  /* Write protection error */
#define FLASH_SR_PGAERR     (1U << 5)  /* Programming alignment error */
#define FLASH_SR_PGPERR     (1U << 6)  /* Programming parallelism error */
#define FLASH_SR_PGSERR     (1U << 7)  /* Programming sequence error */

/* Chìa khóa mở khóa Flash */
#define FLASH_KEY1          (0x45670123UL)
#define FLASH_KEY2          (0xCDEF89ABUL)

/* Giới hạn vòng lặp Timeout chống treo vi điều khiển */
#define FLASH_IP_TIMEOUT    (0x000FFFFFUL)

static Flash_IP_StatusType Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;

/**
 * @brief  Hàm Wait chuẩn phong cách HAL: Chờ cờ BSY hạ về 0 và kiểm tra/clear lỗi SR
 * @param  timeout: Số vòng lặp chờ tối đa
 * @return Kết quả thực thi công việc (JobResult)
 */
static Flash_IP_JobResultType Flash_IP_WaitForLastOperation(uint32 timeout) {
    /* 1. Chờ cờ BSY (Busy) hạ về 0 hoặc hết thời gian Timeout */
    while (((FLASH_REG->SR & FLASH_SR_BSY) != 0U) && (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        return FLASH_IP_JOB_FAILED; /* Lỗi Timeout phần cứng */
    }

    /* 2. Kiểm tra lỗi Write Protection Error (WRPERR) */
    if ((FLASH_REG->SR & FLASH_SR_WRPERR) != 0U) {
        /* Clear flag bằng cách ghi 1 vào bit lỗi */
        FLASH_REG->SR = FLASH_SR_WRPERR;
        return FLASH_IP_WRITE_PROTECT_ERROR;
    }

    /* 3. Kiểm tra các lỗi căn lề / cấu hình nạp phần cứng (PGAERR | PGPERR | PGSERR) */
    if ((FLASH_REG->SR & (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)) != 0U) {
        /* Clear toàn bộ cờ lỗi */
        FLASH_REG->SR = (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR);
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    return FLASH_IP_JOB_OK;
}

/**
 * @brief Mở khóa Flash bằng mã Key phần cứng
 */
static void Flash_IP_Unlock(void) {
    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U) {
        FLASH_REG->KEYR = FLASH_KEY1;
        FLASH_REG->KEYR = FLASH_KEY2;
    }
}

/**
 * @brief Khóa bảo vệ Flash
 */
static void Flash_IP_Lock(void) {
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

/**
 * @brief Khởi tạo Flash IP Driver & Cấu hình Latency (2 Wait States) cho 84MHz
 */
void Flash_IP_Init(void) {
    /* Cấu hình 2 Wait States cho thanh ghi ACR phù hợp xung nhịp 84MHz */
    FLASH_REG->ACR &= ~15U;
    FLASH_REG->ACR |= 2U; 
    
    /* Bật tính năng Prefetch, Instruction Cache, Data Cache */
    FLASH_REG->ACR |= (1U << 8) | (1U << 9) | (1U << 10);
    
    Flash_IP_DriverState = FLASH_IP_INITIALIZED;
}

/**
 * @brief Hủy khởi tạo Flash IP Driver & Khóa bảo vệ Flash
 */
void Flash_IP_DeInit(void) {
    Flash_IP_Lock();
    /* Reset toàn bộ thanh ghi ACR về trạng thái mặc định của chip */
    FLASH_REG->ACR &= ~((1U << 8) | (1U << 9) | (1U << 10) | 15U);
    Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
}

/**
 * @brief Đọc tuyến tính theo Byte từ Bus Flash Memory
 */
Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length) {
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (targetPtr == NULL_PTR)) {
        return FLASH_IP_JOB_FAILED;
    }

    /* Đọc trực tiếp từ địa chỉ Flash */
    for (uint32 i = 0U; i < length; i++) {
        targetPtr[i] = *(__IO uint8*)(address + i);
    }
    
    return FLASH_IP_JOB_OK;
}

/**
 * @brief Xóa 1 Sector bộ nhớ Flash
 */
Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum) {
    Flash_IP_JobResultType status;

    if (Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) {
        return FLASH_IP_JOB_FAILED;
    }

    /* 1. Đảm bảo Flash rảnh trước khi thao tác */
    status = Flash_IP_WaitForLastOperation(FLASH_IP_TIMEOUT);
    if (status != FLASH_IP_JOB_OK) {
        return status;
    }

    Flash_IP_Unlock();

    /* 2. Thiết lập Sector cần xóa trong thanh ghi CR */
    FLASH_REG->CR &= ~(0xFU << 3);              
    FLASH_REG->CR |= (uint32)((sectorNum & 0x0FU) << 3); 
    FLASH_REG->CR |= FLASH_CR_SER;              
    
    /* 3. Phát lệnh kích hoạt mạch xóa nội bộ (STRT) */
    FLASH_REG->CR |= FLASH_CR_STRT;             

    /* 4. Chờ xóa xong và lấy kết quả lỗi (tương tự HAL_FLASHEx_Erase) */
    status = Flash_IP_WaitForLastOperation(FLASH_IP_TIMEOUT);              
    
    /* 5. Tắt bit SER và Lock lại Flash */
    FLASH_REG->CR &= ~FLASH_CR_SER;             
    Flash_IP_Lock();

    return status;
}

/**
 * @brief Ghi dữ liệu vào Flash (Ghi theo Word 32-bit PSIZE=2)
 */
Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length) {
    Flash_IP_JobResultType status;

    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sourcePtr == NULL_PTR)) {
        return FLASH_IP_JOB_FAILED;
    }

    /* Đảm bảo địa chỉ ghi phải được căn lề 4-Byte (Word Aligned) */
    if ((address % 4U) != 0U) {
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    /* 1. Đảm bảo Flash rảnh trước khi thao tác */
    status = Flash_IP_WaitForLastOperation(FLASH_IP_TIMEOUT);
    if (status != FLASH_IP_JOB_OK) {
        return status;
    }

    Flash_IP_Unlock();

    FLASH_REG->CR &= ~(3U << 8); 
    FLASH_REG->CR |= (2U << 8);                 // Cấu hình PSIZE = 32-bit (x32 Program)
    FLASH_REG->CR |= FLASH_CR_PG;               // Bật chế độ Program

    uint32 wordLength = length / 4U;
    uint32 remainder = length % 4U;
    uint32 currentAddr = address;
    uint32 dataWord = 0U;

    /* 2. Xử lý ghi các Word tròn 4-Byte */
    for (uint32 i = 0U; i < wordLength; i++) {
        // Ghép 4 byte độc lập vào 1 Word để tránh lỗi Unaligned Pointer Access
        dataWord = ((uint32)sourcePtr[(i * 4U) + 0U])       |
                   (((uint32)sourcePtr[(i * 4U) + 1U]) << 8)  |
                   (((uint32)sourcePtr[(i * 4U) + 2U]) << 16) |
                   (((uint32)sourcePtr[(i * 4U) + 3U]) << 24);

        *(__IO uint32*)currentAddr = dataWord;
        
        /* Gọi Wait ngay sau mỗi Word được nạp (tương tự HAL_FLASH_Program) */
        status = Flash_IP_WaitForLastOperation(FLASH_IP_TIMEOUT);
        if (status != FLASH_IP_JOB_OK) {
            break;
        }
        currentAddr += 4U;
    }

    /* 3. Xử lý phần byte lẻ còn dư (Remainder < 4 Byte) */
    if ((status == FLASH_IP_JOB_OK) && (remainder > 0U)) {
        dataWord = 0xFFFFFFFFUL; // Padding 0xFF cho các byte không dùng đến
        uint32 offset = wordLength * 4U;

        for (uint32 j = 0U; j < remainder; j++) {
            dataWord &= ~(0xFFUL << (j * 8U));
            dataWord |= ((uint32)sourcePtr[offset + j]) << (j * 8U);
        }

        *(__IO uint32*)currentAddr = dataWord;
        
        /* Chờ nạp Word lẻ cuối cùng */
        status = Flash_IP_WaitForLastOperation(FLASH_IP_TIMEOUT);
    }

    FLASH_REG->CR &= ~FLASH_CR_PG;              
    Flash_IP_Lock();

    return status;
}