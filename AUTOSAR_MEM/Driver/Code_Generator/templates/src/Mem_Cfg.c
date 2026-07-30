/**********************************************************************************************************************
 *  FILE:         Mem_Cfg.c
 *  MODULE:       Mem (Memory Driver) - Pre-Compile Configuration Data
 *  MÔ TẢ:        Bản sao có thể truy cập khi runtime của tham số Published Information của Mem driver
 *                [ECUC_Mem_00020], được khai báo extern const trong Mem_Cfg.h.
 *********************************************************************************************************************/

#include "Mem_Cfg.h"

/* MemErasedValue [ECUC_Mem_00021] */
const uint32 Mem_ErasedValue = MEM_ERASED_VALUE;
