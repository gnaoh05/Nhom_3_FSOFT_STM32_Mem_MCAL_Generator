#ifndef FEE_CFG_H
#define FEE_CFG_H

#include "Std_Types.h"
#include "MemIf_Types.h"

#define FEE_MODULE_ID                     (40U)
#define FEE_INSTANCE_ID                   (0U)

#define FEE_DEV_ERROR_DETECT              (STD_ON)

#define FEE_E_UNINIT                      (0x01U)
#define FEE_E_INVALID_BLOCK_NO            (0x02U)
#define FEE_E_INVALID_BLOCK_OFS           (0x03U)
#define FEE_E_PARAM_POINTER               (0x04U)
#define FEE_E_INVALID_BLOCK_LEN           (0x05U)
#define FEE_E_BUSY                        (0x06U)
#define FEE_E_BUSY_INTERNAL               (0x07U)

/* Gán Address Area ID tương ứng trong MemAcc */
#define FEE_MEMACC_ADDRESS_AREA_ID        (0U)

typedef struct
{
    uint16 blockNumber;          /* Logical Block Number */
    uint16 blockSize;            /* Length of block payload in bytes */
    uint32 logicalAddress;       /* Start logical address mapped into MemAcc */
    boolean immediateData;       /* Immediate data flag */
} Fee_BlockConfigType;

typedef struct
{
    const Fee_BlockConfigType *blockConfigTable;
    uint16 numberOfBlocks;
} Fee_ConfigType;

#define FEE_NUM_BLOCKS                    (2U)

extern const Fee_BlockConfigType Fee_BlockConfigTable[FEE_NUM_BLOCKS];
extern const Fee_ConfigType Fee_Config;

#endif /* FEE_CFG_H */