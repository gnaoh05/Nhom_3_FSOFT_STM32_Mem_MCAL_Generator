#include "Fee_Cfg.h"

/* Bảng định tuyến Block ID logic sang địa chỉ logic trong MemAcc */
const Fee_BlockConfigType Fee_BlockConfigTable[FEE_NUM_BLOCKS] = 
{
    {
        .blockNumber     = 1U,
        .blockSize       = 64U,
        .logicalAddress  = 0x00000000UL, /* Khớp với LogicalStartAddress của MemAcc */
        .immediateData   = FALSE
    },
    {
        .blockNumber     = 2U,
        .blockSize       = 128U,
        .logicalAddress  = 0x00000040UL,
        .immediateData   = FALSE
    }
};

const Fee_ConfigType Fee_Config = 
{
    .blockConfigTable = Fee_BlockConfigTable,
    .numberOfBlocks   = FEE_NUM_BLOCKS
};