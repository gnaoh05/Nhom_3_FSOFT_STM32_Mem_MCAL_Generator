#include "Flash_IP.h"

#define FLASH_BASE_ADDR           (0x40023C00UL)

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
#define FLASH_SR_BSY              (1U << 16) 

#define FLASH_KEY1                (0x45670123UL)
#define FLASH_KEY2                (0xCDEF89ABUL)

static Flash_IP_DriverStateType Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;

void Flash_IP_Init(void) 
{
    uint32 acrTemp = FLASH_REG->ACR;
    
    acrTemp &= ~7U; 
    acrTemp |= 2U;  /* 2 Wait States */

    /* Reset I-Cache & D-Cache tránh lỗi Corrupt */
    FLASH_REG->ACR |= (1U << 11) | (1U << 12); 
    FLASH_REG->ACR &= ~((1U << 11) | (1U << 12)); 

    acrTemp |= (1U << 8) | (1U << 9) | (1U << 10);
    FLASH_REG->ACR = acrTemp;
    
    Flash_IP_DriverState = FLASH_IP_INITIALIZED;
}

void Flash_IP_DeInit(void) 
{
    Flash_IP_Lock();
    Flash_IP_DriverState = FLASH_IP_UNINITIALIZED;
}

void Flash_IP_Unlock(void)
{
    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U) {
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

    if ((srReg & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    /* Giải phóng chế độ PG/SER để trả Flash về Mode Read cho CPU */
    FLASH_REG->CR &= ~(FLASH_CR_PG | FLASH_CR_SER);

    if ((srReg & FLASH_SR_WRPERR) != 0U) 
    {
        FLASH_REG->SR = FLASH_SR_WRPERR; 
        return FLASH_IP_WRITE_PROTECT_ERROR;
    }

    if ((srReg & (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR)) != 0U) 
    {
        FLASH_REG->SR = (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR); 
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    if ((srReg & FLASH_SR_EOP) != 0U) {
        FLASH_REG->SR = FLASH_SR_EOP;
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

    /* Clear cờ cũ và nạp Sector */
    uint32 crReg = FLASH_REG->CR;
    crReg &= ~(FLASH_CR_SNB_MASK | FLASH_CR_PG);
    crReg |= ((uint32)sectorNum << FLASH_CR_SNB_POS) | FLASH_CR_SER;
    FLASH_REG->CR = crReg;

    FLASH_REG->CR |= FLASH_CR_STRT;

    return FLASH_IP_JOB_BUSY;
}

Flash_IP_JobResultType Flash_IP_Write(uint32 address, const uint8 *sourcePtr, uint32 length) 
{
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (sourcePtr == NULL_PTR)) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((address % 4U) != 0U || (length % 4U) != 0U) 
    {
        return FLASH_IP_ALIGNMENT_ERROR;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    /* Bật PSIZE x32 và cờ PG */
    uint32 crReg = FLASH_REG->CR;
    crReg &= ~(FLASH_CR_PSIZE_MASK | FLASH_CR_SER);
    crReg |= FLASH_CR_PSIZE_X32 | FLASH_CR_PG;
    FLASH_REG->CR = crReg;

    /* Ghi dữ liệu 32-bit Word đầu tiên trong chunk */
    uint32 dataWord = *((const uint32*)(const void*)sourcePtr);
    *(__IO_UINT32*)address = dataWord;

    return FLASH_IP_JOB_BUSY;
}

Flash_IP_JobResultType Flash_IP_Read(uint32 address, uint8 *targetPtr, uint32 length) 
{
    if ((Flash_IP_DriverState == FLASH_IP_UNINITIALIZED) || (targetPtr == NULL_PTR)) 
    {
        return FLASH_IP_JOB_FAILED;
    }

    if ((FLASH_REG->SR & FLASH_SR_BSY) != 0U) 
    {
        return FLASH_IP_JOB_BUSY;
    }

    for (uint32 i = 0U; i < length; i++) 
    {
        targetPtr[i] = *(__IO_UINT8*)(address + i);
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

    for (uint32 i = 0U; i < length; i++) 
    {
        if (*(__IO_UINT8*)(address + i) != 0xFFU) 
        {
            return FLASH_IP_JOB_FAILED; /* Trả về FAILED nếu ô nhớ không rỗng */
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