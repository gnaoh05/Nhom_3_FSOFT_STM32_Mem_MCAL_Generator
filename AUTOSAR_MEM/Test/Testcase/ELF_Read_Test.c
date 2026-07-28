/******************************************************************************
 * FILE: ELF_Read_Test.c
 * DESCRIPTION: Read-path tests for the AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_Read_Test.h"
#include "TestManager.h"

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00066], [SWS_Mem_00067], p.15,36
 * - Confirms a valid read request is executed by Mem_MainFunction() and copies the expected flash bytes.
 */
static void ELF_READ_001(void)
{
    Mem_DataType            readBuffer[TEST_FLASH_COMPARE_LENGTH];
    Mem_DataType            referenceBuffer[TEST_FLASH_COMPARE_LENGTH];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;
    boolean                 passed;

    TestManager_PrepareDriver();
    TestManager_CopyFromFlash((Mem_AddressType)FLASH_IP_BASE_ADDRESS, referenceBuffer, TEST_FLASH_COMPARE_LENGTH);

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      readBuffer,
                      TEST_FLASH_COMPARE_LENGTH);

    jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
    passed = (boolean)((retVal == E_OK) &&
                       (jobResult == MEM_JOB_OK) &&
                       (TestManager_BufferEquals(readBuffer, referenceBuffer, TEST_FLASH_COMPARE_LENGTH) == TRUE));

    TestManager_RecordCaseTrace(0x0401u, "ReadValidAddress",
                                "SWS_Mem_10012,SWS_Mem_00066,SWS_Mem_00067",
                                "p.15,36",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00006], p.36
 * - Verifies Mem_Read() rejects an invalid address with MEM_E_PARAM_ADDRESS.
 */
static void ELF_READ_002(void)
{
    Mem_DataType   readBuffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, TEST_FLASH_INVALID_ADDRESS, readBuffer, (Mem_LengthType)sizeof(readBuffer));

    TestManager_RecordCaseTrace(0x0402u, "ReadRejectsInvalidAddress",
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
static void ELF_READ_003(void)
{
    Mem_DataType   readBuffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      readBuffer,
                      (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0403u, "ReadRejectsInvalidLength",
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
static void ELF_READ_004(void)
{
    Mem_DataType   readBuffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INVALID_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      readBuffer,
                      (Mem_LengthType)sizeof(readBuffer));

    TestManager_RecordCaseTrace(0x0404u, "ReadRejectsInvalidInstance",
                                "SWS_Mem_10012,SWS_Mem_00004",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00007], p.36
 * - Verifies Mem_Read() rejects a second read request while a previous job is still pending.
 */
static void ELF_READ_005(void)
{
    Mem_DataType   readBufferA[8];
    Mem_DataType   readBufferB[8];
    Std_ReturnType firstRetVal;
    Std_ReturnType secondRetVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    firstRetVal = Mem_Read(TEST_FLASH_INSTANCE,
                           (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                           readBufferA,
                           (Mem_LengthType)sizeof(readBufferA));
    secondRetVal = Mem_Read(TEST_FLASH_INSTANCE,
                            (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                            readBufferB,
                            (Mem_LengthType)sizeof(readBufferB));

    TestManager_RecordCaseTrace(0x0405u, "ReadRejectsPendingRequest",
                                "SWS_Mem_10012,SWS_Mem_00007",
                                "p.36",
                                (boolean)((firstRetVal == E_OK) &&
                                          (secondRetVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_JOB_PENDING) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)secondRetVal);
}

void ELF_Read_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_READ);

    ELF_READ_001();
    ELF_READ_002();
    ELF_READ_003();
    ELF_READ_004();
    ELF_READ_005();

    TestManager_EndGroup();
}
