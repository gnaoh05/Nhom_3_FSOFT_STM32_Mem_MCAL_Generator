/******************************************************************************
 * FILE: Read_Test.c
 * MÔ TẢ: Test đường đọc cho AUTOSAR Mem driver
 ******************************************************************************/

#include "Read_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00066], [SWS_Mem_00067], p.15,36
 * - Xác nhận yêu cầu đọc hợp lệ được Mem_MainFunction() thực thi và sao chép đúng byte Flash mong đợi.
 */
static void READ_001(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00006], p.36
 * - Kiểm tra Mem_Read() từ chối địa chỉ không hợp lệ với MEM_E_PARAM_ADDRESS.
 */
static void READ_002(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00072], p.36
 * - Kiểm tra Mem_Read() từ chối độ dài không hợp lệ với MEM_E_PARAM_LENGTH.
 */
static void READ_003(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00004], p.36
 * - Kiểm tra Mem_Read() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void READ_004(void)
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

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00007], p.36
 * - Kiểm tra Mem_Read() từ chối yêu cầu đọc thứ hai khi job trước vẫn pending.
 */
static void READ_005(void)
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

void Read_Test(void)
{
    TestManager_BeginGroup(TEST_READ);

    READ_001();
    READ_002();
    READ_003();
    READ_004();
    READ_005();

    TestManager_EndGroup();
}
