/******************************************************************************
 * FILE: Init_Test.c
 * MÔ TẢ: Test khởi tạo cho AUTOSAR Mem driver
 ******************************************************************************/

#include "Init_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10008], [SWS_Mem_00001], p.29-30
 * - Kiểm tra Mem_Init() khởi tạo kết quả job là MEM_JOB_OK.
 */
static void INIT_001(void)
{
    MemAcc_MemJobResultType jobResult;

    TestManager_PrepareDriver();
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);

    TestManager_RecordCaseTrace(0x0101u, "JobResultAfterInit",
                                "SWS_Mem_10008,SWS_Mem_00001",
                                "p.29-30",
                                (boolean)(jobResult == MEM_JOB_OK),
                                (uint32)MEM_JOB_OK,
                                (uint32)jobResult);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10009], p.31
 * - Kiểm tra Mem_GetVersionInfo() trả về thông tin nhận dạng module và phiên bản phần mềm.
 */
static void INIT_002(void)
{
#if (MEM_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType versionInfo = { 0u, 0u, 0u, 0u, 0u };
    boolean             passed;

    TestManager_PrepareDriver();
    Mem_GetVersionInfo(&versionInfo);

    passed = (boolean)((versionInfo.vendorID         == MEM_VENDOR_ID) &&
                       (versionInfo.moduleID         == MEM_MODULE_ID) &&
                       (versionInfo.sw_major_version == MEM_SW_MAJOR_VERSION) &&
                       (versionInfo.sw_minor_version == MEM_SW_MINOR_VERSION) &&
                       (versionInfo.sw_patch_version == MEM_SW_PATCH_VERSION));

    TestManager_RecordCaseTrace(0x0102u, "VersionInfoMatches",
                                "SWS_Mem_10009",
                                "p.31",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
#else
    TestManager_RecordCaseTrace(0x0102u, "VersionInfoDisabled",
                                "SWS_Mem_10009",
                                "p.31",
                                TRUE,
                                0u,
                                0u);
#endif
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10008], [SWS_Mem_00087], p.29-30
 * - Kiểm tra Mem_Init() báo MEM_E_PARAM_POINTER khi configPtr khác NULL.
 */
static void INIT_003(void)
{
    MemAcc_MemJobResultType jobResult;
    boolean                 passed;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    Mem_Init((const Mem_ConfigType*)1UL);
    jobResult = Mem_GetJobResult(TEST_FLASH_INSTANCE);
    passed = (boolean)((TestManager_CheckDet(MEM_SID_INIT, MEM_E_PARAM_POINTER) == TRUE) &&
                       (jobResult == MEM_JOB_OK));

    TestManager_RecordCaseTrace(0x0103u, "InitRejectsNonNullConfig",
                                "SWS_Mem_10008,SWS_Mem_00087",
                                "p.29-30",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10009], [SWS_Mem_00002], p.31
 * - Kiểm tra Mem_GetVersionInfo() báo MEM_E_PARAM_POINTER khi versionInfoPtr là NULL.
 */
static void INIT_004(void)
{
#if (MEM_VERSION_INFO_API == STD_ON)
    boolean passed;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    Mem_GetVersionInfo(NULL_PTR);
    passed = (boolean)(TestManager_CheckDet(MEM_SID_GET_VERSION_INFO, MEM_E_PARAM_POINTER) == TRUE);

    TestManager_RecordCaseTrace(0x0104u, "VersionInfoRejectsNullPointer",
                                "SWS_Mem_10009,SWS_Mem_00002",
                                "p.31",
                                passed,
                                1u,
                                (passed == TRUE) ? 1u : 0u);
#else
    TestManager_RecordCaseTrace(0x0104u, "VersionInfoDisabledNullPointer",
                                "SWS_Mem_10009,SWS_Mem_00002",
                                "p.31",
                                TRUE,
                                0u,
                                0u);
#endif
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10018], [SWS_Mem_00079], section 7.3.1, [SWS_Mem_00052],
 *   p.23,25,30-31
 * - Kiểm tra Mem_DeInit() hủy khởi tạo trạng thái module để lời gọi API tiếp theo bị từ chối do chưa khởi tạo.
 */
static void INIT_005(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;
    boolean        passed;

    TestManager_PrepareDriver();
    Mem_DeInit();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)sizeof(buffer));

    passed = (boolean)((retVal == E_NOT_OK) &&
                       (TestManager_CheckDet(MEM_SID_READ, MEM_E_UNINIT) == TRUE));

    TestManager_RecordCaseTrace(0x0105u, "DeInitTransitionsToUninit",
                                "SWS_Mem_10018,SWS_Mem_00079,Section7.3.1,SWS_Mem_00052",
                                "p.23,25,30-31",
                                passed,
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

void Init_Test(void)
{
    TestManager_BeginGroup(TEST_INIT);

    INIT_001();
    INIT_002();
    INIT_003();
    INIT_004();
    INIT_005();

    TestManager_EndGroup();
}
