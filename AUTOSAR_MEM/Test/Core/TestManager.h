/******************************************************************************
 * FILE: TestManager.h
 * MÔ TẢ: Test Manager AUTOSAR MEM Driver và hỗ trợ kiểm thử mức thanh ghi
 ******************************************************************************/

#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mem.h"
#include "det.h"
#include "Flash_IP_Cfg.h"

/* Compatibility names used by the tests. The flash_ip Driver exposes the
 * same service IDs with the *_ID naming convention. */
#define MEM_SID_INIT                 MEM_INIT_ID
#define MEM_SID_GET_VERSION_INFO     MEM_GETVERSIONINFO_ID
#define MEM_SID_GET_JOB_RESULT       MEM_GETJOBRESULT_ID
#define MEM_SID_READ                 MEM_READ_ID
#define MEM_SID_WRITE                MEM_WRITE_ID
#define MEM_SID_ERASE                MEM_ERASE_ID
#define MEM_SID_PROPAGATE_ERROR      MEM_PROPAGATEERROR_ID
#define MEM_SID_BLANK_CHECK          MEM_BLANKCHECK_ID
#define MEM_SID_HW_SPECIFIC_SERVICE  MEM_HWSPECIFICSERVICE_ID
#define MEM_SID_SUSPEND              MEM_SUSPEND_ID
#define MEM_SID_RESUME               MEM_RESUME_ID

#define FLASH_IP_BASE_ADDRESS        0x08000000UL
#define FLASH_IP_TOTAL_SIZE          0x00080000UL

/*===========================================================================*/
/* Nhóm test                                                                 */
/*===========================================================================*/

typedef enum
{
    TEST_NONE = 0,

    TEST_INIT,
    TEST_DET,
    TEST_STATUS,
    TEST_READ,
    TEST_WRITE,
    TEST_ERASE,
    TEST_BLANKCHECK,
    TEST_JOB,

    TEST_ALL_GROUPS

} TestGroupType;

/*===========================================================================*/
/* Báo cáo kết quả / fault                                                   */
/*===========================================================================*/

typedef enum
{
    TEST_CASE_NOT_RUN = 0u,
    TEST_CASE_PASS,
    TEST_CASE_FAIL
} TestCaseResultType;

typedef enum
{
    TEST_FAULT_NONE = 0u,
    TEST_FAULT_NMI,
    TEST_FAULT_HARDFAULT,
    TEST_FAULT_MEMMANAGE,
    TEST_FAULT_BUSFAULT,
    TEST_FAULT_USAGEFAULT
} TestFaultType;

#define TEST_MAX_GROUP_REPORTS     8u
#define TEST_MAX_CASE_REPORTS      96u
#define TEST_REPORT_INVALID_INDEX  0xFFFFFFFFUL

typedef struct
{
    uint32 GroupId;
    uint32 Total;
    uint32 Passed;
    uint32 Failed;
} TestGroupReportType;

typedef struct
{
    uint32 Sequence;
    uint32 GroupId;
    uint32 CaseId;
    uint32 Result;
    uint32 Expected;
    uint32 Actual;
    uint32 SourceLine;
    uint32 DetValid;
    uint32 DetModuleId;
    uint32 DetInstanceId;
    uint32 DetApiId;
    uint32 DetErrorId;
    const char* CaseName;
    const char* RequirementIds;
    const char* SpecPages;
    const char* SourceFile;
} TestCaseReportType;

typedef struct
{
    uint32 Signature;
    uint32 Completed;
    uint32 OverallTotal;
    uint32 OverallPassed;
    uint32 OverallFailed;
    uint32 ActiveGroup;
    uint32 LastFault;
    uint32 FaultCfsr;
    uint32 FaultHfsr;
    uint32 FaultMmfar;
    uint32 FaultBfar;
    uint32 GroupCount;
    uint32 CaseCount;
    uint32 FirstFailedCaseIndex;
    uint32 LastFailedCaseIndex;
    uint32 DroppedCaseCount;
    TestGroupReportType GroupReports[TEST_MAX_GROUP_REPORTS];
    TestCaseReportType  CaseReports[TEST_MAX_CASE_REPORTS];
} TestReportType;

extern volatile TestReportType g_TestReport;

/*===========================================================================*/
/* Select Active Test                                                        */
/*===========================================================================*/

/* Chỉnh macro này để chạy một nhóm test cụ thể hoặc TEST_ALL_GROUPS.
 * Macro này được dùng khi debugger đang kết nối và menu UART được bỏ qua. */
#define ACTIVE_TEST_GROUP             TEST_ALL_GROUPS

/*===========================================================================*/
/* Cấu hình test hướng phần cứng                                             */
/*===========================================================================*/

#define TEST_ENABLE_LED_OUTPUT        STD_ON
#define TEST_UART_BAUDRATE            115200u

/* UART luôn được biên dịch vào firmware. TestManager kiểm tra C_DEBUGEN lúc
 * khởi động: chỉ bỏ qua UART/menu khi debugger thực sự đang kết nối với MCU. */
#define TEST_ENABLE_UART_OUTPUT       STD_ON
#define TEST_ENABLE_UART_MENU         STD_ON

/* Dành riêng một Flash sector đầy đủ cho test phá hủy dữ liệu (write/erase/blank-check).
 * Mặc định: sector 7 trên STM32F401RE (0x08060000 - 0x0807FFFF). */
#define TEST_FLASH_INSTANCE           ((Mem_InstanceIdType)MEM_INDEX)
#define TEST_FLASH_INVALID_INSTANCE   ((Mem_InstanceIdType)MEM_MAX_INSTANCES)
#define TEST_FLASH_SECTOR_ADDRESS     ((Mem_AddressType)0x08060000UL)
#define TEST_FLASH_SECTOR_LENGTH      ((Mem_LengthType)0x00020000UL)
#define TEST_FLASH_WRITE_ADDRESS      ((Mem_AddressType)(TEST_FLASH_SECTOR_ADDRESS + 0x00000100UL))
#define TEST_FLASH_COMPARE_LENGTH     ((Mem_LengthType)16u)
#define TEST_FLASH_INVALID_ADDRESS    ((Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE))

/* Số lần polling Mem_MainFunction tối đa cho job bất đồng bộ đã được chấp nhận. */
#define TEST_MAINFUNCTION_TIMEOUT     32u

/*===========================================================================*/
/* API                                                                       */
/*===========================================================================*/

void TestManager_Run(void);
void TestManager_BeginGroup(TestGroupType group);
void TestManager_RecordCaseTraceAt(
        uint32 caseId,
        const char* caseName,
        const char* requirementIds,
        const char* specPages,
        boolean passed,
        uint32 expected,
        uint32 actual,
        const char* sourceFile,
        uint32 sourceLine);

#define TestManager_RecordCaseTrace(caseId, caseName, requirementIds, specPages, passed, expected, actual) \
    TestManager_RecordCaseTraceAt((caseId), (caseName), (requirementIds), (specPages), \
                                  (passed), (expected), (actual), __FILE__, (uint32)__LINE__)

void TestManager_RecordCase(uint32 caseId, const char* caseName, boolean passed, uint32 expected, uint32 actual);
void TestManager_EndGroup(void);
void TestManager_Finish(void);
void TestManager_ReportFault(TestFaultType fault);

void TestManager_PrepareDriver(void);
void TestManager_ShutdownDriver(void);

void TestManager_ResetDetState(void);
boolean TestManager_HasDetError(void);
boolean TestManager_CheckDet(uint8 apiId, uint8 errorId);

MemAcc_MemJobResultType TestManager_ExecuteMainUntilDone(Mem_InstanceIdType instanceId, uint32 timeoutTicks);

boolean TestManager_IsFlashTestAreaSafe(void);
boolean TestManager_EraseTestSectorAndWait(void);
void TestManager_FillPattern(Mem_DataType* buffer, Mem_LengthType length, uint8 seed);
boolean TestManager_BufferEquals(const Mem_DataType* lhs, const Mem_DataType* rhs, Mem_LengthType length);
void TestManager_CopyFromFlash(Mem_AddressType sourceAddress, Mem_DataType* dest, Mem_LengthType length);
boolean TestManager_IsFlashRangeErased(Mem_AddressType address, Mem_LengthType length);

#ifdef __cplusplus
}
#endif

#endif
