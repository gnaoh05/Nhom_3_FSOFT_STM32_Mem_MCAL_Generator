#include <Stm32F401_BareMetal.h>
#include "TestManager.h"

#include "Init_Test.h"
#include "Det_Test.h"
#include "Status_Test.h"
#include "Read_Test.h"
#include "Write_Test.h"
#include "Erase_Test.h"
#include "BlankCheck_Test.h"
#include "Job_Test.h"
#include "Boundary_Test.h"
#include "Manual_Test.h"

#define TEST_REPORT_SIGNATURE       0x54455354UL
#define TEST_INVALID_INDEX          0xFFFFFFFFUL
#define TEST_LED_PIN_MASK           (1UL << 5u)
#define TEST_SCB_CFSR               (*(volatile uint32*)0xE000ED28UL)
#define TEST_SCB_HFSR               (*(volatile uint32*)0xE000ED2CUL)
#define TEST_SCB_MMFAR              (*(volatile uint32*)0xE000ED34UL)
#define TEST_SCB_BFAR               (*(volatile uint32*)0xE000ED38UL)

extern uint32 _sidata;
extern uint32 _sdata;
extern uint32 _edata;

volatile TestReportType g_TestReport;

TestSectorConfigType g_CurrentTestSector = {
    7u,
    0x08060000UL,
    0x00020000UL
};

static const TestSectorConfigType g_SafeTestSectors[] = {
    { 3u, 0x0800C000UL, 0x00004000UL }, /* Sector 3 - 16 KB  */
    { 4u, 0x08010000UL, 0x00010000UL }, /* Sector 4 - 64 KB  */
    { 5u, 0x08020000UL, 0x00020000UL }, /* Sector 5 - 128 KB */
    { 6u, 0x08040000UL, 0x00020000UL }, /* Sector 6 - 128 KB */
    { 7u, 0x08060000UL, 0x00020000UL }  /* Sector 7 - 128 KB (Mặc định) */
};
#define TEST_SAFE_SECTOR_COUNT (sizeof(g_SafeTestSectors) / sizeof(g_SafeTestSectors[0]))

static uint32  TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
static boolean TestManager_UartReady = FALSE;

/*===========================================================================*/
/* 1. SECTOR MANAGEMENT                                                      */
/*===========================================================================*/

boolean TestManager_SetTargetSector(uint8 sectorId)
{
    uint32 i;
    for (i = 0u; i < TEST_SAFE_SECTOR_COUNT; i++)
    {
        if (g_SafeTestSectors[i].SectorId == sectorId)
        {
            g_CurrentTestSector = g_SafeTestSectors[i];
            return TRUE;
        }
    }
    return FALSE;
}

uint8 TestManager_GetTargetSectorId(void)
{
    return g_CurrentTestSector.SectorId;
}

TestNeighborBoundsType TestManager_GetNeighborBounds(void)
{
    TestNeighborBoundsType bounds;
    bounds.HasPrev = FALSE;
    bounds.PrevTailAddress = 0u;
    bounds.HasNext = FALSE;
    bounds.NextHeadAddress = 0u;

    switch (g_CurrentTestSector.SectorId)
    {
        case 3u:
            bounds.HasPrev = TRUE;
            bounds.PrevTailAddress = 0x0800BFF0UL; /* 16 byte cuối Sector 2 */
            bounds.HasNext = TRUE;
            bounds.NextHeadAddress = 0x08010000UL; /* Đầu Sector 4 */
            break;
        case 4u:
            bounds.HasPrev = TRUE;
            bounds.PrevTailAddress = 0x0800FFF0UL; /* 16 byte cuối Sector 3 */
            bounds.HasNext = TRUE;
            bounds.NextHeadAddress = 0x08020000UL; /* Đầu Sector 5 */
            break;
        case 5u:
            bounds.HasPrev = TRUE;
            bounds.PrevTailAddress = 0x0801FFF0UL; /* 16 byte cuối Sector 4 */
            bounds.HasNext = TRUE;
            bounds.NextHeadAddress = 0x08040000UL; /* Đầu Sector 6 */
            break;
        case 6u:
            bounds.HasPrev = TRUE;
            bounds.PrevTailAddress = 0x0803FFF0UL; /* 16 byte cuối Sector 5 */
            bounds.HasNext = TRUE;
            bounds.NextHeadAddress = 0x08060000UL; /* Đầu Sector 7 */
            break;
        case 7u:
            bounds.HasPrev = TRUE;
            bounds.PrevTailAddress = 0x0805FFF0UL; /* 16 byte cuối Sector 6 */
            bounds.HasNext = FALSE;
            break;
        default:
            break;
    }

    return bounds;
}

/*===========================================================================*/
/* 2. PLATFORM & PERIPHERAL DRIVER (LED & USART2)                            */
/*===========================================================================*/

static void TestManager_LedInit(void)
{
    STM32_RCC->AHB1ENR |= STM32_RCC_AHB1ENR_GPIOAEN;
    STM32_GPIOA->MODER &= ~(0x3UL << (5u * 2u));
    STM32_GPIOA->MODER |=  (0x1UL << (5u * 2u));
    STM32_GPIOA->OTYPER &= ~(1UL << 5u);
    STM32_GPIOA->OSPEEDR &= ~(0x3UL << (5u * 2u));
    STM32_GPIOA->PUPDR &= ~(0x3UL << (5u * 2u));
    STM32_GPIOA->BSRR = (1UL << (5u + 16u));
}

static void TestManager_LedOn(void)
{
    STM32_GPIOA->BSRR = (1UL << 5u);
}

static void TestManager_LedOff(void)
{
    STM32_GPIOA->BSRR = (1UL << (5u + 16u));
}

static void TestManager_UartInit(void)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    uint32 uartDivider;

    STM32_RCC->AHB1ENR |= STM32_RCC_AHB1ENR_GPIOAEN;
    STM32_RCC->APB1ENR |= STM32_RCC_APB1ENR_USART2EN;

    STM32_GPIOA->MODER &= ~((0x3UL << (2u * 2u)) | (0x3UL << (3u * 2u)));
    STM32_GPIOA->MODER |=  ((0x2UL << (2u * 2u)) | (0x2UL << (3u * 2u)));
    STM32_GPIOA->OTYPER &= ~((1UL << 2u) | (1UL << 3u));
    STM32_GPIOA->OSPEEDR |= ((0x3UL << (2u * 2u)) | (0x3UL << (3u * 2u)));
    STM32_GPIOA->PUPDR &= ~((0x3UL << (2u * 2u)) | (0x3UL << (3u * 2u)));
    STM32_GPIOA->AFR[0] &= ~((0xFUL << (2u * 4u)) | (0xFUL << (3u * 4u)));
    STM32_GPIOA->AFR[0] |=  ((0x7UL << (2u * 4u)) | (0x7UL << (3u * 4u)));

    Stm32_BareMetalSystemCoreClockUpdate();
    uartDivider = (Stm32_SystemCoreClock + (TEST_UART_BAUDRATE / 2u)) / TEST_UART_BAUDRATE;
    if (uartDivider == 0u)
    {
        uartDivider = 1u;
    }

    STM32_USART2->CR1 = 0u;
    STM32_USART2->BRR = uartDivider;
    STM32_USART2->CR2 = 0u;
    STM32_USART2->CR3 = 0u;
    STM32_USART2->CR1 = STM32_USART_CR1_TE | STM32_USART_CR1_RE | STM32_USART_CR1_UE;

    TestManager_UartReady = TRUE;
#else
    TestManager_UartReady = FALSE;
#endif
}

void TestManager_UartWriteChar(char c)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_UartReady == TRUE)
    {
        if (c == '\n')
        {
            while ((STM32_USART2->SR & STM32_USART_SR_TXE) == 0u)
            {
            }
            STM32_USART2->DR = (uint32)((uint8)'\r');
        }

        while ((STM32_USART2->SR & STM32_USART_SR_TXE) == 0u)
        {
        }
        STM32_USART2->DR = (uint32)((uint8)c);
    }
#else
    (void)c;
#endif
}

static void TestManager_UartWriteString(const char* str)
{
    if (str != NULL_PTR)
    {
        while (*str != '\0')
        {
            TestManager_UartWriteChar(*str);
            str++;
        }
    }
}

char TestManager_UartReadCharBlocking(void)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_UartReady == TRUE)
    {
        while ((STM32_USART2->SR & STM32_USART_SR_RXNE) == 0u)
        {
        }
        return (char)(STM32_USART2->DR & 0xFFu);
    }
#endif
    return '\0';
}

static void TestManager_UartFlushRx(void)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_UartReady == TRUE)
    {
        while ((STM32_USART2->SR & STM32_USART_SR_RXNE) != 0u)
        {
            (void)STM32_USART2->DR;
        }
    }
#endif
}

static void TestManager_PlatformInit(void)
{
#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
    TestManager_LedInit();
#endif
    TestManager_UartInit();
}

boolean TestManager_IsDebuggerConnected(void)
{
    return ((STM32_CORE_DEBUG_DHCSR & STM32_CORE_DEBUG_DHCSR_C_DEBUGEN) != 0UL) ? TRUE : FALSE;
}

/*===========================================================================*/
/* 3. DET ERROR STATE MANAGEMENT                                             */
/*===========================================================================*/

void TestManager_ResetDetState(void)
{
    Det_ResetLastError();
}

boolean TestManager_HasDetError(void)
{
    return Det_HasError();
}

boolean TestManager_CheckDet(uint8 apiId, uint8 errorId)
{
    Det_LastErrorType lastError = Det_GetLastError();
    return ((lastError.Valid == TRUE) && (lastError.ApiId == apiId) && (lastError.ErrorId == errorId)) ? TRUE : FALSE;
}

/*===========================================================================*/
/* 4. STRING & FORMATTING LOGGERS                                            */
/*===========================================================================*/

static char TestManager_ToUpper(char c)
{
    if ((c >= 'a') && (c <= 'z'))
    {
        return (char)(c - ('a' - 'A'));
    }
    return c;
}

static const char* TestManager_GetGroupName(TestGroupType group)
{
    switch (group)
    {
        case TEST_INIT:       return "INIT";
        case TEST_DET:        return "DET";
        case TEST_STATUS:     return "STATUS";
        case TEST_READ:       return "READ";
        case TEST_WRITE:      return "WRITE";
        case TEST_ERASE:      return "ERASE";
        case TEST_BLANKCHECK: return "BLANKCHECK";
        case TEST_JOB:        return "JOB";
        case TEST_BOUNDARY:   return "BOUNDARY";
        case TEST_ALL_GROUPS: return "ALL";
        case TEST_MANUAL_CLI: return "MANUAL_CLI";
        default:              return "NONE";
    }
}

void TestManager_LogString(const char* str)
{
    TestManager_UartWriteString(str);
}

void TestManager_LogUnsigned(uint32 value)
{
    char buf[12];
    uint32 i = 0u;

    if (value == 0u)
    {
        TestManager_UartWriteChar('0');
        return;
    }

    while (value > 0u)
    {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (i > 0u)
    {
        TestManager_UartWriteChar(buf[--i]);
    }
}

void TestManager_LogHex8(uint8 value)
{
    const char hexChars[] = "0123456789ABCDEF";

    TestManager_UartWriteString("0x");
    TestManager_UartWriteChar(hexChars[(value >> 4u) & 0x0Fu]);
    TestManager_UartWriteChar(hexChars[value & 0x0Fu]);
}

void TestManager_LogHex32(uint32 value)
{
    const char hexChars[] = "0123456789ABCDEF";
    int shift;

    TestManager_UartWriteString("0x");
    for (shift = 28; shift >= 0; shift -= 4)
    {
        TestManager_UartWriteChar(hexChars[(value >> (uint32)shift) & 0x0Fu]);
    }
}

static void TestManager_LogCaseId(uint32 caseId)
{
    const char hexChars[] = "0123456789ABCDEF";

    if (caseId <= 0xFFFu)
    {
        /* Clean 3-digit format: 101, 102, 501, 810, 905 */
        TestManager_UartWriteChar(hexChars[(caseId >> 8u) & 0x0Fu]);
        TestManager_UartWriteChar(hexChars[(caseId >> 4u) & 0x0Fu]);
        TestManager_UartWriteChar(hexChars[caseId & 0x0Fu]);
    }
    else
    {
        TestManager_LogUnsigned(caseId);
    }
}

static void TestManager_LogRunBanner(void)
{
    TestManager_LogString("\n========================================\n");
    TestManager_LogString("AUTOSAR MEM Test Console Ready\n");
    TestManager_LogString("Target: STM32F401RE Flash MCAL Driver\n");
    TestManager_LogString("========================================\n");
}

static void TestManager_LogCaseResult(const TestCaseReportType* caseReport, const Det_LastErrorType* detSnapshot)
{
    TestManager_LogString("  [");
    TestManager_LogString((caseReport->Result == TEST_CASE_PASS) ? "PASS" : "FAIL");
    TestManager_LogString("] ");
    TestManager_LogCaseId(caseReport->CaseId);
    TestManager_LogString(" ");
    TestManager_LogString(caseReport->CaseName);
    TestManager_LogString(" (req=");
    TestManager_LogString(caseReport->RequirementIds);
    TestManager_LogString(")");

    if (caseReport->Result == TEST_CASE_FAIL)
    {
        TestManager_LogString(" expected=");
        TestManager_LogHex32(caseReport->Expected);
        TestManager_LogString(" actual=");
        TestManager_LogHex32(caseReport->Actual);
        if (caseReport->SourceFile != NULL_PTR)
        {
            TestManager_LogString(" at=");
            TestManager_LogString(caseReport->SourceFile);
            TestManager_LogString(":");
            TestManager_LogUnsigned(caseReport->SourceLine);
        }
    }

    if (detSnapshot->Valid == TRUE)
    {
        TestManager_LogString(" (det: api=");
        TestManager_LogHex8(detSnapshot->ApiId);
        TestManager_LogString(", error=");
        TestManager_LogHex8(detSnapshot->ErrorId);
        TestManager_LogString(")");
    }
    TestManager_LogString("\n");
}

static void TestManager_LogGroupSummary(const TestGroupReportType* summary)
{
    TestManager_LogString("[GROUP SUMMARY] ");
    TestManager_LogString(TestManager_GetGroupName((TestGroupType)summary->GroupId));
    TestManager_LogString(" -> total=");
    TestManager_LogUnsigned(summary->Total);
    TestManager_LogString(" passed=");
    TestManager_LogUnsigned(summary->Passed);
    TestManager_LogString(" failed=");
    TestManager_LogUnsigned(summary->Failed);
    TestManager_LogString("\n");
}

/*===========================================================================*/
/* 5. REPORT & RECORDING MANAGEMENT (GIỮ NGUYÊN CẤU TRÚC BÁO CÁO)           */
/*===========================================================================*/

static void TestManager_ResetReport(void)
{
    uint32 i;

    g_TestReport.Signature = TEST_REPORT_SIGNATURE;
    g_TestReport.Completed = FALSE;
    g_TestReport.OverallTotal = 0u;
    g_TestReport.OverallPassed = 0u;
    g_TestReport.OverallFailed = 0u;
    g_TestReport.ActiveGroup = (uint32)TEST_NONE;
    g_TestReport.LastFault = (uint32)TEST_FAULT_NONE;
    g_TestReport.FaultCfsr = 0u;
    g_TestReport.FaultHfsr = 0u;
    g_TestReport.FaultMmfar = 0u;
    g_TestReport.FaultBfar = 0u;
    g_TestReport.GroupCount = 0u;
    g_TestReport.CaseCount = 0u;
    g_TestReport.FirstFailedCaseIndex = TEST_INVALID_INDEX;
    g_TestReport.LastFailedCaseIndex = TEST_INVALID_INDEX;
    g_TestReport.DroppedCaseCount = 0u;

    for (i = 0u; i < TEST_MAX_GROUP_REPORTS; i++)
    {
        g_TestReport.GroupReports[i].GroupId = (uint32)TEST_NONE;
        g_TestReport.GroupReports[i].Total = 0u;
        g_TestReport.GroupReports[i].Passed = 0u;
        g_TestReport.GroupReports[i].Failed = 0u;
    }

    for (i = 0u; i < TEST_MAX_CASE_REPORTS; i++)
    {
        g_TestReport.CaseReports[i].Sequence = 0u;
        g_TestReport.CaseReports[i].GroupId = (uint32)TEST_NONE;
        g_TestReport.CaseReports[i].CaseId = 0u;
        g_TestReport.CaseReports[i].Result = (uint32)TEST_CASE_NOT_RUN;
        g_TestReport.CaseReports[i].Expected = 0u;
        g_TestReport.CaseReports[i].Actual = 0u;
        g_TestReport.CaseReports[i].SourceLine = 0u;
        g_TestReport.CaseReports[i].DetValid = 0u;
        g_TestReport.CaseReports[i].DetModuleId = 0u;
        g_TestReport.CaseReports[i].DetInstanceId = 0u;
        g_TestReport.CaseReports[i].DetApiId = 0u;
        g_TestReport.CaseReports[i].DetErrorId = 0u;
        g_TestReport.CaseReports[i].CaseName = "";
        g_TestReport.CaseReports[i].RequirementIds = "";
        g_TestReport.CaseReports[i].SpecPages = "";
        g_TestReport.CaseReports[i].SourceFile = "";
    }

    TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
    TestManager_ResetDetState();
}

void TestManager_BeginGroup(TestGroupType group)
{
    uint32 i;

    g_TestReport.ActiveGroup = (uint32)group;
    TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;

    for (i = 0u; i < g_TestReport.GroupCount; i++)
    {
        if (g_TestReport.GroupReports[i].GroupId == (uint32)group)
        {
            TestManager_CurrentGroupIndex = i;
            break;
        }
    }

    if ((TestManager_CurrentGroupIndex == TEST_INVALID_INDEX) && (g_TestReport.GroupCount < TEST_MAX_GROUP_REPORTS))
    {
        TestManager_CurrentGroupIndex = g_TestReport.GroupCount;
        g_TestReport.GroupReports[TestManager_CurrentGroupIndex].GroupId = (uint32)group;
        g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Total = 0u;
        g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Passed = 0u;
        g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Failed = 0u;
        g_TestReport.GroupCount++;
    }

    TestManager_LogString("\n[GROUP] ");
    TestManager_LogString(TestManager_GetGroupName(group));
    TestManager_LogString("\n");
}

void TestManager_EndGroup(void)
{
    if (TestManager_CurrentGroupIndex != TEST_INVALID_INDEX)
    {
        TestManager_LogGroupSummary((const TestGroupReportType*)&g_TestReport.GroupReports[TestManager_CurrentGroupIndex]);
    }
    g_TestReport.ActiveGroup = (uint32)TEST_NONE;
    TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
}

void TestManager_RecordCaseTraceAt(
    uint32 caseId,
    const char* caseName,
    const char* requirementIds,
    const char* specPages,
    boolean passed,
    uint32 expected,
    uint32 actual,
    const char* sourceFile,
    uint32 sourceLine)
{
    TestCaseReportType snapshot;
    Det_LastErrorType  detSnapshot;
    uint32             storedIndex;

    detSnapshot = Det_GetLastError();

    snapshot.Sequence = g_TestReport.OverallTotal + 1u;
    snapshot.GroupId = g_TestReport.ActiveGroup;
    snapshot.CaseId = caseId;
    snapshot.Result = (passed == TRUE) ? (uint32)TEST_CASE_PASS : (uint32)TEST_CASE_FAIL;
    snapshot.Expected = expected;
    snapshot.Actual = actual;
    snapshot.SourceLine = sourceLine;
    snapshot.DetValid = (uint32)detSnapshot.Valid;
    snapshot.DetModuleId = (uint32)detSnapshot.ModuleId;
    snapshot.DetInstanceId = (uint32)detSnapshot.InstanceId;
    snapshot.DetApiId = (uint32)detSnapshot.ApiId;
    snapshot.DetErrorId = (uint32)detSnapshot.ErrorId;
    snapshot.CaseName = (caseName != NULL_PTR) ? caseName : "";
    snapshot.RequirementIds = (requirementIds != NULL_PTR) ? requirementIds : "";
    snapshot.SpecPages = (specPages != NULL_PTR) ? specPages : "";
    snapshot.SourceFile = (sourceFile != NULL_PTR) ? sourceFile : "";

    g_TestReport.OverallTotal++;
    if (passed == TRUE)
    {
        g_TestReport.OverallPassed++;
    }
    else
    {
        g_TestReport.OverallFailed++;
    }

    if (TestManager_CurrentGroupIndex != TEST_INVALID_INDEX)
    {
        g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Total++;
        if (passed == TRUE)
        {
            g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Passed++;
        }
        else
        {
            g_TestReport.GroupReports[TestManager_CurrentGroupIndex].Failed++;
        }
    }

    if (g_TestReport.CaseCount < TEST_MAX_CASE_REPORTS)
    {
        storedIndex = g_TestReport.CaseCount;
        g_TestReport.CaseReports[storedIndex] = snapshot;
        g_TestReport.CaseCount++;

        if (passed == FALSE)
        {
            if (g_TestReport.FirstFailedCaseIndex == TEST_INVALID_INDEX)
            {
                g_TestReport.FirstFailedCaseIndex = storedIndex;
            }
            g_TestReport.LastFailedCaseIndex = storedIndex;
        }
    }
    else
    {
        g_TestReport.DroppedCaseCount++;
    }

    TestManager_LogCaseResult(&snapshot, &detSnapshot);
    TestManager_ResetDetState();
}

void TestManager_RecordCase(uint32 caseId, const char* caseName, boolean passed, uint32 expected, uint32 actual)
{
    TestManager_RecordCaseTraceAt(caseId, caseName, "", "", passed, expected, actual, NULL_PTR, 0u);
}

void TestManager_ReportFault(TestFaultType fault)
{
    g_TestReport.LastFault = (uint32)fault;
    g_TestReport.FaultCfsr = TEST_SCB_CFSR;
    g_TestReport.FaultHfsr = TEST_SCB_HFSR;
    g_TestReport.FaultMmfar = TEST_SCB_MMFAR;
    g_TestReport.FaultBfar = TEST_SCB_BFAR;

    TestManager_LogString("[FAULT] fault=");
    TestManager_LogUnsigned((uint32)fault);
    TestManager_LogString(" cfsr=");
    TestManager_LogHex32(g_TestReport.FaultCfsr);
    TestManager_LogString(" hfsr=");
    TestManager_LogHex32(g_TestReport.FaultHfsr);
    TestManager_LogString("\n");

#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
    TestManager_LedOff();
#endif
}

/*===========================================================================*/
/* 6. DRIVER & HARDWARE HELPERS                                              */
/*===========================================================================*/

void TestManager_PrepareDriver(void)
{
    TestManager_ResetDetState();
    Mem_Init(NULL_PTR);
}

void TestManager_ShutdownDriver(void)
{
    Mem_DeInit();
    TestManager_ResetDetState();
}

MemAcc_MemJobResultType TestManager_ExecuteMainUntilDone(Mem_InstanceIdType instanceId, uint32 timeoutTicks)
{
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    uint32 ticks;

    for (ticks = 0u; ticks < timeoutTicks; ticks++)
    {
        Mem_MainFunction();
        jobResult = Mem_GetJobResult(instanceId);
        if (jobResult != MEM_JOB_PENDING)
        {
            break;
        }
    }

    return jobResult;
}

boolean TestManager_IsFlashTestAreaSafe(void)
{
    uint32 codeEndFlashAddress = (uint32)&_sidata + ((uint32)&_edata - (uint32)&_sdata);
    return (g_CurrentTestSector.SectorAddress >= codeEndFlashAddress) ? TRUE : FALSE;
}

boolean TestManager_EraseTestSectorAndWait(void)
{
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult;

    if (TestManager_IsFlashTestAreaSafe() == FALSE)
    {
        return FALSE;
    }

    retVal = Mem_Erase(TEST_FLASH_INSTANCE, g_CurrentTestSector.SectorAddress, g_CurrentTestSector.SectorLength);
    if (retVal != E_OK)
    {
        return FALSE;
    }

    jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
    return (jobResult == MEM_JOB_OK) ? TRUE : FALSE;
}

void TestManager_FillPattern(Mem_DataType* buffer, Mem_LengthType length, uint8 seed)
{
    Mem_LengthType i;
    for (i = 0u; i < length; i++)
    {
        buffer[i] = (Mem_DataType)(seed + (uint8)i);
    }
}

boolean TestManager_BufferEquals(const Mem_DataType* lhs, const Mem_DataType* rhs, Mem_LengthType length)
{
    Mem_LengthType i;
    for (i = 0u; i < length; i++)
    {
        if (lhs[i] != rhs[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}

void TestManager_CopyFromFlash(Mem_AddressType sourceAddress, Mem_DataType* dest, Mem_LengthType length)
{
    Mem_LengthType i;
    for (i = 0u; i < length; i++)
    {
        dest[i] = *(const volatile Mem_DataType*)((uintptr_t)sourceAddress + (uintptr_t)i);
    }
}

boolean TestManager_IsFlashRangeErased(Mem_AddressType address, Mem_LengthType length)
{
    Mem_LengthType i;
    for (i = 0u; i < length; i++)
    {
        if (*(const volatile Mem_DataType*)((uintptr_t)address + (uintptr_t)i) != (Mem_DataType)MEM_ERASED_VALUE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*===========================================================================*/
/* 7. INTERACTIVE UART MENU & RUNNER                                         */
/*===========================================================================*/

static void TestManager_LogSectorMenu(void)
{
    TestManager_LogString("\n[MENU] Chon Sector Flash de kiem thu:\n");
    TestManager_LogString("  3 - Sector 3 (16 KB  - 0x0800C000)\n");
    TestManager_LogString("  4 - Sector 4 (64 KB  - 0x08010000)\n");
    TestManager_LogString("  5 - Sector 5 (128 KB - 0x08020000)\n");
    TestManager_LogString("  6 - Sector 6 (128 KB - 0x08040000)\n");
    TestManager_LogString("  7 - Sector 7 (128 KB - 0x08060000) [Mac dinh]\n");
    TestManager_LogString("  (*) Chu y: KHONG THE chon Sector 0, 1, 2 vi chua Code & Vector Table!\n");
    TestManager_LogString("Lua chon Sector (3-7, Enter de chon 7): ");
}

static void TestManager_SelectSectorInteractive(void)
{
    boolean valid = FALSE;

    TestManager_UartFlushRx();
    TestManager_LogSectorMenu();

    while (valid == FALSE)
    {
        char choice = TestManager_UartReadCharBlocking();

        if ((choice == '\r') || (choice == '\n'))
        {
            (void)TestManager_SetTargetSector(7u);
            TestManager_LogString("7\n[OK] Chon Sector 7 (Mac dinh - 128 KB)\n");
            valid = TRUE;
            break;
        }

        if ((choice == ' ') || (choice == '\t'))
        {
            continue;
        }

        TestManager_UartWriteChar(choice);
        TestManager_LogString("\n");

        if ((choice == '0') || (choice == '1') || (choice == '2'))
        {
            TestManager_LogString("[ERROR] Sector 0/1/2 chua Vector Table & Code dang chay! Khong hop le.\n");
            TestManager_LogString("Vui long chon lai (3-7): ");
        }
        else if ((choice >= '3') && (choice <= '7'))
        {
            uint8 sectorId = (uint8)(choice - '0');
            (void)TestManager_SetTargetSector(sectorId);
            TestManager_LogString("[OK] Da chon Sector ");
            TestManager_LogUnsigned((uint32)sectorId);
            TestManager_LogString(" (Address=");
            TestManager_LogHex32((uint32)g_CurrentTestSector.SectorAddress);
            TestManager_LogString(", Size=");
            TestManager_LogHex32((uint32)g_CurrentTestSector.SectorLength);
            TestManager_LogString(")\n");
            valid = TRUE;
        }
        else
        {
            TestManager_LogString("[ERROR] Lua chon khong hop le. Vui long chon lai (3-7): ");
        }
    }
}

/*===========================================================================*/
/* 7. MAIN TEST MENU & RUNNER                                                */
/*===========================================================================*/

static void TestManager_LogMenu(void)
{
    TestManager_LogString("\n================ AUTOSAR MEM TEST MENU ================\n");
    TestManager_LogString("  1 - INIT Group\n");
    TestManager_LogString("  2 - DET Group\n");
    TestManager_LogString("  3 - STATUS Group\n");
    TestManager_LogString("  4 - READ Group\n");
    TestManager_LogString("  5 - WRITE Group (Flash Sector Test)\n");
    TestManager_LogString("  6 - ERASE Group (Flash Sector Test)\n");
    TestManager_LogString("  7 - BLANKCHECK Group (Flash Sector Test)\n");
    TestManager_LogString("  8 - JOB Group\n");
    TestManager_LogString("  9 - BOUNDARY Group (Neighbor Isolation & Edge Test)\n");
    TestManager_LogString("  A - RUN ALL 54 TESTCASES\n");
    TestManager_LogString("  M - Interactive Manual CLI Mode\n");
    TestManager_LogString("=======================================================\n");
    TestManager_LogString("Choice (1-9, A, M): ");
}

static TestGroupType TestManager_GetMenuSelection(char choice)
{
    switch (TestManager_ToUpper(choice))
    {
        case '1': return TEST_INIT;
        case '2': return TEST_DET;
        case '3': return TEST_STATUS;
        case '4': return TEST_READ;
        case '5': return TEST_WRITE;
        case '6': return TEST_ERASE;
        case '7': return TEST_BLANKCHECK;
        case '8': return TEST_JOB;
        case '9': return TEST_BOUNDARY;
        case 'A': return TEST_ALL_GROUPS;
        case 'M': return TEST_MANUAL_CLI;
        default:  return TEST_NONE;
    }
}

static TestGroupType TestManager_SelectGroupInteractive(void)
{
    TestGroupType selection = TEST_NONE;

    while (selection == TEST_NONE)
    {
        char choice;

        TestManager_UartFlushRx();
        TestManager_LogMenu();

        do
        {
            choice = TestManager_UartReadCharBlocking();
        }
        while ((choice == '\r') || (choice == '\n') || (choice == ' ') || (choice == '\t'));

        TestManager_UartWriteChar(choice);
        TestManager_LogString("\n");

        selection = TestManager_GetMenuSelection(choice);
        if (selection == TEST_NONE)
        {
            TestManager_LogString("[MENU] Invalid selection. Use 1-9, A or M.\n");
        }
    }

    return selection;
}

static void TestManager_RunSelectedGroup(TestGroupType group)
{
    switch (group)
    {
        case TEST_INIT:       Init_Test();       break;
        case TEST_DET:        Det_Test();        break;
        case TEST_STATUS:     Status_Test();     break;
        case TEST_READ:       Read_Test();       break;
        case TEST_WRITE:      Write_Test();      break;
        case TEST_ERASE:      Erase_Test();      break;
        case TEST_BLANKCHECK: BlankCheck_Test(); break;
        case TEST_JOB:        Job_Test();        break;
        case TEST_BOUNDARY:   Boundary_Test();   break;
        case TEST_MANUAL_CLI: Manual_Test();     break;
        default:                                 break;
    }
}

static void TestManager_RunSelection(TestGroupType selection)
{
    if (selection == TEST_ALL_GROUPS)
    {
        TestManager_RunSelectedGroup(TEST_INIT);
        TestManager_RunSelectedGroup(TEST_DET);
        TestManager_RunSelectedGroup(TEST_STATUS);
        TestManager_RunSelectedGroup(TEST_READ);
        TestManager_RunSelectedGroup(TEST_WRITE);
        TestManager_RunSelectedGroup(TEST_ERASE);
        TestManager_RunSelectedGroup(TEST_BLANKCHECK);
        TestManager_RunSelectedGroup(TEST_JOB);
        TestManager_RunSelectedGroup(TEST_BOUNDARY);
    }
    else
    {
        TestManager_RunSelectedGroup(selection);
    }
}

void TestManager_Run(void)
{
    TestManager_PlatformInit();

#if (TEST_ENABLE_UART_MENU == STD_ON) && (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_IsDebuggerConnected() == FALSE)
    {
        TestManager_LogRunBanner();
        TestManager_LogString("[MENU] Interactive UART selection is enabled.\n");

        for (;;)
        {
            TestGroupType selection = TestManager_SelectGroupInteractive();

            if (selection == TEST_MANUAL_CLI)
            {
                TestManager_RunSelection(selection);
                continue;
            }

            if ((selection == TEST_WRITE) || (selection == TEST_ERASE) ||
                (selection == TEST_BLANKCHECK) || (selection == TEST_BOUNDARY) ||
                (selection == TEST_ALL_GROUPS))
            {
                TestManager_SelectSectorInteractive();
            }

            TestManager_ResetReport();
            TestManager_LogString("[RUN] selected=");
            TestManager_LogString(TestManager_GetGroupName(selection));
            TestManager_LogString(" (Target Sector ");
            TestManager_LogUnsigned((uint32)g_CurrentTestSector.SectorId);
            TestManager_LogString(", Addr=");
            TestManager_LogHex32((uint32)g_CurrentTestSector.SectorAddress);
            TestManager_LogString(", Size=");
            TestManager_LogHex32((uint32)g_CurrentTestSector.SectorLength);
            TestManager_LogString(")\n");

            TestManager_RunSelection(selection);
            TestManager_Finish();
            TestManager_LogString("[MENU] Run complete. Select next group.\n");
        }
    }
    else
    {
        /* Debugger is connected: bypass UART interactive blocking menu and run ACTIVE_TEST_GROUP */
        TestManager_ResetReport();
        TestManager_LogRunBanner();
        TestManager_LogString("[DEBUG] Debugger attached (C_DEBUGEN=1). Bypassing UART menu and running ACTIVE_TEST_GROUP.\n");
        TestManager_RunSelection((TestGroupType)ACTIVE_TEST_GROUP);
        TestManager_Finish();
    }
#else
    TestManager_ResetReport();
    TestManager_LogRunBanner();
    TestManager_RunSelection((TestGroupType)ACTIVE_TEST_GROUP);
    TestManager_Finish();
#endif
}

void TestManager_Finish(void)
{
    g_TestReport.Completed = TRUE;

    TestManager_LogString("\n[FINAL] total=");
    TestManager_LogUnsigned(g_TestReport.OverallTotal);
    TestManager_LogString(" pass=");
    TestManager_LogUnsigned(g_TestReport.OverallPassed);
    TestManager_LogString(" fail=");
    TestManager_LogUnsigned(g_TestReport.OverallFailed);
    TestManager_LogString(" fault=");
    TestManager_LogUnsigned(g_TestReport.LastFault);
    TestManager_LogString("\n");

#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
    if ((g_TestReport.OverallFailed == 0u) && (g_TestReport.LastFault == (uint32)TEST_FAULT_NONE))
    {
        TestManager_LedOn();
    }
    else
    {
        TestManager_LedOff();
    }
#endif
}
