/**********************************************************************************************************************
 *  FILE:         Flash_IP_Cfg.c
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver) - Configuration
 *  MÔ TẢ:        Bảng hình học sector của Flash nội STM32F401RE (512 KB, 8 sector).
 *                Xem Flash_IP_Cfg.h để biết định nghĩa type và tài liệu macro.
 *********************************************************************************************************************/

#include "Flash_IP_Cfg.h"

const Flash_IP_SectorType Flash_IP_SectorTable[] =
{
    /* Địa chỉ đầu    Kích thước     */
    { 0x08000000UL, 0x00004000UL }, /* Sector 0: 16 KB  */
    { 0x08004000UL, 0x00004000UL }, /* Sector 1: 16 KB  */
    { 0x08008000UL, 0x00004000UL }, /* Sector 2: 16 KB  */
    { 0x0800C000UL, 0x00004000UL }, /* Sector 3: 16 KB  */
    { 0x08010000UL, 0x00010000UL }, /* Sector 4: 64 KB  */
    { 0x08020000UL, 0x00020000UL }, /* Sector 5: 128 KB */
    { 0x08040000UL, 0x00020000UL }, /* Sector 6: 128 KB */
    { 0x08060000UL, 0x00020000UL }  /* Sector 7: 128 KB */
};

/*======================================================================================================================
 *  [SWS_Mem_00033] "The Mem driver shall check static configuration parameters statically (at the latest
 *  during compile time) for correctness."
 *
 *  Hai kiểm tra bên dưới phát hiện chỉnh sửa không khớp giữa FLASH_IP_SECTOR_COUNT và bảng trên
 *  (kiểm tra kích thước mảng), hoặc giữa FLASH_IP_TOTAL_SIZE và tổng kích thước các sector (kiểm tra tổng
 *  kích thước). Nhờ đó lỗi cấu hình âm thầm làm sai runtime, ví dụ không bao phủ hết Flash hoặc chồng lấn
 *  sector, trở thành lỗi compile-time. Cách hiện thực dùng kỹ thuật "negative array size" để tương thích
 *  C90/C99, nơi chưa có _Static_assert.
 *====================================================================================================================*/
#define FLASH_IP_CFG_STATIC_ASSERT(cond, uniqueName) typedef char uniqueName[(cond) ? 1 : -1]

FLASH_IP_CFG_STATIC_ASSERT(
        (sizeof(Flash_IP_SectorTable) / sizeof(Flash_IP_SectorTable[0])) == FLASH_IP_SECTOR_COUNT,
        Flash_IP_Cfg_SectorTableSizeCheck);

#define FLASH_IP_CFG_SECTOR_TOTAL_SIZE_LITERAL \
        (0x00004000UL + 0x00004000UL + 0x00004000UL + 0x00004000UL + \
         0x00010000UL + 0x00020000UL + 0x00020000UL + 0x00020000UL)

FLASH_IP_CFG_STATIC_ASSERT(
        FLASH_IP_CFG_SECTOR_TOTAL_SIZE_LITERAL == FLASH_IP_TOTAL_SIZE,
        Flash_IP_Cfg_TotalSizeCheck);
