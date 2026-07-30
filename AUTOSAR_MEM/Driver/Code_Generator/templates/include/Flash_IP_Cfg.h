/**********************************************************************************************************************
 *  FILE:         Flash_IP_Cfg.h
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver) - Configuration
 *  MÔ TẢ:        Dữ liệu cấu hình cho Flash_IP: macro hình học Flash và bảng sector của Flash nội
 *                STM32F401RE (single Flash bank, 512 KB, 8 sector, base 0x08000000), dựa trên RM0368 / PM0059:
 *                  - Sector 0..3 : 16 KB  each  (0x08000000 .. 0x0800FFFF)
 *                  - Sector 4    : 64 KB        (0x08010000 .. 0x0801FFFF)
 *                  - Sector 5..7 : 128 KB each  (0x08020000 .. 0x0807FFFF)
 *
 *                Chỉ khai báo; dữ liệu bảng sector thực tế nằm trong Flash_IP_Cfg.c.
 *                Hãy sinh lại/chỉnh cặp tệp này nếu port sang STM32F4xx khác density/variant.
 *********************************************************************************************************************/

#ifndef FLASH_IP_CFG_H
#define FLASH_IP_CFG_H

#include "Std_Types.h"

/*======================================================================================================================
 *  Hình học Flash (STM32F401RE - 512 KB, 8 sector)
 *====================================================================================================================*/
#define FLASH_IP_BASE_ADDRESS       0x08000000UL
#define FLASH_IP_TOTAL_SIZE         0x00080000UL   /* 512 KB */
#define FLASH_IP_SECTOR_COUNT       8u
#define FLASH_IP_ERASED_VALUE       0xFFu

/*======================================================================================================================
 *  Type và dữ liệu bảng sector, định nghĩa tại Flash_IP_Cfg.c.
 *  Chủ ý khai báo là mảng INCOMPLETE ("[]"): số phần tử thực tế do initializer list trong Flash_IP_Cfg.c
 *  quyết định và không lặp lại tại đây. Vì vậy kiểm tra compile-time của Flash_IP_Cfg.c giữa số initializer
 *  thực tế và FLASH_IP_SECTOR_COUNT có ý nghĩa, xem ghi chú [SWS_Mem_00033] ở đó.
 *====================================================================================================================*/
typedef struct
{
    uint32 StartAddress;
    uint32 Size;
} Flash_IP_SectorType;

extern const Flash_IP_SectorType Flash_IP_SectorTable[];

#endif /* FLASH_IP_CFG_H */
