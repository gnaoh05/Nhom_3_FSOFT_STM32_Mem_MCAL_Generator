/******************************************************************************
 * FILE: ELF_BlankCheck_Test.c
 * DESCRIPTION: Blank-check tests for the AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_BlankCheck_Test.h"
#include "TestManager.h"

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00066], [SWS_Mem_00067], p.15,40-41
 * - Verifies BlankCheck on an erased sector completes successfully with MEM_JOB_OK.
 */
static void ELF_BLANKCHECK_001(void)
{
    Std_ReturnType          retVal = E_NOT_OK;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    if ((TestManager_IsFlashTestAreaSafe() == TRUE) &&
        (TestManager_EraseTestSectorAndWait() == TRUE))
    {
        retVal = Mem_BlankCheck(TEST_FLASH_INSTANCE,
                                TEST_FLASH_SECTOR_ADDRESS,
                                TEST_FLASH_SECTOR_LENGTH);

        if (retVal == E_OK)
        {
            jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
            passed = (boolean)(jobResult == MEM_JOB_OK);
        }
    }

    TestManager_RecordCaseTrace(0x0701u, "BlankSectorReturnsOk",
                                "SWS_Mem_10016,SWS_Mem_00066,SWS_Mem_00067",
                                "p.15,40-41",
                                passed,
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00076], p.16,40-41
 * - Verifies BlankCheck reports MEM_INCONSISTENT when the checked sector is not blank.
 */
static void ELF_BLANKCHECK_002(void)
{
    Mem_DataType            writePattern[4];
    Std_ReturnType          retVal = E_NOT_OK;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();
    TestManager_FillPattern(writePattern, (Mem_LengthType)sizeof(writePattern), 0x77u);

    if ((TestManager_IsFlashTestAreaSafe() == TRUE) &&
        (TestManager_EraseTestSectorAndWait() == TRUE))
    {
        retVal = Mem_Write(TEST_FLASH_INSTANCE,
                           TEST_FLASH_WRITE_ADDRESS,
                           writePattern,
                           (Mem_LengthType)sizeof(writePattern));

        if (retVal == E_OK)
        {
            jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

            if (jobResult == MEM_JOB_OK)
            {
                retVal = Mem_BlankCheck(TEST_FLASH_INSTANCE,
                                        TEST_FLASH_SECTOR_ADDRESS,
                                        TEST_FLASH_SECTOR_LENGTH);

                if (retVal == E_OK)
                {
                    jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
                    passed = (boolean)(jobResult == MEM_INCONSISTENT);
                }
            }
        }
    }

    TestManager_RecordCaseTrace(0x0702u, "NonBlankSectorReturnsInconsistent",
                                "SWS_Mem_10016,SWS_Mem_00076",
                                "p.16,40-41",
                                passed,
                                (uint32)MEM_INCONSISTENT,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00022], p.40
 * - Verifies Mem_BlankCheck() rejects an invalid instance ID with MEM_E_PARAM_INSTANCE_ID.
 */
static void ELF_BLANKCHECK_003(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_BlankCheck(TEST_FLASH_INVALID_INSTANCE,
                            TEST_FLASH_SECTOR_ADDRESS,
                            TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0703u, "BlankCheckRejectsInvalidInstance",
                                "SWS_Mem_10016,SWS_Mem_00022",
                                "p.40",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_BLANK_CHECK, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00023], p.40
 * - Verifies Mem_BlankCheck() rejects an invalid address with MEM_E_PARAM_ADDRESS.
 */
static void ELF_BLANKCHECK_004(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_BlankCheck(TEST_FLASH_INSTANCE,
                            TEST_FLASH_INVALID_ADDRESS,
                            TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0704u, "BlankCheckRejectsInvalidAddress",
                                "SWS_Mem_10016,SWS_Mem_00023",
                                "p.40",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_BLANK_CHECK, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00024], p.40
 * - Verifies Mem_BlankCheck() rejects an invalid length with MEM_E_PARAM_LENGTH.
 */
static void ELF_BLANKCHECK_005(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_BlankCheck(TEST_FLASH_INSTANCE,
                            TEST_FLASH_SECTOR_ADDRESS,
                            (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0705u, "BlankCheckRejectsInvalidLength",
                                "SWS_Mem_10016,SWS_Mem_00024",
                                "p.40",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_BLANK_CHECK, MEM_E_PARAM_LENGTH) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00025], p.40-41
 * - Verifies Mem_BlankCheck() rejects a request while another job is already pending.
 */
static void ELF_BLANKCHECK_006(void)
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
    secondRetVal = Mem_BlankCheck(TEST_FLASH_INSTANCE,
                                  TEST_FLASH_SECTOR_ADDRESS,
                                  TEST_FLASH_SECTOR_LENGTH);

    TestManager_RecordCaseTrace(0x0706u, "BlankCheckRejectsPendingJob",
                                "SWS_Mem_10016,SWS_Mem_00025",
                                "p.40-41",
                                (boolean)((firstRetVal == E_OK) &&
                                          (secondRetVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_BLANK_CHECK, MEM_E_JOB_PENDING) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)secondRetVal);
}

void ELF_BlankCheck_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_BLANKCHECK);

    ELF_BLANKCHECK_001();
    ELF_BLANKCHECK_002();
    ELF_BLANKCHECK_003();
    ELF_BLANKCHECK_004();
    ELF_BLANKCHECK_005();
    ELF_BLANKCHECK_006();

    TestManager_EndGroup();
}
