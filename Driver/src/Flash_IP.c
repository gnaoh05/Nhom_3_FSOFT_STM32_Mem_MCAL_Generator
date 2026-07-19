#include "Flash_IP.h"
#include "stddef.h"

/* Cơ sở địa chỉ thanh ghi Flash trên STM32F401RE */
#define FLASH_BASE_ADDR   (0x40023C00UL)

typedef struct {
    volatile uint32_t ACR;      
    volatile uint32_t KEYR;     
    volatile uint32_t OPTKEYR;  
    volatile uint32_t SR;       
    volatile uint32_t CR;       
    volatile uint32_t OPTCR;    
} Flash_RegType;

#define FLASH_REG    ((Flash_RegType*)FLASH_BASE_ADDR)

/* Định nghĩa các bit mặt nạ hệ thống */
#define FLASH_CR_LOCK       (1U << 31)
#define FLASH_CR_PG         (1U << 0)
#define FLASH_CR_SER        (1U << 1)
#define FLASH_CR_STRT       (1U << 16)

#define FLASH_SR_BSY        (1U << 16)
#define FLASH_SR_EOP        (1U << 0)
#define FLASH_SR_WRPERR     (1U << 4)  /* Write protection error */
#define FLASH_SR_PGAERR     (1U << 5)  /* Programming alignment error */

#define FLASH_KEY1          (0x45670123UL)
#define FLASH_KEY2          (0xCDEF89ABUL)

static Flash_IP_StatusType Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;

/* Hàm nội bộ: Mở khóa Flash bằng mã Key phần cứng */
static void Flash_IP_Unlock(void) {
    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U) {
        FLASH_REG->KEYR = FLASH_KEY1;
        FLASH_REG->KEYR = FLASH_KEY2;
    }
}

/* Hàm nội bộ: Khóa bảo vệ Flash */
static void Flash_IP_Lock(void) {
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

/* Hàm nội bộ: Đợi phần cứng hết bận và kiểm tra lỗi thanh ghi SR */
static Flash_IP_JobResultType Flash_IP_GetStatus(void) {
    // Đợi bit BSY hạ về 0
    while ((FLASH_REG->SR & FLASH_SR_BSY) != 0U);

    // Kiểm tra lỗi bảo vệ vùng nhớ (Ghi vào vùng cấm)
    if ((FLASH_REG->SR & FLASH_SR_WRPERR) != 0U) {
        FLASH_REG->SR = FLASH_SR_WRPERR; // Xóa cờ lỗi
        return FLASH_IP_WRITE_PROTECT_ERROR;
    }
    // Kiểm tra lỗi căn lề địa chỉ
    if ((FLASH_REG->SR & FLASH_SR_PGAERR) != 0U) {
        FLASH_REG->SR = FLASH_SR_PGAERR; // Xóa cờ lỗi
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    return FLASH_IP_JOB_OK;
}

void Flash_IP_Init(void) {
    /* Cấu hình 2 Wait States cho thanh ghi ACR phù hợp xung nhịp 84MHz */
    FLASH_REG->ACR &= ~7U;
    FLASH_REG->ACR |= 2U; 
    
    /* Bật tính năng Prefetch, Instruction Cache, Data Cache */
    FLASH_REG->ACR |= (1U << 8) | (1U << 9) | (1U << 10);
    
    Flash_IP_DriverState = FLASH_IP_INITIALIZED;
}

void Flash_IP_DeInit(void) {
    Flash_IP_Lock();
    /* Reset toàn bộ thanh ghi ACR về trạng thái mặc định của chip */
    FLASH_REG->ACR &= ~((1U << 8) | (1U << 9) | (1U << 10) | 7U);
    Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
}

Flash_IP_JobResultType Flash_IP_Read(uint32_t address, uint8_t *targetPtr, uint32_t length) {
    if (Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) {
        return FLASH_IP_JOB_FAILED;
    }

    /* Đọc tuyến tính theo Byte, tầng này không cần quan tâm ép kiểu Word */
    for (uint32_t i = 0; i < length; i++) {
        targetPtr[i] = *(volatile uint8_t*)(address + i);
    }
    
    return FLASH_IP_JOB_OK;
}

Flash_IP_JobResultType Flash_IP_Erase(uint8_t sectorNum) {
    Flash_IP_JobResultType result;

    result = Flash_IP_GetStatus();
    if (result != FLASH_IP_JOB_OK) return result;

    Flash_IP_Unlock();

    /* Thiết lập các bit thanh ghi CR để xóa Sector */
    FLASH_REG->CR &= ~(0xFU << 3);              
    FLASH_REG->CR |= (uint32_t)(sectorNum << 3); 
    FLASH_REG->CR |= FLASH_CR_SER;              
    FLASH_REG->CR |= FLASH_CR_STRT;             // Phát lệnh kích hoạt mạch xóa nội bộ

    result = Flash_IP_GetStatus();              // Chờ xóa xong và lấy trạng thái lỗi
    
    FLASH_REG->CR &= ~FLASH_CR_SER;             
    Flash_IP_Lock();

    return result;
}

Flash_IP_JobResultType Flash_IP_Write(uint32_t address, const uint8_t *sourcePtr, uint32_t length) {
    Flash_IP_JobResultType result;

    result = Flash_IP_GetStatus();
    if (result != FLASH_IP_JOB_OK) return result;

    Flash_IP_Unlock();

    FLASH_REG->CR |= FLASH_CR_PG;               // Bật chế độ Program
    FLASH_REG->CR &= ~(3U << 8);                
    FLASH_REG->CR |= (2U << 8);                 // Cấu hình PSIZE = 32-bit (Word)

    /* Vì PSIZE cấu hình ghi theo từng Word 4-byte, ta sẽ ép kiểu ép xung ghi theo cụm 4 byte */
    uint32_t wordLength = length / 4;
    const uint32_t *wordSrcPtr = (const uint32_t*)sourcePtr;

    for (uint32_t i = 0; i < wordLength; i++) {
        *(volatile uint32_t*)(address + (i * 4)) = wordSrcPtr[i];
        result = Flash_IP_GetStatus();
        if (result != FLASH_IP_JOB_OK) {
            break;
        }
    }

    FLASH_REG->CR &= ~FLASH_CR_PG;              
    Flash_IP_Lock();

    return result;
}