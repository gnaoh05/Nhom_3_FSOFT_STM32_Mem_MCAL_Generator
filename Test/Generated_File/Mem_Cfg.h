#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"
#include "Mem_Types.h" 

/* Các cấu hình macro cơ bản */
#define MEM_DEV_ERROR_DETECT (STD_ON)
#define MEM_MAX_INSTANCES    (1u)
#define MEM_INDEX            (0u)
#define MEM_ERASED_VALUE     (0xFFu)

/* Khai báo biến cấu hình toàn cục */
extern const Mem_ConfigType Mem_ConfigData;

#endif /* MEM_CFG_H */