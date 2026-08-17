/**********************************************************************************************************************
 *  FILE:         Flash_IP.h
 *  MODULE:       Flash_IP (STM32F401RE internal Flash - bare-metal IP driver)
 *  MÔ TẢ:        Driver mức thanh ghi cho Flash nội STM32F401RE (single Flash bank, 512 KB, 8 sector,
 *                địa chỉ gốc 0x08000000), dựa trên RM0368 / PM0059:
 *                  - Sector 0..3 : 16 KB  each  (0x08000000 .. 0x0800FFFF)
 *                  - Sector 4    : 64 KB        (0x08010000 .. 0x0801FFFF)
 *                  - Sector 5..7 : 128 KB each  (0x08020000 .. 0x0807FFFF)
 *
 *                KHÔNG dùng CMSIS, ST HAL hoặc LL; địa chỉ thanh ghi và bit-mask được định nghĩa trực tiếp
 *                từ RM0368 trong Stm32F401_BareMetal.h.
 *
 *                Program/Erase được kích hoạt rồi hoàn tất bằng polling trong Flash_IP_MainFunction().
 *                Flash_IP_GetStatus() là truy vấn thuần và không xóa cờ phần cứng.
 *                ở lớp trên tệp này.
 *
 *  QUAN TRỌNG (giới hạn single Flash bank, RM0368 chapter 3.4):
 *                STM32F401 có một Flash bank và KHÔNG hỗ trợ Read-While-Write. Khi program/erase đang chạy
 *                (BSY=1), mọi truy cập CPU bus vào vùng địa chỉ Flash, kể cả instruction fetch, sẽ bị AHB
 *                bus matrix stall tự động cho đến khi thao tác hoàn tất. Vì vậy: mã chạy từ Flash sẽ có vẻ
 *                "đứng" trong thời gian thao tác. Đây là hành vi mong đợi, không phải lỗi. Nếu ứng dụng không chấp nhận
 *                stall này, ví dụ hard real-time task, hãy chạy mã gọi và vector table từ RAM.
 *********************************************************************************************************************/

#ifndef FLASH_IP_H
#define FLASH_IP_H

#include "Std_Types.h"
#include "Flash_IP_Cfg.h"   /* Flash_IP_SectorType, Flash_IP_SectorTable[] - sector geometry (config data) */

/*======================================================================================================================
 *  Trạng thái thao tác phần cứng đang chạy / được kích hoạt gần nhất
 *====================================================================================================================*/
typedef enum
{
    FLASH_IP_IDLE = 0,   /* chưa có thao tác kể từ Init/lần đọc kết quả thành công gần nhất */
    FLASH_IP_BUSY,       /* thao tác đang chạy (BSY=1 hoặc chờ FLASH_IRQHandler)            */
    FLASH_IP_OK,         /* thao tác gần nhất hoàn tất thành công                            */
    FLASH_IP_ERROR       /* thao tác gần nhất hoàn tất với lỗi phần cứng (WRPERR/PGAERR/...) */
} Flash_IP_StatusType;

/*======================================================================================================================
 *  Vòng đời
 *====================================================================================================================*/
extern void Flash_IP_Init(void);
extern void Flash_IP_DeInit(void);
extern void Flash_IP_Cancel(void);

/*======================================================================================================================
 *  Trạng thái
 *====================================================================================================================*/
extern Flash_IP_StatusType Flash_IP_GetStatus(void);

/*======================================================================================================================
 *  Read: Flash là memory-mapped, đây là phép sao chép đồng bộ thông thường.
 *====================================================================================================================*/
extern Std_ReturnType Flash_IP_Read(uint32 address, uint8* data, uint32 length);

/*======================================================================================================================
 *  Program / Erase: chỉ kích hoạt, hoàn tất bất đồng bộ (xem Flash_IP_GetStatus()).
 *  Trả về E_NOT_OK ngay nếu đã có job đang chạy hoặc tham số không hợp lệ.
 *====================================================================================================================*/
extern Std_ReturnType Flash_IP_ProgramStart(uint32 address, const uint8* data, uint32 length);
extern Std_ReturnType Flash_IP_EraseSectorStart(uint8 sectorNumber);

/*======================================================================================================================
 *  Gọi tuần hoàn từ Mem_Ipw_MainFunction(). Hàm này là nơi duy nhất đọc/xóa cờ hoàn tất hoặc lỗi.
 *====================================================================================================================*/
extern void Flash_IP_MainFunction(void);

/*======================================================================================================================
 *  Hàm hỗ trợ hình học sector
 *====================================================================================================================*/
extern uint8  Flash_IP_GetSectorFromAddress(uint32 address); /* trả về 0xFFu nếu địa chỉ ngoài phạm vi */
extern uint32 Flash_IP_GetSectorStartAddress(uint8 sectorNumber);
extern uint32 Flash_IP_GetSectorSize(uint8 sectorNumber);

#endif /* FLASH_IP_H */
