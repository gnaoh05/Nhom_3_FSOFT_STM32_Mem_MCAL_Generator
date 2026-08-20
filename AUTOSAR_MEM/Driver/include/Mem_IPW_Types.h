#ifndef MEM_IPW_TYPES_H
#define MEM_IPW_TYPES_H

#include "Std_Types.h"
#include "Mem_Types.h"
#include "Flash_IP_Types.h"

/* Execution status of the IP wrapper layer */
typedef enum 
{
    MEM_IPW_IDLE  = 0U,
    MEM_IPW_BUSY  = 1U,
    MEM_IPW_ERROR = 2U
} Mem_Ipw_StatusType;

#endif /* MEM_IPW_TYPES_H */
