#ifndef FEE_H
#define FEE_H

#include "Std_Types.h"
#include "MemIf_Types.h"
#include "Fee_Cfg.h"

#define FEE_SID_INIT                      (0x00U)
#define FEE_SID_SET_MODE                  (0x01U)
#define FEE_SID_READ                      (0x02U)
#define FEE_SID_WRITE                     (0x03U)
#define FEE_SID_CANCEL                    (0x04U)
#define FEE_SID_GET_STATUS                (0x05U)
#define FEE_SID_GET_JOB_RESULT            (0x06U)
#define FEE_SID_INVALIDATE_BLOCK          (0x07U)
#define FEE_SID_GET_VERSION_INFO          (0x08U)
#define FEE_SID_ERASE_IMMEDIATE_BLOCK     (0x09U)
#define FEE_SID_JOB_END_NOTIFICATION      (0x10U)
#define FEE_SID_JOB_ERROR_NOTIFICATION    (0x11U)
#define FEE_SID_MAIN_FUNCTION             (0x12U)

/* ============================================================================
 * API FUNCTION PROTOTYPES
 * ============================================================================ */

/**
 * @brief Service to initialize the FEE module.
 * @details [SWS_Fee_00085], [SWS_Fee_00189], [SWS_Fee_00120], [SWS_Fee_00168]
 * 
 * @param[in] ConfigPtr Pointer to configuration structure (shall always be NULL_PTR).
 */
void Fee_Init(const Fee_ConfigType *ConfigPtr);

/**
 * @brief Service to switch operational mode (Fast/Slow) of underlying driver.
 * @details [SWS_Fee_00086], [SWS_Fee_00020], [SWS_Fee_00121], [SWS_Fee_00170]
 * 
 * @param[in] Mode Desired mode for the underlying driver.
 */
void Fee_SetMode(MemIf_ModeType Mode);

/**
 * @brief Service to initiate an asynchronous read job.
 * @details [SWS_Fee_00087], [SWS_Fee_00021], [SWS_Fee_00022], [SWS_Fee_00172],
 *          [SWS_Fee_00122], [SWS_Fee_00133], [SWS_Fee_00134], [SWS_Fee_00135],
 *          [SWS_Fee_00136], [SWS_Fee_00137], [SWS_Fee_00162]
 * 
 * @param[in]  BlockNumber   Logical block identifier.
 * @param[in]  BlockOffset   Read address offset inside the block.
 * @param[out] DataBufferPtr Pointer to destination data buffer.
 * @param[in]  Length        Number of bytes to read.
 * 
 * @return Std_ReturnType E_OK if accepted, E_NOT_OK otherwise.
 */
Std_ReturnType Fee_Read(uint16 BlockNumber, 
                        uint16 BlockOffset, 
                        uint8 *DataBufferPtr, 
                        uint16 Length);

/**
 * @brief Service to initiate an asynchronous write job.
 * @details [SWS_Fee_00088], [SWS_Fee_00024], [SWS_Fee_00025], [SWS_Fee_00174],
 *          [SWS_Fee_00026], [SWS_Fee_00123], [SWS_Fee_00144], [SWS_Fee_00138],
 *          [SWS_Fee_00139], [SWS_Fee_00163]
 * 
 * @param[in] BlockNumber   Logical block identifier.
 * @param[in] DataBufferPtr Pointer to source data buffer.
 * 
 * @return Std_ReturnType E_OK if accepted, E_NOT_OK otherwise.
 */
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8 *DataBufferPtr);

/**
 * @brief Service to return the current status of the FEE module.
 * @details [SWS_Fee_00090], [SWS_Fee_00034], [SWS_Fee_00128], [SWS_Fee_00129], [SWS_Fee_00074]
 * 
 * @return MemIf_StatusType
 */
MemIf_StatusType Fee_GetStatus(void);

/**
 * @brief Service to query the result of the last accepted job issued by upper layer.
 * @details [SWS_Fee_00091], [SWS_Fee_00035], [SWS_Fee_00156], [SWS_Fee_00157],
 *          [SWS_Fee_00158], [SWS_Fee_00159], [SWS_Fee_00160], [SWS_Fee_00155], [SWS_Fee_00125]
 * 
 * @return MemIf_JobResultType
 */
MemIf_JobResultType Fee_GetJobResult(void);

/**
 * @brief Cyclic main function handling the Fee asynchronous state machine.
 * @details [SWS_Fee_00073], [SWS_Fee_00026]
 */
void Fee_MainFunction(void);

#endif /* FEE_H */