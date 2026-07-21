/**********************************************************************************************************************
 *  FILE:         Mem_Cfg.h
 *  MODULE:       Mem (Memory Driver) - Pre-Compile Configuration
 *  DESCRIPTION:  Example generated configuration for the Mem driver, corresponding to chapter 10
 *                "Configuration Specification" of AUTOSAR_CP_SWS_MemoryDriver (Doc ID 1018, R25-11).
 *
 *                Maps to:
 *                  MemGeneral            -> [ECUC_Mem_00002]
 *                  MemDevErrorDetect     -> [ECUC_Mem_00004]
 *                  MemIndex              -> [ECUC_Mem_00023]
 *                  MemInvocation         -> [ECUC_Mem_00025]
 *                  MemMainFunctionPeriod -> [ECUC_Mem_00029]
 *                  MemInstance           -> [ECUC_Mem_00003]/[ECUC_Mem_00007]
 *                  MemSectorBatch        -> [ECUC_Mem_00009] .. [ECUC_Mem_00014]
 *                  MemPublishedInformation/MemErasedValue -> [ECUC_Mem_00020]/[ECUC_Mem_00021]
 *
 *                NOTE: This is a hand-written EXAMPLE configuration for one Mem driver instance mapped to
 *                the STM32F401RE internal Flash (512 KB / 8 sectors, addresses 0x08000000-0x0807FFFF).
 *                Replace with the output of your configuration tool for a real project.
 *********************************************************************************************************************/

#ifndef MEM_CFG_H
#define MEM_CFG_H

#include "Std_Types.h"

/*======================================================================================================================
 *  MemGeneral container  [ECUC_Mem_00002]
 *====================================================================================================================*/

/* MemDevErrorDetect [ECUC_Mem_00004] - switches DET on/off */
#define MEM_DEV_ERROR_DETECT            STD_ON

/* Version info API, see [SWS_Mem_10009] / SRS_BSW_00003 */
#define MEM_VERSION_INFO_API            STD_ON

/* MemIndex [ECUC_Mem_00023] - InstanceId of this Mem driver module instance (0 if only one Mem driver is used) */
#define MEM_INDEX                       0u

/* MemInvocation [ECUC_Mem_00025] - DIRECT_STATIC | INDIRECT_STATIC | INDIRECT_DYNAMIC */
#define MEM_INVOCATION_DIRECT_STATIC    0u
#define MEM_INVOCATION_INDIRECT_STATIC  1u
#define MEM_INVOCATION_INDIRECT_DYNAMIC 2u
#define MEM_INVOCATION                  MEM_INVOCATION_DIRECT_STATIC

/* [SWS_Mem_00038]/[SRS_MemHwAb_14045]/[SRS_MemHwAb_14049]: INDIRECT_STATIC and INDIRECT_DYNAMIC
 * invocation require the standardized Mem driver binary image format of chapter 7.2.6 (header, service
 * function pointer table, delimiter, dynamic driver activation) - NOT implemented by this reference
 * driver (out-of-scope decision, see project traceability review: no OTA background update use case).
 * Fail at compile time rather than silently accepting a MemInvocation value this driver cannot honor. */
#if (MEM_INVOCATION != MEM_INVOCATION_DIRECT_STATIC)
#error "Only MEM_INVOCATION_DIRECT_STATIC is implemented by this reference Mem driver. INDIRECT_STATIC/INDIRECT_DYNAMIC require the chapter 7.2.6 binary image format, which is not implemented - see Mem_Cfg.h."
#endif

/* MemMainFunctionPeriod [ECUC_Mem_00029] in seconds (for documentation / SchM configuration only) */
#define MEM_MAIN_FUNCTION_PERIOD        0.005f

/*======================================================================================================================
 *  MemInstance containers  [ECUC_Mem_00003]
 *  The STM32F401RE has a single internal Flash memory device, so exactly one Mem driver instance is
 *  configured, mapped to that device ([SRS_MemHwAb_14043] multi-instance support is not exercised here,
 *  but the driver remains generic - see MEM_INSTANCE_COUNT).
 *====================================================================================================================*/

#define MEM_INSTANCE_COUNT              1u

/* Symbolic name for MemInstanceId [ECUC_Mem_00007] */
#define MemConf_MemInstance_MemInstance_0   0u

/* MemSectorBatch parameters [ECUC_Mem_00009]..[ECUC_Mem_00014].
 * The STM32F401RE Flash is NOT uniformly segmented (4x16KB + 1x64KB + 3x128KB sectors), so the actual
 * per-sector geometry is owned by Flash_IP.c (Flash_IP_GetSectorFromAddress/StartAddress/Size()) rather
 * than duplicated here. Only the overall address range is needed by Mem_Cfg.h for documentation /
 * cross-checking purposes; Mem_IPW.c validates addresses/lengths against Flash_IP directly. */

#define MEM_INSTANCE_0_START_ADDRESS     0x08000000UL   /* MemStartAddress [ECUC_Mem_00014] - Flash base   */
#define MEM_INSTANCE_0_SIZE              0x00080000UL   /* Total size: 512 KB                              */

/*======================================================================================================================
 *  MemPublishedInformation container  [ECUC_Mem_00020]
 *====================================================================================================================*/

/* MemErasedValue [ECUC_Mem_00021] - contents of an erased memory cell.
 * Exposed both as a compile-time macro (for use in preprocessor conditions / static initializers) and
 * as a runtime-readable const symbol Mem_ErasedValue (defined in Mem_Cfg.c), matching how AUTOSAR
 * "Published Information" parameters are typically also made available as data, not just macros, so
 * other modules/tools can query them without recompiling against Mem_Cfg.h. */
#define MEM_ERASED_VALUE                 0xFFu

extern const uint32 Mem_ErasedValue;

#endif /* MEM_CFG_H */