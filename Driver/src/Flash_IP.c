#include "Flash_IP.h"
#include "Flash_IP_Cfg.h"

#ifndef FLASH_IP_LATENCY_SETTING
#define FLASH_IP_LATENCY_SETTING    (2U) /* 2 Wait States */
#endif

#define FLASH_BASE_ADDR           (0x40023C00UL)

/* Cấu hình số lần poll BSY tối đa trước khi hủy job do Hardware Timeout */
#define FLASH_IP_TIMEOUT_MAX_TICKS (1000000UL) 

typedef volatile uint32_t __IO_UINT32;
typedef volatile uint8_t  __IO_UINT8;

typedef struct 
{
    __IO_UINT32 ACR;      
    __IO_UINT32 KEYR;     
    __IO_UINT32 OPTKEYR;  
    __IO_UINT32 SR;       
    __IO_UINT32 CR;       
    __IO_UINT32 OPTCR;    
} Flash_RegType;

#define FLASH_REG                 ((Flash_RegType*)FLASH_BASE_ADDR)

#define FLASH_CR_PG               (1U << 0)
#define FLASH_CR_SER              (1U << 1)
#define FLASH_CR_MER              (1U << 2)
#define FLASH_CR_SNB_POS          (3U)
#define FLASH_CR_SNB_MASK         (0xFU << FLASH_CR_SNB_POS)
#define FLASH_CR_PSIZE_POS        (8U)
#define FLASH_CR_PSIZE_MASK       (3U << FLASH_CR_PSIZE_POS)
#define FLASH_CR_PSIZE_X32        (2U << FLASH_CR_PSIZE_POS)
#define FLASH_CR_STRT             (1U << 16)
#define FLASH_CR_LOCK             (1U << 31)

#define FLASH_SR_EOP              (1U << 0)
#define FLASH_SR_OPERR            (1U << 1)
#define FLASH_SR_WRPERR           (1U << 4)  
#define FLASH_SR_PGAERR           (1U << 5)  
#define FLASH_SR_PGPERR           (1U << 6)  
#define FLASH_SR_PGSERR           (1U << 7)  
/* Khai báo cờ RDERR (bit 8 - Read Protection Error) để xử lý lỗi Unit Test */
#define FLASH_SR_RDERR            (1U << 8)   
#define FLASH_SR_BSY              (1U << 16) 

#define FLASH_KEY1                (0x45670123UL)
#define FLASH_KEY2                (0xCDEF89ABUL)

/* Bắt buộc đưa RDERR vào mask clear cờ lỗi để tránh treo cờ RDERR cũ */
#define FLASH_SR_ALL_ERRORS       (FLASH_SR_EOP | FLASH_SR_OPERR | FLASH_SR_WRPERR | \
                                   FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | \
                                   FLASH_SR_RDERR)

static Flash_IP_StatusType Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
/* Biến đếm phục vụ cơ chế Timeout */
static uint32 Flash_IP_TimeoutCounter = 0U; 

void Flash_IP_Init(void) 
{
    uint32 acrTemp = FLASH_REG->ACR;
    
    acrTemp &= ~0x07U;
    acrTemp |= ((uint32)FLASH_IP_LATENCY_SETTING & 0x07U);
    FLASH_REG->ACR = acrTemp;

    FLASH_REG->ACR |= (1U << 11) | (1U << 12); 
    FLASH_REG->ACR &= ~((1U << 11) | (1U << 12)); 

    FLASH_REG->ACR |= (1U << 8) | (1U << 9) | (1U << 10);
    
    Flash_IP_DriverState = FLASH_IP_INITIALIZED;
    Flash_IP_TimeoutCounter = 0U;
}

void Flash_IP_DeInit(void) 
{
    Flash_IP_Lock();
    Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
    Flash_IP_TimeoutCounter = 0U;
}

void Flash_IP_Unlock(void)
{
    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U) 
    {
        FLASH_REG->KEYR = FLASH_KEY1;
        FLASH_REG->KEYR = FLASH_KEY2;
    }
}

void Flash_IP_Lock(void) 
{
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

Flash_IP_JobResultType Flash_IP_GetStatus(void) 
{
    uint32 srReg = FLASH_REG->SR;

    /* Xử lý Hardware Timeout nếu BSY bị treo quá lâu */
    if ((srReg & FLASH_SR_BSY) != 0U) 
    {
        Flash_IP_TimeoutCounter++;
        if (Flash_IP_TimeoutCounter >= FLASH_IP_TIMEOUT_MAX_TICKS)
        {
            /* Hủy chế độ ghi/xóa ở CR để giải phóng phần cứng khi Timeout */
            FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_STRT);
            Flash_IP_TimeoutCounter = 0U;
            return FLASH_IP_JOB_FAILED;
        }
        return FLASH_IP_JOB_BUSY;
    }

    /* BSY = 0 (Hardware rảnh): Reset lại counter cho lượt chạy sau */
    Flash_IP_TimeoutCounter = 0U;

    /* Bắt và clear cờ RDERR nếu phát hiện lỗi bảo vệ đọc */
    if ((srReg & FLASH_SR_RDERR) != 0U) 
    {
        FLASH_REG->SR = FLASH_SR_RDERR; /* Clear bằng cách ghi 1 */
        FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER);
        return FLASH_IP_JOB_FAILED;
    }

    if ((srReg & FLASH_SR_WRPERR) != 0U) 
    {
        FLASH_REG->SR = FLASH_SR_WRPERR; 
        FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER);
        return FLASH_IP_WRITE_PROTECT_ERROR;
    }

    if ((srReg & (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR)) != 0U) 
    {
        FLASH_REG->SR = (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR); 
        FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER);
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    /* Chỉ giải phóng bit PG/SER khi có cờ EOP (Job xong hoàn tất).
       Tuyệt đối KHÔNG clear PG/SER ở đầu hàm vì sẽ làm ngắt dở chừng lệnh Erase/Write đang chạy! */
    if ((srReg & FLASH_SR_EOP) != 0U) 
    {
        FLASH_REG->SR = FLASH_SR_EOP;
        FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER);

        /* Flush Cache để CPU đọc dữ liệu mới nhất từ Flash */
        FLASH_REG->ACR |= (1U << 11) | (1U << 12); 
        FLASH_REG->ACR &= ~((1U << 11) | (1U << 12)); 
    }

    return FLASH_IP_JOB_OK;
}

Flash_IP_JobResultType Flash_IP_Erase(uint8 sectorNum) 
{
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sectorNum > 7U)) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    /* Clear sạch cờ lỗi cũ (bao gồm RDERR) và reset Timeout trước khi Start Erase */
    FLASH_REG->SR = FLASH_SR_ALL_ERRORS;
    Flash_IP_TimeoutCounter = 0U;

    uint32 crReg = FLASH_REG->CR;
    crReg &= ~(FLASH_CR_SNB_MASK | FLASH_CR_PG);
    crReg |= ((uint32)sectorNum << FLASH_CR_SNB_POS) | FLASH_CR_SER;
    FLASH_REG->CR = crReg;

    /* Kích hoạt Erase */
    FLASH_REG->CR |= FLASH_CR_STRT;

    return FLASH_IP_JOB_BUSY;
}

Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length) 
{
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sourcePtr == NULL_PTR)) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((address % 4U) != 0U || (length % 4U) != 0U || length == 0U) 
    {
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    /* Clear cờ lỗi cũ và reset Timeout trước khi Write */
    FLASH_REG->SR = FLASH_SR_ALL_ERRORS;
    Flash_IP_TimeoutCounter = 0U;

    uint32 crReg = FLASH_REG->CR;
    crReg &= ~(FLASH_CR_PSIZE_MASK | FLASH_CR_SER);
    crReg |= FLASH_CR_PSIZE_X32 | FLASH_CR_PG;
    FLASH_REG->CR = crReg;

    const uint32 *srcWordPtr = (const uint32 *)(const void *)sourcePtr;
    *(__IO_UINT32*)address = srcWordPtr[0];

    return FLASH_IP_JOB_BUSY;
}

Flash_IP_JobResultType Flash_IP_Read(uint32_t address, uint8_t *targetPtr, uint32_t length) 
{
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (targetPtr == NULL_PTR)) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    uint32_t wordsCount = length / 4U;
    uint32_t remainderBytes = length % 4U;

    uint32_t *targetPtr32 = (uint32_t *)(void *)targetPtr;
    uint32_t currentAddr = address;

    /* Đọc nhanh theo khối 32-bit (Word) bằng lệnh LDR để tối ưu tốc độ x4 */
    for (uint32_t i = 0U; i < wordsCount; i++) 
    {
        targetPtr32[i] = *(__IO_UINT32 *)currentAddr;
        currentAddr += 4U;
    }

    /* Đọc phần byte lẻ còn lại ở cuối (nếu có) */
    if (remainderBytes > 0U)
    {
        uint8_t *targetPtr8 = &targetPtr[wordsCount * 4U];

        for (uint32_t j = 0U; j < remainderBytes; j++) 
        {
            targetPtr8[j] = *(__IO_UINT8 *)(currentAddr + j);
        }
    }

    return FLASH_IP_JOB_OK;
}

Flash_IP_JobResultType Flash_IP_BlankCheck(uint32 address, uint32 length)
{
    if (Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    uint32 wordsCount = length / 4U;
    uint32 remainderBytes = length % 4U;
    uint32 currentAddr = address;

    /* Check 32-bit (0xFFFFFFFF) trước thay vì đếm từng byte để tránh block CPU lâu */
    for (uint32 i = 0U; i < wordsCount; i++) 
    {
        if (*(__IO_UINT32 *)currentAddr != 0xFFFFFFFFU) 
        {
            return FLASH_IP_INCONSISTENT; 
        }
        currentAddr += 4U;
    }

    /* Check các byte lẻ còn lại */
    for (uint32 j = 0U; j < remainderBytes; j++) 
    {
        if (*(__IO_UINT8 *)(currentAddr + j) != 0xFFU) 
        {
            return FLASH_IP_INCONSISTENT; 
        }
    }

    return FLASH_IP_JOB_OK;
}

uint8 Flash_IP_GetSectorFromAddress(uint32 address) 
{
    if ((address >= 0x08000000UL) && (address <= 0x08003FFFUL)) return 0U; 
    if ((address >= 0x08004000UL) && (address <= 0x08007FFFUL)) return 1U; 
    if ((address >= 0x08008000UL) && (address <= 0x0800BFFFUL)) return 2U; 
    if ((address >= 0x0800C000UL) && (address <= 0x0800FFFFUL)) return 3U; 
    if ((address >= 0x08010000UL) && (address <= 0x0801FFFFUL)) return 4U; 
    if ((address >= 0x08020000UL) && (address <= 0x0803FFFFUL)) return 5U; 
    if ((address >= 0x08040000UL) && (address <= 0x0805FFFFUL)) return 6U; 
    if ((address >= 0x08060000UL) && (address <= 0x0807FFFFUL)) return 7U; 

    return 0xFFU; 
}