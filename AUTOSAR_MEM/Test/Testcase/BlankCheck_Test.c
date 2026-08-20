/******************************************************************************
 * FILE: BlankCheck_Test.c
 * MÔ TẢ: Test blank-check cho AUTOSAR Mem driver
 ******************************************************************************/

#include "BlankCheck_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00066], [SWS_Mem_00067], p.15,40-41
 * - Kiểm tra BlankCheck trên sector đã xóa hoàn tất thành công với MEM_JOB_OK.
 */
static void BLANKCHECK_001(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00076], p.16,40-41
 * - Kiểm tra BlankCheck báo MEM_INCONSISTENT khi sector được kiểm tra không blank.
 */
static void BLANKCHECK_002(void)
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
//Tạo thêm 3 trường hợp biên đầu giữa và cuối
/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00022], p.40
 * - Kiểm tra Mem_BlankCheck() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void BLANKCHECK_003(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00023], p.40
 * - Kiểm tra Mem_BlankCheck() từ chối địa chỉ không hợp lệ với MEM_E_PARAM_ADDRESS.
 */
static void BLANKCHECK_004(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00024], p.40
 * - Kiểm tra Mem_BlankCheck() từ chối độ dài không hợp lệ với MEM_E_PARAM_LENGTH.
 */
static void BLANKCHECK_005(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10016], [SWS_Mem_00025], p.40-41
 * - Kiểm tra Mem_BlankCheck() từ chối yêu cầu khi một job khác đang pending.
 */
static void BLANKCHECK_006(void)
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

void BlankCheck_Test(void)
{
    TestManager_BeginGroup(TEST_BLANKCHECK);

    BLANKCHECK_001();
    BLANKCHECK_002();
    BLANKCHECK_003();
    BLANKCHECK_004();
    BLANKCHECK_005();
    BLANKCHECK_006();

    TestManager_EndGroup();
}
