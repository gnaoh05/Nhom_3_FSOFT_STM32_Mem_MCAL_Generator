/******************************************************************************
 * FILE: Job_Test.c
 * MÔ TẢ: Test điều khiển job cho AUTOSAR Mem driver
 ******************************************************************************/

#include "Job_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_00057], [SWS_Mem_00059], [SWS_Mem_00007], [SWS_Mem_10012], p.14-15,36
 * - Kiểm tra yêu cầu job thứ hai bị từ chối khi job thứ nhất vẫn pending.
 */
static void JOB_001(void)
{
    Mem_DataType   readBufferA[8];
    Mem_DataType   readBufferB[8];
    Std_ReturnType firstRetVal;
    Std_ReturnType secondRetVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    firstRetVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, readBufferA, (Mem_LengthType)sizeof(readBufferA));
    secondRetVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, readBufferB, (Mem_LengthType)sizeof(readBufferB));

    TestManager_RecordCaseTrace(0x0801u, "RejectSecondPendingJob",
                                "SWS_Mem_00057,SWS_Mem_00059,SWS_Mem_00007,SWS_Mem_10012",
                                "p.14-15,36",
                                (boolean)((firstRetVal == E_OK) &&
                                          (secondRetVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_JOB_PENDING) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)secondRetVal);

    (void)TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10015], [SWS_Mem_00061], p.35
 * - Kiểm tra Mem_PropagateError() hủy job hiện tại và đặt MEM_ECC_UNCORRECTED.
 */
static void JOB_002(void)
{
    Mem_DataType            readBuffer[8];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;

    TestManager_PrepareDriver();
    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, readBuffer, (Mem_LengthType)sizeof(readBuffer));

    if (retVal == E_OK)
    {
        Mem_PropagateError(TEST_FLASH_INSTANCE);
    }

    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0802u, "PropagateErrorCancelsJob",
                                "SWS_Mem_10015,SWS_Mem_00061",
                                "p.35",
                                (boolean)((retVal == E_OK) && (jobResult == MEM_ECC_UNCORRECTED)),
                                (uint32)MEM_ECC_UNCORRECTED,
                                (uint32)jobResult);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10017], [SWS_Mem_00070], p.23,41
 * - Kiểm tra hardware-specific service không được hỗ trợ trả E_MEM_SERVICE_NOT_AVAIL.
 */
static void JOB_003(void)
{
    Mem_DataType   dataBuffer[4] = { 0u, 0u, 0u, 0u };
    Mem_LengthType length = (Mem_LengthType)sizeof(dataBuffer);
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    retVal = Mem_HwSpecificService(TEST_FLASH_INSTANCE, 0u, dataBuffer, &length);

    TestManager_RecordCaseTrace(0x0803u, "HwSpecificServiceNotAvailable",
                                "SWS_Mem_10017,SWS_Mem_00070",
                                "p.23,41",
                                (boolean)(retVal == E_MEM_SERVICE_NOT_AVAIL),
                                (uint32)E_MEM_SERVICE_NOT_AVAIL,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10024], [SWS_Mem_00082], p.16,33
 * - Kiểm tra Mem_Suspend() trả E_MEM_SERVICE_NOT_AVAIL khi công nghệ bộ nhớ không hỗ trợ suspend.
 */
static void JOB_004(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    retVal = Mem_Suspend(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0804u, "SuspendNotAvailable",
                                "SWS_Mem_10024,SWS_Mem_00082",
                                "p.16,33",
                                (boolean)(retVal == E_MEM_SERVICE_NOT_AVAIL),
                                (uint32)E_MEM_SERVICE_NOT_AVAIL,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10025], [SWS_Mem_00082], p.16,34
 * - Kiểm tra Mem_Resume() trả E_MEM_SERVICE_NOT_AVAIL khi công nghệ bộ nhớ không hỗ trợ resume.
 */
static void JOB_005(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    retVal = Mem_Resume(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0805u, "ResumeNotAvailable",
                                "SWS_Mem_10025,SWS_Mem_00082",
                                "p.16,34",
                                (boolean)(retVal == E_MEM_SERVICE_NOT_AVAIL),
                                (uint32)E_MEM_SERVICE_NOT_AVAIL,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10015], [SWS_Mem_00020], p.35
 * - Kiểm tra Mem_PropagateError() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void JOB_006(void)
{
    MemAcc_MemJobResultType jobResult;
    boolean                 passed;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    Mem_PropagateError(TEST_FLASH_INVALID_INSTANCE);
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);
    passed = (boolean)((jobResult == MEM_JOB_OK) &&
                       (TestManager_CheckDet(MEM_SID_PROPAGATE_ERROR, MEM_E_PARAM_INSTANCE_ID) == TRUE));

    TestManager_RecordCaseTrace(0x0806u, "PropagateErrorInvalidInstance",
                                "SWS_Mem_10015,SWS_Mem_00020",
                                "p.35",
                                passed,
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10017], [SWS_Mem_00026], p.41-42
 * - Kiểm tra Mem_HwSpecificService() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void JOB_007(void)
{
    Mem_DataType   dataBuffer[4] = { 0u, 0u, 0u, 0u };
    Mem_LengthType length = (Mem_LengthType)sizeof(dataBuffer);
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_HwSpecificService(TEST_FLASH_INVALID_INSTANCE, 0u, dataBuffer, &length);

    TestManager_RecordCaseTrace(0x0807u, "HwSpecificRejectsInvalidInstance",
                                "SWS_Mem_10017,SWS_Mem_00026",
                                "p.41-42",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_HW_SPECIFIC_SERVICE, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10017], [SWS_Mem_00027], p.41-42
 * - Kiểm tra Mem_HwSpecificService() từ chối data pointer NULL với MEM_E_PARAM_POINTER.
 */
static void JOB_008(void)
{
    Mem_LengthType length = 4u;
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_HwSpecificService(TEST_FLASH_INSTANCE, 0u, NULL_PTR, &length);

    TestManager_RecordCaseTrace(0x0808u, "HwSpecificRejectsNullDataPtr",
                                "SWS_Mem_10017,SWS_Mem_00027",
                                "p.41-42",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_HW_SPECIFIC_SERVICE, MEM_E_PARAM_POINTER) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10017], [SWS_Mem_00027], p.41-42
 * - Kiểm tra Mem_HwSpecificService() từ chối length pointer NULL với MEM_E_PARAM_POINTER.
 */
static void JOB_009(void)
{
    Mem_DataType   dataBuffer[4] = { 0u, 0u, 0u, 0u };
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_HwSpecificService(TEST_FLASH_INSTANCE, 0u, dataBuffer, NULL_PTR);

    TestManager_RecordCaseTrace(0x0809u, "HwSpecificRejectsNullLengthPtr",
                                "SWS_Mem_10017,SWS_Mem_00027",
                                "p.41-42",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_HW_SPECIFIC_SERVICE, MEM_E_PARAM_POINTER) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10024], [SWS_Mem_00091], p.33
 * - Kiểm tra Mem_Suspend() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void JOB_010(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Suspend(TEST_FLASH_INVALID_INSTANCE);

    TestManager_RecordCaseTrace(0x0810u, "SuspendRejectsInvalidInstance",
                                "SWS_Mem_10024,SWS_Mem_00091",
                                "p.33",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_SUSPEND, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10025], [SWS_Mem_00092], p.34
 * - Kiểm tra Mem_Resume() từ chối instance ID không hợp lệ với MEM_E_PARAM_INSTANCE_ID.
 */
static void JOB_011(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Resume(TEST_FLASH_INVALID_INSTANCE);

    TestManager_RecordCaseTrace(0x0811u, "ResumeRejectsInvalidInstance",
                                "SWS_Mem_10025,SWS_Mem_00092",
                                "p.34",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_RESUME, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

void Job_Test(void)
{
    TestManager_BeginGroup(TEST_JOB);

    JOB_001();
    JOB_002();
    JOB_003();
    JOB_004();
    JOB_005();
    JOB_006();
    JOB_007();
    JOB_008();
    JOB_009();
    JOB_010();
    JOB_011();

    TestManager_EndGroup();
}
