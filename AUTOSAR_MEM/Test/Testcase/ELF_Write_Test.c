/******************************************************************************
 * FILE: ELF_Write_Test.c
 * MÔ TẢ: Test đường ghi cho AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_Write_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00066], [SWS_Mem_00067], [SWS_Mem_00088], p.15,24,37-38
 * - Kiểm tra yêu cầu ghi hợp lệ hoàn tất với MEM_JOB_OK; đọc lại Flash là cơ chế xác nhận của upper layer,
 *   phù hợp với [SWS_Mem_00088].
 */
static void ELF_WRITE_001(void)
{
    Mem_DataType            writePattern[TEST_FLASH_COMPARE_LENGTH];
    Mem_DataType            readBack[TEST_FLASH_COMPARE_LENGTH];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    if (TestManager_IsFlashTestAreaSafe() == TRUE)
    {
        TestManager_FillPattern(writePattern, TEST_FLASH_COMPARE_LENGTH, 0x31u);

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
                    TestManager_CopyFromFlash(TEST_FLASH_WRITE_ADDRESS, readBack, TEST_FLASH_COMPARE_LENGTH);
                    passed = TestManager_BufferEquals(writePattern, readBack, TEST_FLASH_COMPARE_LENGTH);
                }
            }
        }
    }

    TestManager_RecordCaseTrace(0x0501u, "WriteValidPattern",
                                "SWS_Mem_10013,SWS_Mem_00066,SWS_Mem_00067,SWS_Mem_00088",
                                "p.15,24,37-38",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00011], p.38
 * - Kiểm tra Mem_Write() từ chối địa chỉ không hợp lệ với MEM_E_PARAM_ADDRESS.
 */
static void ELF_WRITE_002(void)
{
    Mem_DataType   writePattern[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_FillPattern(writePattern, (Mem_LengthType)sizeof(writePattern), 0x44u);
    TestManager_ResetDetState();

    retVal = Mem_Write(TEST_FLASH_INSTANCE, TEST_FLASH_INVALID_ADDRESS, writePattern, (Mem_LengthType)sizeof(writePattern));

    TestManager_RecordCaseTrace(0x0502u, "WriteRejectsInvalidAddress",
                                "SWS_Mem_10013,SWS_Mem_00011",
                                "p.38",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00009], p.37-38
 * - Kiểm tra Mem_Write() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void ELF_WRITE_003(void)
{
    Mem_DataType   writePattern[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_FillPattern(writePattern, (Mem_LengthType)sizeof(writePattern), 0x52u);
    TestManager_ResetDetState();

    retVal = Mem_Write(TEST_FLASH_INVALID_INSTANCE,
                       TEST_FLASH_WRITE_ADDRESS,
                       writePattern,
                       (Mem_LengthType)sizeof(writePattern));

    TestManager_RecordCaseTrace(0x0503u, "WriteRejectsInvalidInstance",
                                "SWS_Mem_10013,SWS_Mem_00009",
                                "p.37-38",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00010], p.37-38
 * - Kiểm tra Mem_Write() từ chối source buffer NULL với MEM_E_PARAM_POINTER.
 */
static void ELF_WRITE_004(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Write(TEST_FLASH_INSTANCE,
                       TEST_FLASH_WRITE_ADDRESS,
                       NULL_PTR,
                       4u);

    TestManager_RecordCaseTrace(0x0504u, "WriteRejectsNullPointer",
                                "SWS_Mem_10013,SWS_Mem_00010",
                                "p.37-38",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_PARAM_POINTER) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00012], p.38
 * - Kiểm tra Mem_Write() từ chối độ dài không hợp lệ với MEM_E_PARAM_LENGTH.
 */
static void ELF_WRITE_005(void)
{
    Mem_DataType   writePattern[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_FillPattern(writePattern, (Mem_LengthType)sizeof(writePattern), 0x63u);
    TestManager_ResetDetState();

    retVal = Mem_Write(TEST_FLASH_INSTANCE,
                       TEST_FLASH_WRITE_ADDRESS,
                       writePattern,
                       (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0505u, "WriteRejectsInvalidLength",
                                "SWS_Mem_10013,SWS_Mem_00012",
                                "p.38",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_PARAM_LENGTH) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00013], p.38
 * - Kiểm tra Mem_Write() từ chối yêu cầu khi một job khác đang pending.
 */
static void ELF_WRITE_006(void)
{
    Mem_DataType   readBuffer[8];
    Mem_DataType   writePattern[4];
    Std_ReturnType firstRetVal;
    Std_ReturnType secondRetVal;

    TestManager_PrepareDriver();
    TestManager_FillPattern(writePattern, (Mem_LengthType)sizeof(writePattern), 0x74u);
    TestManager_ResetDetState();

    firstRetVal = Mem_Read(TEST_FLASH_INSTANCE,
                           (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                           readBuffer,
                           (Mem_LengthType)sizeof(readBuffer));
    secondRetVal = Mem_Write(TEST_FLASH_INSTANCE,
                             TEST_FLASH_WRITE_ADDRESS,
                             writePattern,
                             (Mem_LengthType)sizeof(writePattern));

    TestManager_RecordCaseTrace(0x0506u, "WriteRejectsPendingJob",
                                "SWS_Mem_10013,SWS_Mem_00013",
                                "p.38",
                                (boolean)((firstRetVal == E_OK) &&
                                          (secondRetVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_JOB_PENDING) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)secondRetVal);
}

void ELF_Write_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_WRITE);

    ELF_WRITE_001();
    ELF_WRITE_002();
    ELF_WRITE_003();
    ELF_WRITE_004();
    ELF_WRITE_005();
    ELF_WRITE_006();

    TestManager_EndGroup();
}
