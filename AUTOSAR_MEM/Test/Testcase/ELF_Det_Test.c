/******************************************************************************
 * FILE: ELF_Det_Test.c
 * DESCRIPTION: DET-oriented tests for the AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_Det_Test.h"
#include "TestManager.h"

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: section 7.3.1, p.23; [SWS_Mem_00052], p.25
 * - Derived check that an API used before initialization is rejected with MEM_E_UNINIT.
 */
static void ELF_DET_001(void)
{
    Mem_DataType    buffer[4];
    Std_ReturnType  retVal;

    TestManager_PrepareDriver();
    Mem_DeInit();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0201u, "ReadBeforeInit",
                                "Section7.3.1,SWS_Mem_00052",
                                "p.23,25",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_READ, MEM_E_UNINIT) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00005], p.36
 * - Verifies Mem_Read() rejects a NULL destination buffer with MEM_E_PARAM_POINTER.
 */
static void ELF_DET_002(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, NULL_PTR, 4u);

    TestManager_RecordCaseTrace(0x0202u, "ReadNullPointer",
                                "SWS_Mem_10012,SWS_Mem_00005",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_POINTER) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00006], p.36
 * - Verifies Mem_Read() rejects an invalid address with MEM_E_PARAM_ADDRESS.
 */
static void ELF_DET_003(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, TEST_FLASH_INVALID_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0203u, "ReadInvalidAddress",
                                "SWS_Mem_10012,SWS_Mem_00006",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00072], p.36
 * - Verifies Mem_Read() rejects an invalid length with MEM_E_PARAM_LENGTH.
 */
static void ELF_DET_004(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0204u, "ReadInvalidLength",
                                "SWS_Mem_10012,SWS_Mem_00072",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_LENGTH) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00004], p.36
 * - Verifies Mem_Read() rejects an invalid instance ID with MEM_E_PARAM_INSTANCE_ID.
 */
static void ELF_DET_005(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INVALID_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0205u, "ReadInvalidInstance",
                                "SWS_Mem_10012,SWS_Mem_00004",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

void ELF_Det_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_DET);

    ELF_DET_001();
    ELF_DET_002();
    ELF_DET_003();
    ELF_DET_004();
    ELF_DET_005();

    TestManager_EndGroup();
}
