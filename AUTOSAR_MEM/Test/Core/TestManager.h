/******************************************************************************
 * FILE: TestManager.h
 * DESCRIPTION: AUTOSAR MEM Driver Test Manager and register-level test support
 ******************************************************************************/

#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mem.h"
#include "det.h"
#include "Flash_IP_Cfg.h"

/*===========================================================================*/
/* Test Group                                                                */
/*===========================================================================*/

typedef enum
{
    TEST_NONE = 0,

    TEST_ELF_INIT,
    TEST_ELF_DET,
    TEST_ELF_STATUS,
    TEST_ELF_READ,
    TEST_ELF_WRITE,
    TEST_ELF_ERASE,
    TEST_ELF_BLANKCHECK,
    TEST_ELF_JOB,

    TEST_ALL_GROUPS

} TestGroupType;

/*===========================================================================*/
/* Result / Fault Report                                                     */
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

typedef struct
{
    uint32 GroupId;
    uint32 Total;
    uint32 Passed;
    uint32 Failed;
} TestGroupReportType;

typedef struct
{
    uint32 GroupId;
    uint32 CaseId;
    uint32 Result;
    uint32 Expected;
    uint32 Actual;
    const char* CaseName;
    const char* RequirementIds;
    const char* SpecPages;
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
    uint32 GroupCount;
    uint32 CaseCount;
    TestGroupReportType GroupReports[TEST_MAX_GROUP_REPORTS];
    TestCaseReportType  CaseReports[TEST_MAX_CASE_REPORTS];
} TestReportType;

extern volatile TestReportType g_TestReport;

/*===========================================================================*/
/* Select Active Test                                                        */
/*===========================================================================*/

/* Chinh macro nay de chay 1 nhom test cu the hoac TEST_ALL_GROUPS.
 * Macro nay chi duoc su dung khi TEST_ENABLE_UART_MENU == STD_OFF. */
#define ACTIVE_TEST_GROUP             TEST_ALL_GROUPS

/*===========================================================================*/
/* Hardware-oriented test configuration                                      */
/*===========================================================================*/

#define TEST_ENABLE_UART_OUTPUT       STD_ON
#define TEST_ENABLE_LED_OUTPUT        STD_ON
#define TEST_ENABLE_UART_MENU         STD_ON
#define TEST_UART_BAUDRATE            115200u

/* Reserve one full flash sector for destructive tests (write/erase/blank-check).
 * Default: sector 7 on STM32F401RE (0x08060000 - 0x0807FFFF). */
#define TEST_FLASH_INSTANCE           MemConf_MemInstance_MemInstance_0
#define TEST_FLASH_INVALID_INSTANCE   ((Mem_InstanceIdType)MEM_INSTANCE_COUNT)
#define TEST_FLASH_SECTOR_ADDRESS     ((Mem_AddressType)0x08060000UL)
#define TEST_FLASH_SECTOR_LENGTH      ((Mem_LengthType)0x00020000UL)
#define TEST_FLASH_WRITE_ADDRESS      ((Mem_AddressType)(TEST_FLASH_SECTOR_ADDRESS + 0x00000100UL))
#define TEST_FLASH_COMPARE_LENGTH     ((Mem_LengthType)16u)
#define TEST_FLASH_INVALID_ADDRESS    ((Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE))

/* Mem_MainFunction polling budget for accepted asynchronous jobs. */
#define TEST_MAINFUNCTION_TIMEOUT     8u

/*===========================================================================*/
/* API                                                                       */
/*===========================================================================*/

void TestManager_Run(void);
void TestManager_BeginGroup(TestGroupType group);
void TestManager_RecordCaseTrace(
        uint32 caseId,
        const char* caseName,
        const char* requirementIds,
        const char* specPages,
        boolean passed,
        uint32 expected,
        uint32 actual);
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
