/**********************************************************************************************************************
 *  FILE:         Mem_IPW.h
 *  MODULE:       Mem_IPW (Memory Driver - IP Wrapper Layer)
 *  DESCRIPTION:  Hardware wrapper layer used internally by Mem.c.
 *
 *                AUTOSAR_CP_SWS_MemoryDriver does not itself define an "IPW" layer - it only requires that
 *                the Mem driver's public API (Mem.c) be memory-device agnostic ([SWS_Mem_00035]) and,
 *                when built as a separate binary, self-contained ([SWS_Mem_00039]). Following the common
 *                AUTOSAR MCAL convention (e.g. Fls_Ipw, Adc_Ipw, ...), Mem_IPW is the thin layer that sits
 *                between the SWS-compliant Mem.c and the actual memory device / IP register driver, so
 *                that porting to new hardware only requires re-implementing Mem_IPW.c.
 *
 *                For this project, Mem_IPW.c wraps Flash_IP (Flash_IP.h/.c), the bare-metal register
 *                level driver for the STM32F401RE internal Flash. All chip-specific details (registers,
 *                sector geometry, IRQ handling) live in Flash_IP; Mem_IPW.c only translates between the
 *                Mem driver's generic instance-based API and Flash_IP's single-Flash-device API.
 *********************************************************************************************************************/

#ifndef MEM_IPW_H
#define MEM_IPW_H

#include "Mem.h"

/*======================================================================================================================
 *  Lifecycle
 *====================================================================================================================*/

/* Initializes hardware-specific state for one Mem driver instance. Called from Mem_Init() [SWS_Mem_00001]. */
extern void Mem_Ipw_Init(Mem_InstanceIdType instanceId);

/* Cancels any ongoing hardware operation and de-initializes hardware state for one instance.
 * Called from Mem_DeInit() [SWS_Mem_00079]. */
extern void Mem_Ipw_DeInit(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Optional service capability queries
 *  Used by Mem.c to reject unavailable services synchronously with E_MEM_SERVICE_NOT_AVAIL, as required
 *  by [SWS_Mem_00070] and the Suspend/Resume rules in [SWS_Mem_00082].
 *====================================================================================================================*/

extern boolean Mem_Ipw_IsHwSpecificServiceSupported(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId);

extern boolean Mem_Ipw_IsSuspendResumeSupported(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Asynchronous memory operations
 *  Each function only triggers the hardware operation and returns immediately. Progress is advanced by
 *  Mem_Ipw_MainFunction() and the outcome is retrieved with Mem_Ipw_GetJobResult().
 *====================================================================================================================*/

extern Std_ReturnType Mem_Ipw_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length);

extern Std_ReturnType Mem_Ipw_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length);

extern Std_ReturnType Mem_Ipw_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr);

/*======================================================================================================================
 *  Suspend / Resume  [SRS_MemHwAb_14031] / [SWS_Mem_00082]
 *====================================================================================================================*/

extern Std_ReturnType Mem_Ipw_Suspend(Mem_InstanceIdType instanceId);
extern Std_ReturnType Mem_Ipw_Resume(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Scheduling / job result retrieval
 *====================================================================================================================*/

/* Advances the ongoing hardware operation (if any) of the given instance by one step.
 * Called from Mem_MainFunction() [SWS_Mem_00066]. */
extern void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId);

/* Returns the current hardware job result for the given instance (MEM_JOB_OK / MEM_JOB_PENDING /
 * MEM_JOB_FAILED / MEM_INCONSISTENT / MEM_ECC_CORRECTED / MEM_ECC_UNCORRECTED). */
extern MemAcc_MemJobResultType Mem_Ipw_GetJobResult(Mem_InstanceIdType instanceId);

/*======================================================================================================================
 *  Address / length validation helpers
 *  Used by Mem.c to implement [SWS_Mem_00006][SWS_Mem_00072][SWS_Mem_00011][SWS_Mem_00012]
 *  [SWS_Mem_00016][SWS_Mem_00017][SWS_Mem_00023][SWS_Mem_00024] development error checks.
 *====================================================================================================================*/

extern boolean Mem_Ipw_IsAddressValid(Mem_InstanceIdType instanceId, Mem_AddressType address);

extern boolean Mem_Ipw_IsLengthValid(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length);

/* [SWS_Mem_00035]: used by Mem.c's Mem_CheckEraseAlignment() to synchronously reject Mem_Erase() requests
 * that do not exactly match one physical sector, BEFORE the job is accepted (per [SWS_Mem_00059]) - i.e.
 * before it is queued for the deferred hardware trigger performed in Mem_MainFunction(). */
extern boolean Mem_Ipw_IsEraseAligned(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length);

#endif /* MEM_IPW_H */
