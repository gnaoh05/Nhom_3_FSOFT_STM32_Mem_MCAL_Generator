/******************************************************************************
 * FILE: ELF_Erase_Test.c
 * DESCRIPTION: Erase-path tests for the AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_Erase_Test.h"
#include "TestManager.h"

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00066], [SWS_Mem_00067], [SWS_Mem_00088], p.15,24,38-39
 * - Verifies a valid sector erase request completes with MEM_JOB_OK; erased-state inspection is the
 *   upper-layer confirmation mechanism aligned with [SWS_Mem_00088].
 */
static void ELF_ERASE_001(void)
{
    Mem_DataType            writePattern[TEST_FLASH_COMPARE_LENGTH];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    if (TestManager_IsFlashTestAreaSafe() == TRUE)
    {
        TestManager_FillPattern(writePattern, TEST_FLASH_COMPARE_LENGTH, 0x55u);

        if (TestManager_EraseTestSectorAndWait() == TRUE)
        {
            retVal = Mem_Write(TEST_FLASH_INSTANCE,
                               TEST_FLASH_WRITE_ADDRESS,
                               writePattern,
                               TEST_FLASH_COMPARE_LENGTH);

            if (retVal == E_OK)
            {
                jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

                if (jobResult == MEM_JOB_OK)
                {
                    retVal = Mem_Erase(TEST_FLASH_INSTANCE, TEST_FLASH_SECTOR_ADDRESS, TEST_FLASH_SECTOR_LENGTH);

                    if (retVal == E_OK)
                    {
                        jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
                        passed = (boolean)((jobResult == MEM_JOB_OK) &&
                                           (TestManager_IsFlashRangeErased(TEST_FLASH_WRITE_ADDRESS, TEST_FLASH_COMPARE_LENGTH) == TRUE));
                    }
                }
            }
        }
    }

    TestManager_RecordCaseTrace(0x0601u, "EraseValidSector",
                                "SWS_Mem_10014,SWS_Mem_00066,SWS_Mem_00067,SWS_Mem_00088",
                                "p.15,24,38-39",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00016], [SWS_Mem_00035], p.24,39
 * - Verifies Mem_Erase() rejects a request that does not match physical sector boundaries.
 */
static void ELF_ERASE_002(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Erase(TEST_FLASH_INSTANCE,
                       TEST_FLASH_WRITE_ADDRESS,
                       TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0602u, "EraseRejectsMisalignedRequest",
                                "SWS_Mem_10014,SWS_Mem_00016,SWS_Mem_00035",
                                "p.24,39",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_ERASE, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00016], p.39
 * - Verifies Mem_Erase() rejects an address outside the configured flash range.
 */
static void ELF_ERASE_003(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Erase(TEST_FLASH_INSTANCE,
                       TEST_FLASH_INVALID_ADDRESS,
                       TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0603u, "EraseRejectsInvalidAddress",
                                "SWS_Mem_10014,SWS_Mem_00016",
                                "p.39",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_ERASE, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00015], p.39
 * - Verifies Mem_Erase() rejects an invalid instance ID with MEM_E_PARAM_INSTANCE_ID.
 */
static void ELF_ERASE_004(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Erase(TEST_FLASH_INVALID_INSTANCE,
                       TEST_FLASH_SECTOR_ADDRESS,
                       TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0604u, "EraseRejectsInvalidInstance",
                                "SWS_Mem_10014,SWS_Mem_00015",
                                "p.39",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_ERASE, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00017], p.39
 * - Verifies Mem_Erase() rejects an invalid length with MEM_E_PARAM_LENGTH.
 */
static void ELF_ERASE_005(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Erase(TEST_FLASH_INSTANCE,
                       TEST_FLASH_SECTOR_ADDRESS,
                       (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0605u, "EraseRejectsInvalidLength",
                                "SWS_Mem_10014,SWS_Mem_00017",
                                "p.39",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_ERASE, MEM_E_PARAM_LENGTH) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00018], p.39
 * - Verifies Mem_Erase() rejects a request while another job is already pending.
 */
static void ELF_ERASE_006(void)
{
    Mem_DataType   readBuffer[8];
    Std_ReturnType firstRetVal;
    Std_ReturnType secondRetVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    firstRetVal = Mem_Read(TEST_FLASH_INSTANCE,
                           (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                           readBuffer,
                           (Mem_LengthType)sizeof(readBuffer));
    secondRetVal = Mem_Erase(TEST_FLASH_INSTANCE,
                             TEST_FLASH_SECTOR_ADDRESS,
                             TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0606u, "EraseRejectsPendingJob",
                                "SWS_Mem_10014,SWS_Mem_00018",
                                "p.39",
                                (boolean)((firstRetVal == E_OK) &&
                                          (secondRetVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_ERASE, MEM_E_JOB_PENDING) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)secondRetVal);
}

void ELF_Erase_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_ERASE);

    ELF_ERASE_001();
    ELF_ERASE_002();
    ELF_ERASE_003();
    ELF_ERASE_004();
    ELF_ERASE_005();
    ELF_ERASE_006();

    TestManager_EndGroup();
}
