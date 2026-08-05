
#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"

/* --- Các Macro cấu hình sinh ra từ GUI --- */
#define MEM_DEV_ERROR_DETECT  (STD_ON)
#define MEM_MAX_INSTANCES     (1u)
#define MEM_INDEX             (0u)
#define MEM_ERASED_VALUE      (0xFFu)

/* --- Khai báo biến toàn cục chứa cấu trúc vật lý Flash --- */
/* Biến này sẽ được định nghĩa ở Mem_Cfg.c, tầng Core sẽ dùng biến này */
extern const Mem_ConfigType Mem_ConfigData;

#endif /* MEM_CFG_H */