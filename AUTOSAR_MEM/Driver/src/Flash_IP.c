/**********************************************************************************************************************
 *  FILE:         Flash_IP.c
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver)
 *  MÔ TẢ:        Xem Flash_IP.h để biết mô tả module và giới hạn quan trọng của single Flash bank.
 *
 *                Register reference (RM0368 / PM0059):
 *                  FLASH_CR  : PG(0) SER(1) MER(2) SNB(6:3) PSIZE(9:8) STRT(16) EOPIE(24) ERRIE(25) LOCK(31)
 *                  FLASH_SR  : EOP(0) OPERR(1) WRPERR(4) PGAERR(5) PGPERR(6) PGSERR(7) BSY(16)
 *                  FLASH_KEYR: unlock sequence KEY1=0x45670123, KEY2=0xCDEF89AB
 *
 *  PHỤ THUỘC:    Stm32F401_BareMetal.h cung cấp địa chỉ thanh ghi và bit-mask được xây dựng trực tiếp
 *                từ RM0368. Module không phụ thuộc CMSIS, HAL hoặc LL.
 *********************************************************************************************************************/

#include "Flash_IP.h"
#include "Stm32F401_BareMetal.h"

/* GHI CHÚ: Trên Cortex-M4 32-bit thực tế, sizeof(void*) == sizeof(uint32), nên ép kiểu địa chỉ uint32 sang
 * (volatile uint8*) bên dưới có đúng độ rộng và không sinh cảnh báo. Cảnh báo -Wint-to-pointer-cast chỉ có
 * khi biên dịch tệp này trên máy chủ 64-bit để kiểm tra cú pháp; đó không phải target triển khai. */

/*======================================================================================================================
 *  Định nghĩa bit-mask / vị trí cục bộ theo RM0368.
 *====================================================================================================================*/
#ifndef FLASH_CR_PG
#define FLASH_CR_PG            (1UL << 0)
#endif
#ifndef FLASH_CR_SER
#define FLASH_CR_SER           (1UL << 1)
#endif
#ifndef FLASH_CR_MER
#define FLASH_CR_MER           (1UL << 2)
#endif
#ifndef FLASH_CR_SNB_Pos
#define FLASH_CR_SNB_Pos       3U
#endif
#ifndef FLASH_CR_SNB
#define FLASH_CR_SNB           (0xFUL << FLASH_CR_SNB_Pos)
#endif
#ifndef FLASH_CR_PSIZE_Pos
#define FLASH_CR_PSIZE_Pos     8U
#endif
#ifndef FLASH_CR_PSIZE
#define FLASH_CR_PSIZE         (0x3UL << FLASH_CR_PSIZE_Pos)
#endif
#ifndef FLASH_CR_STRT
#define FLASH_CR_STRT          (1UL << 16)
#endif
#ifndef FLASH_CR_EOPIE
#define FLASH_CR_EOPIE         (1UL << 24)
#endif
#ifndef FLASH_CR_ERRIE
#define FLASH_CR_ERRIE         (1UL << 25)
#endif
#ifndef FLASH_CR_LOCK
#define FLASH_CR_LOCK          (1UL << 31)
#endif

#ifndef FLASH_ACR_ICEN
#define FLASH_ACR_ICEN         (1UL << 9)
#endif
#ifndef FLASH_ACR_DCEN
#define FLASH_ACR_DCEN         (1UL << 10)
#endif
#ifndef FLASH_ACR_ICRST
#define FLASH_ACR_ICRST        (1UL << 11)
#endif
#ifndef FLASH_ACR_DCRST
#define FLASH_ACR_DCRST        (1UL << 12)
#endif

#ifndef FLASH_SR_EOP
#define FLASH_SR_EOP           (1UL << 0)
#endif
#ifndef FLASH_SR_OPERR
#define FLASH_SR_OPERR         (1UL << 1)
#endif
#ifndef FLASH_SR_WRPERR
#define FLASH_SR_WRPERR        (1UL << 4)
#endif
#ifndef FLASH_SR_PGAERR
#define FLASH_SR_PGAERR        (1UL << 5)
#endif
#ifndef FLASH_SR_PGPERR
#define FLASH_SR_PGPERR        (1UL << 6)
#endif
#ifndef FLASH_SR_PGSERR
#define FLASH_SR_PGSERR        (1UL << 7)
#endif
#ifndef FLASH_SR_BSY
#define FLASH_SR_BSY           (1UL << 16)
#endif

#define FLASH_IP_SR_ALL_ERROR_FLAGS  (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)
#define FLASH_IP_SR_ALL_CLEAR_FLAGS  (FLASH_SR_EOP | FLASH_IP_SR_ALL_ERROR_FLAGS)

#define FLASH_IP_KEY1                0x45670123UL
#define FLASH_IP_KEY2                0xCDEF89ABUL

#define FLASH_IP_PSIZE_BYTE          0x0UL   /* PSIZE = 00: song song x8 (lập trình theo byte) */

/*======================================================================================================================
 *  Trạng thái module
 *  Hình học sector (Flash_IP_SectorType / Flash_IP_SectorTable[]) được định nghĩa tại Flash_IP_Cfg.c và
 *  khai báo tại Flash_IP_Cfg.h (được include gián tiếp qua Flash_IP.h).
 *====================================================================================================================*/
typedef enum
{
    FLASH_IP_OP_NONE = 0,
    FLASH_IP_OP_PROGRAM,
    FLASH_IP_OP_ERASE
} Flash_IP_OperationType;

static volatile Flash_IP_StatusType    Flash_IP_Status    = FLASH_IP_IDLE;
static volatile Flash_IP_OperationType Flash_IP_CurrentOp = FLASH_IP_OP_NONE;

/* Tiến trình job program: một byte được lập trình trong mỗi chu kỳ thao tác phần cứng / interrupt.
 * Lập trình theo byte (PSIZE = x8) hỗ trợ địa chỉ và độ dài bất kỳ, kể cả không căn word như Mem_Write()
 * có thể yêu cầu, mà không cần logic căn chỉnh hoặc đệm bổ sung. */
static const uint8* Flash_IP_ProgSrcPtr;
static uint32        Flash_IP_ProgAddress;
static uint32        Flash_IP_ProgRemaining;

/*======================================================================================================================
 *  Hàm hỗ trợ cục bộ
 *====================================================================================================================*/
static void Flash_IP_Unlock(void)
{
    if ((STM32_FLASH->CR & FLASH_CR_LOCK) != 0u)
    {
        STM32_FLASH->KEYR = FLASH_IP_KEY1;
        STM32_FLASH->KEYR = FLASH_IP_KEY2;
    }
}

static void Flash_IP_Lock(void)
{
    STM32_FLASH->CR |= FLASH_CR_LOCK;
}

static void Flash_IP_ClearAllFlags(void)
{
    /* Bit lỗi/EOP của FLASH_SR là rc_w1, được xóa bằng cách ghi 1 */
    STM32_FLASH->SR = FLASH_IP_SR_ALL_CLEAR_FLAGS;
}

/* RM0368 3.5.5 nêu rằng erase có thể để lại dữ liệu cũ trong I/D cache của Flash. Chỉ reset cache sau khi
 * tạm thời tắt chúng, rồi khôi phục trạng thái enable trước đó. */
static void Flash_IP_RefreshCachesAfterErase(void)
{
    uint32 cacheEnableMask = STM32_FLASH->ACR & (FLASH_ACR_ICEN | FLASH_ACR_DCEN);

    STM32_FLASH->ACR &= ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    STM32_FLASH->ACR |= FLASH_ACR_ICRST | FLASH_ACR_DCRST;
    STM32_FLASH->ACR &= ~(FLASH_ACR_ICRST | FLASH_ACR_DCRST);
    STM32_FLASH->ACR |= cacheEnableMask;
}

/* Kích hoạt lập trình đúng một byte tại Flash_IP_ProgAddress từ *Flash_IP_ProgSrcPtr.
 * Kết quả thành công hoặc lỗi sẽ được FLASH_IRQHandler() báo sau đó. */
static void Flash_IP_TriggerNextByte(void)
{
    STM32_FLASH->CR &= ~FLASH_CR_PSIZE;
    STM32_FLASH->CR |= (FLASH_IP_PSIZE_BYTE << FLASH_CR_PSIZE_Pos);
    STM32_FLASH->CR |= FLASH_CR_PG | FLASH_CR_EOPIE | FLASH_CR_ERRIE;

    *(volatile uint8*)(Flash_IP_ProgAddress) = *Flash_IP_ProgSrcPtr;
}

/*======================================================================================================================
 *  Vòng đời
 *====================================================================================================================*/
void Flash_IP_Init(void)
{
    Flash_IP_Status    = FLASH_IP_IDLE;
    Flash_IP_CurrentOp = FLASH_IP_OP_NONE;

    Flash_IP_Lock(); /* bảo đảm Flash khởi động ở trạng thái khóa */
    Flash_IP_ClearAllFlags();

    Stm32_BareMetalEnableIrq(STM32_FLASH_IRQ_NUMBER);
}

void Flash_IP_DeInit(void)
{
    /* Hủy theo khả năng tốt nhất: tắt nguồn interrupt và khóa Flash. Thao tác phần cứng đã chạy (BSY=1)
     * không thể bị hủy; bus sẽ stall như mô tả trong Flash_IP.h cho đến khi thao tác tự hoàn tất. */
    STM32_FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
    Flash_IP_Lock();

    Flash_IP_Status    = FLASH_IP_IDLE;
    Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
}

/*======================================================================================================================
 *  Trạng thái
 *====================================================================================================================*/
Flash_IP_StatusType Flash_IP_GetStatus(void)
{
    return Flash_IP_Status;
}

/*======================================================================================================================
 *  Read (đồng bộ - Flash là memory-mapped)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_Read(uint32 address, uint8* data, uint32 length)
{
    uint32 i;

    for (i = 0u; i < length; i++)
    {
        data[i] = *(volatile uint8*)(address + i);
    }

    return E_OK;
}

/*======================================================================================================================
 *  Program (bất đồng bộ, theo byte, điều khiển bởi interrupt)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_ProgramStart(uint32 address, const uint8* data, uint32 length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Flash_IP_Status != FLASH_IP_BUSY) && (length > 0u))
    {
        Flash_IP_ProgSrcPtr    = data;
        Flash_IP_ProgAddress   = address;
        Flash_IP_ProgRemaining = length;
        Flash_IP_CurrentOp     = FLASH_IP_OP_PROGRAM;
        Flash_IP_Status        = FLASH_IP_BUSY;

        Flash_IP_Unlock();
        Flash_IP_ClearAllFlags();
        Flash_IP_TriggerNextByte();

        retVal = E_OK;
    }

    return retVal;
}

/*======================================================================================================================
 *  Erase một sector (bất đồng bộ, điều khiển bởi interrupt)
 *====================================================================================================================*/
Std_ReturnType Flash_IP_EraseSectorStart(uint8 sectorNumber)
{
    Std_ReturnType retVal = E_NOT_OK;

    if ((Flash_IP_Status != FLASH_IP_BUSY) && (sectorNumber < FLASH_IP_SECTOR_COUNT))
    {
        Flash_IP_CurrentOp = FLASH_IP_OP_ERASE;
        Flash_IP_Status    = FLASH_IP_BUSY;

        Flash_IP_Unlock();
        Flash_IP_ClearAllFlags();

        STM32_FLASH->CR &= ~FLASH_CR_SNB;
        STM32_FLASH->CR |= ((uint32)sectorNumber << FLASH_CR_SNB_Pos) |
                           FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE;
        STM32_FLASH->CR |= FLASH_CR_STRT;

        retVal = E_OK;
    }

    return retVal;
}

/*======================================================================================================================
 *  Lập lịch: hoàn tất do interrupt điều khiển, hiện chưa cần polling. Dành cho giám sát timeout/watchdog
 *  trong tương lai, ví dụ phát hiện bit BSY bị kẹt nếu interrupt bị mất hoặc bị mask.
 *====================================================================================================================*/
void Flash_IP_MainFunction(void)
{
    /* Chủ ý để trống, xem chú thích trong header */
}

/*======================================================================================================================
 *  Hàm hỗ trợ hình học sector
 *====================================================================================================================*/
uint8 Flash_IP_GetSectorFromAddress(uint32 address)
{
    uint8 sector;
    uint8 found = FLASH_IP_SECTOR_COUNT; /* giá trị canh gác "không tìm thấy", ép thành 0xFF bên dưới */

    for (sector = 0u; sector < FLASH_IP_SECTOR_COUNT; sector++)
    {
        uint32 start = Flash_IP_SectorTable[sector].StartAddress;
        uint32 end   = start + Flash_IP_SectorTable[sector].Size; /* không bao gồm địa chỉ kết thúc */

        if ((address >= start) && (address < end))
        {
            found = sector;
            break;
        }
    }

    return (found == FLASH_IP_SECTOR_COUNT) ? 0xFFu : found;
}

uint32 Flash_IP_GetSectorStartAddress(uint8 sectorNumber)
{
    uint32 result = 0u;

    if (sectorNumber < FLASH_IP_SECTOR_COUNT)
    {
        result = Flash_IP_SectorTable[sectorNumber].StartAddress;
    }

    return result;
}

uint32 Flash_IP_GetSectorSize(uint8 sectorNumber)
{
    uint32 result = 0u;

    if (sectorNumber < FLASH_IP_SECTOR_COUNT)
    {
        result = Flash_IP_SectorTable[sectorNumber].Size;
    }

    return result;
}

/*======================================================================================================================
 *  Trình xử lý FLASH global interrupt
 *  Xảy ra khi EOP (hoàn tất thành công) hoặc bất kỳ bit lỗi nào được bật thông qua EOPIE/ERRIE trong
 *  Flash_IP_TriggerNextByte()/Flash_IP_EraseSectorStart(). Xem Flash_IP.h về hành vi stall của single bank,
 *  yếu tố quyết định thời điểm handler này thực sự được thực thi.
 *====================================================================================================================*/
void FLASH_IRQHandler(void)
{
    uint32 sr = STM32_FLASH->SR;

    if ((sr & FLASH_IP_SR_ALL_ERROR_FLAGS) != 0u)
    {
        Flash_IP_ClearAllFlags();
        STM32_FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
        Flash_IP_Lock();

        Flash_IP_Status    = FLASH_IP_ERROR;
        Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
    }
    else if ((sr & FLASH_SR_EOP) != 0u)
    {
        STM32_FLASH->SR = FLASH_SR_EOP; /* xóa EOP (rc_w1) */

        if (Flash_IP_CurrentOp == FLASH_IP_OP_PROGRAM)
        {
            STM32_FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_EOPIE | FLASH_CR_ERRIE);

            Flash_IP_ProgSrcPtr++;
            Flash_IP_ProgAddress++;
            Flash_IP_ProgRemaining--;

            if (Flash_IP_ProgRemaining == 0u)
            {
                Flash_IP_Lock();
                Flash_IP_Status    = FLASH_IP_OK;
                Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
            }
            else
            {
                /* vẫn BUSY: lập trình byte tiếp theo, không cần mở khóa lại vì LOCK chưa được đặt */
                Flash_IP_TriggerNextByte();
            }
        }
        else if (Flash_IP_CurrentOp == FLASH_IP_OP_ERASE)
        {
            STM32_FLASH->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB | FLASH_CR_EOPIE | FLASH_CR_ERRIE);
            Flash_IP_Lock();
            Flash_IP_RefreshCachesAfterErase();

            Flash_IP_Status    = FLASH_IP_OK;
            Flash_IP_CurrentOp = FLASH_IP_OP_NONE;
        }
        else
        {
            /* EOP giả khi không có thao tác được theo dõi: xóa và bỏ qua. */
        }
    }
    else
    {
        /* Interrupt giả, không có EOP hoặc bit lỗi: không cần xử lý. */
    }
}
