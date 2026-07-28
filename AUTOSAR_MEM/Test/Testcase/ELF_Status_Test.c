/******************************************************************************
 * FILE: ELF_Status_Test.c
 * DESCRIPTION: Job-status tests for the AUTOSAR Mem driver
 ******************************************************************************/

#include "ELF_Status_Test.h"
#include "TestManager.h"

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10011], [SWS_Mem_00029], [SWS_Mem_00001], p.15,30,32
 * - Verifies Mem_GetJobResult() reports MEM_JOB_OK after driver initialization.
 */
static void ELF_STATUS_001(void)
{
    MemAcc_MemJobResultType jobResult;

    TestManager_PrepareDriver();
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0301u, "InitialJobResultOk",
                                "SWS_Mem_10011,SWS_Mem_00029,SWS_Mem_00001",
                                "p.15,30,32",
                                (boolean)(jobResult == MEM_JOB_OK),
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00030], [SWS_Mem_00029], p.15,36
 * - Verifies an accepted asynchronous read request changes the job result to MEM_JOB_PENDING.
 */
static void ELF_STATUS_002(void)
{
    Mem_DataType            buffer[8];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0302u, "AcceptedReadSetsPending",
                                "SWS_Mem_10012,SWS_Mem_00030,SWS_Mem_00029",
                                "p.15,36",
                                (boolean)((retVal == E_OK) && (jobResult == MEM_JOB_PENDING)),
                                (uint32)MEM_JOB_PENDING,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00066], [SWS_Mem_00067], p.15,36
 * - Verifies Mem_MainFunction() completes an accepted read request and reports MEM_JOB_OK.
 */
static void ELF_STATUS_003(void)
{
    Mem_DataType            buffer[8];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;

    TestManager_PrepareDriver();
    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));
    jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

    TestManager_RecordCaseTrace(0x0303u, "ReadCompletesWithOk",
                                "SWS_Mem_10012,SWS_Mem_00066,SWS_Mem_00067",
                                "p.15,36",
                                (boolean)((retVal == E_OK) && (jobResult == MEM_JOB_OK)),
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10011], [SWS_Mem_00090], p.32
 * - Verifies Mem_GetJobResult() rejects an invalid instance ID with MEM_E_PARAM_INSTANCE_ID.
 */
static void ELF_STATUS_004(void)
{
    MemAcc_MemJobResultType jobResult;
    boolean                 passed;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    jobResult = Mem_GetJobResult(TEST_FLASH_INVALID_INSTANCE);
    passed = (boolean)((jobResult == MEM_JOB_FAILED) &&
                       (TestManager_CheckDet(MEM_SID_GET_JOB_RESULT, MEM_E_PARAM_INSTANCE_ID) == TRUE));

    TestManager_RecordCaseTrace(0x0304u, "GetJobResultInvalidInstance",
                                "SWS_Mem_10011,SWS_Mem_00090",
                                "p.32",
                                passed,
                                (uint32)MEM_JOB_FAILED,
                                (uint32)jobResult);
}

/* Traceability:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_00059], [SWS_Mem_00072], [SWS_Mem_00029], [SWS_Mem_10012], p.15,32,36
 * - Verifies a synchronously rejected request returns E_NOT_OK and does not move the stored job result away from MEM_JOB_OK.
 */
static void ELF_STATUS_005(void)
{
    Mem_DataType            buffer[4];
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;
    boolean                 passed;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)0u);
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);
    passed = (boolean)((retVal == E_NOT_OK) &&
                       (jobResult == MEM_JOB_OK) &&
                       (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_LENGTH) == TRUE));

    TestManager_RecordCaseTrace(0x0305u, "RejectedReadKeepsJobResultOk",
                                "SWS_Mem_00059,SWS_Mem_00072,SWS_Mem_00029,SWS_Mem_10012",
                                "p.15,32,36",
                                passed,
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

void ELF_Status_Test(void)
{
    TestManager_BeginGroup(TEST_ELF_STATUS);

    ELF_STATUS_001();
    ELF_STATUS_002();
    ELF_STATUS_003();
    ELF_STATUS_004();
    ELF_STATUS_005();

    TestManager_EndGroup();
}
