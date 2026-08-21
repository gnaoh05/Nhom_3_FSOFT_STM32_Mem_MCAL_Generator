/******************************************************************************
 * FILE: TestManager.c
 * MÔ TẢ: Khung kiểm thử AUTOSAR MEM Driver mức thanh ghi cho STM32F401
 ******************************************************************************/

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


#define TEST_REPORT_SIGNATURE    0x54455354UL
#define TEST_INVALID_INDEX       0xFFFFFFFFUL
#define TEST_LED_PIN_MASK        (1UL << 5)
#define TEST_SCB_CFSR            (*(volatile uint32*)0xE000ED28UL)
#define TEST_SCB_HFSR            (*(volatile uint32*)0xE000ED2CUL)
#define TEST_SCB_MMFAR           (*(volatile uint32*)0xE000ED34UL)
#define TEST_SCB_BFAR            (*(volatile uint32*)0xE000ED38UL)
#define TEST_COREDEBUG_DHCSR      (*(volatile const uint32*)0xE000EDF0UL)
#define TEST_COREDEBUG_C_DEBUGEN  (1UL << 0u)
/* Run của STM32CubeIDE cũng dùng GDB server trong thời gian nạp ELF. Chờ đủ
 * lâu để Run kịp ngắt kết nối trước khi kết luận đây là một phiên Debug. */
#define TEST_DEBUG_ATTACH_POLL_LIMIT  4000000UL

extern uint32 _sidata;
extern uint32 _sdata;
extern uint32 _edata;

volatile TestReportType g_TestReport;

static uint32  TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
static boolean TestManager_UartReady = FALSE;
static boolean TestManager_DebuggerAttached = FALSE;

static boolean TestManager_IsDebuggerAttached(void)
{
    return ((TEST_COREDEBUG_DHCSR & TEST_COREDEBUG_C_DEBUGEN) != 0u) ? TRUE : FALSE;
}

static boolean TestManager_DetectDebugSession(void)
{
    uint32 pollCount;

    if (TestManager_IsDebuggerAttached() == FALSE)
    {
        return FALSE;
    }

    for (pollCount = 0u; pollCount < TEST_DEBUG_ATTACH_POLL_LIMIT; pollCount++)
    {
        if (TestManager_IsDebuggerAttached() == FALSE)
        {
            /* Nút Run đã nạp xong ELF và GDB server đã ngắt kết nối. */
            return FALSE;
        }
    }

    /* Debugger vẫn còn kết nối sau khoảng chờ: đây là phiên Debug thực sự. */
    return TRUE;
}

static const char* TestManager_GetGroupName(TestGroupType group)
{
    const char* name;

    switch (group)
    {
        case TEST_INIT:
            name = "INIT";
            break;
        case TEST_DET:
            name = "DET";
            break;
        case TEST_STATUS:
            name = "STATUS";
            break;
        case TEST_READ:
            name = "READ";
            break;
        case TEST_WRITE:
            name = "WRITE";
            break;
        case TEST_ERASE:
            name = "ERASE";
            break;
        case TEST_BLANKCHECK:
            name = "BLANKCHECK";
            break;
        case TEST_JOB:
            name = "JOB";
            break;
        case TEST_ALL_GROUPS:
            name = "ALL";
            break;
        case TEST_NONE:
        default:
            name = "NONE";
            break;
    }

    return name;
}

static void TestManager_Delay(volatile uint32 cycles)
{
    while (cycles > 0u)
    {
        cycles--;
    }
}

static void TestManager_LedInit(void)
{
    STM32_RCC->AHB1ENR |= STM32_RCC_AHB1ENR_GPIOAEN;

    STM32_GPIOA->MODER &= ~(0x3UL << (5u * 2u));
    STM32_GPIOA->MODER |=  (0x1UL << (5u * 2u));
    STM32_GPIOA->OTYPER &= ~TEST_LED_PIN_MASK;
    STM32_GPIOA->OSPEEDR |= (0x3UL << (5u * 2u));
    STM32_GPIOA->PUPDR &= ~(0x3UL << (5u * 2u));
}

static void TestManager_LedOn(void)
{
    STM32_GPIOA->BSRR = TEST_LED_PIN_MASK;
}

static void TestManager_LedOff(void)
{
    STM32_GPIOA->BSRR = (TEST_LED_PIN_MASK << 16u);
}

static void TestManager_LedToggle(void)
{
    if ((STM32_GPIOA->ODR & TEST_LED_PIN_MASK) != 0u)
    {
        TestManager_LedOff();
    }
    else
    {
        TestManager_LedOn();
    }
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

static void TestManager_PlatformInit(void)
{
    TestManager_DebuggerAttached = TestManager_DetectDebugSession();

#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
    TestManager_LedInit();
    TestManager_LedOff();
#endif

#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_DebuggerAttached == FALSE)
    {
        TestManager_UartInit();
    }
    else
    {
        TestManager_UartReady = FALSE;
    }
#endif
}

static void TestManager_UartWriteChar(char ch)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_UartReady == TRUE)
    {
        while ((STM32_USART2->SR & STM32_USART_SR_TXE) == 0u)
        {
        }

        STM32_USART2->DR = (uint8)ch;
    }
#else
    (void)ch;
#endif
}

static void TestManager_LogString(const char* text)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (text != NULL_PTR)
    {
        while (*text != '\0')
        {
            if (*text == '\n')
            {
                TestManager_UartWriteChar('\r');
            }

            TestManager_UartWriteChar(*text);
            text++;
        }
    }
#else
    (void)text;
#endif
}

static void TestManager_LogUnsigned(uint32 value)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    char   buffer[10];
    uint32 index = 0u;

    if (value == 0u)
    {
        TestManager_UartWriteChar('0');
    }
    else
    {
        while (value > 0u)
        {
            buffer[index] = (char)('0' + (value % 10u));
            value /= 10u;
            index++;
        }

        while (index > 0u)
        {
            index--;
            TestManager_UartWriteChar(buffer[index]);
        }
    }
#else
    (void)value;
#endif
}

static void TestManager_LogHex32(uint32 value)
{
#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    uint32 nibbleShift;

    TestManager_LogString("0x");

    for (nibbleShift = 28u; ; nibbleShift -= 4u)
    {
        uint32 nibble = (value >> nibbleShift) & 0xFUL;

        if (nibble < 10u)
        {
            TestManager_UartWriteChar((char)('0' + nibble));
        }
        else
        {
            TestManager_UartWriteChar((char)('A' + (nibble - 10u)));
        }

        if (nibbleShift == 0u)
        {
            break;
        }
    }
#else
    (void)value;
#endif
}

static boolean TestManager_UartTryReadChar(char* ch)
{
    boolean available = FALSE;

#if (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if ((ch != NULL_PTR) &&
        (TestManager_UartReady == TRUE) &&
        ((STM32_USART2->SR & STM32_USART_SR_RXNE) != 0u))
    {
        *ch = (char)(STM32_USART2->DR & 0xFFu);
        available = TRUE;
    }
#else
    (void)ch;
#endif

    return available;
}

static char TestManager_UartReadCharBlocking(void)
{
    char ch = '\0';

    while (TestManager_UartTryReadChar(&ch) == FALSE)
    {
    }

    return ch;
}

static void TestManager_UartFlushRx(void)
{
    char discard;

    while (TestManager_UartTryReadChar(&discard) == TRUE)
    {
    }
}

static char TestManager_ToUpper(char ch)
{
    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static void TestManager_ResetReport(void)
{
    uint32 index;

    g_TestReport.Signature     = TEST_REPORT_SIGNATURE;
    g_TestReport.Completed     = FALSE;
    g_TestReport.OverallTotal  = 0u;
    g_TestReport.OverallPassed = 0u;
    g_TestReport.OverallFailed = 0u;
    g_TestReport.ActiveGroup   = TEST_NONE;
    g_TestReport.LastFault     = TEST_FAULT_NONE;
    g_TestReport.FaultCfsr     = 0u;
    g_TestReport.FaultHfsr     = 0u;
    g_TestReport.FaultMmfar    = 0u;
    g_TestReport.FaultBfar     = 0u;
    g_TestReport.GroupCount    = 0u;
    g_TestReport.CaseCount     = 0u;
    g_TestReport.FirstFailedCaseIndex = TEST_REPORT_INVALID_INDEX;
    g_TestReport.LastFailedCaseIndex  = TEST_REPORT_INVALID_INDEX;
    g_TestReport.DroppedCaseCount     = 0u;

    for (index = 0u; index < TEST_MAX_GROUP_REPORTS; index++)
    {
        g_TestReport.GroupReports[index].GroupId = TEST_NONE;
        g_TestReport.GroupReports[index].Total   = 0u;
        g_TestReport.GroupReports[index].Passed  = 0u;
        g_TestReport.GroupReports[index].Failed  = 0u;
    }

    for (index = 0u; index < TEST_MAX_CASE_REPORTS; index++)
    {
        g_TestReport.CaseReports[index].Sequence       = 0u;
        g_TestReport.CaseReports[index].GroupId        = TEST_NONE;
        g_TestReport.CaseReports[index].CaseId         = 0u;
        g_TestReport.CaseReports[index].Result         = TEST_CASE_NOT_RUN;
        g_TestReport.CaseReports[index].Expected       = 0u;
        g_TestReport.CaseReports[index].Actual         = 0u;
        g_TestReport.CaseReports[index].SourceLine     = 0u;
        g_TestReport.CaseReports[index].DetValid       = FALSE;
        g_TestReport.CaseReports[index].DetModuleId    = 0u;
        g_TestReport.CaseReports[index].DetInstanceId  = 0u;
        g_TestReport.CaseReports[index].DetApiId       = 0u;
        g_TestReport.CaseReports[index].DetErrorId     = 0u;
        g_TestReport.CaseReports[index].CaseName       = NULL_PTR;
        g_TestReport.CaseReports[index].RequirementIds = NULL_PTR;
        g_TestReport.CaseReports[index].SpecPages      = NULL_PTR;
        g_TestReport.CaseReports[index].SourceFile     = NULL_PTR;
    }

    TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
}

static void TestManager_LogGroupBanner(TestGroupType group)
{
    TestManager_LogString("\n[GROUP] ");
    TestManager_LogString(TestManager_GetGroupName(group));
    TestManager_LogString("\n");
}

static void TestManager_LogGroupSummary(void)
{
    if (TestManager_CurrentGroupIndex < g_TestReport.GroupCount)
    {
        const TestGroupReportType* summary = (const TestGroupReportType*)&g_TestReport.GroupReports[TestManager_CurrentGroupIndex];

        TestManager_LogString("[SUMMARY] ");
        TestManager_LogString(TestManager_GetGroupName((TestGroupType)summary->GroupId));
        TestManager_LogString(" total=");
        TestManager_LogUnsigned(summary->Total);
        TestManager_LogString(" pass=");
        TestManager_LogUnsigned(summary->Passed);
        TestManager_LogString(" fail=");
        TestManager_LogUnsigned(summary->Failed);
        TestManager_LogString("\n");
    }
}

static void TestManager_LogRunBanner(void)
{
    TestManager_LogString("\nAUTOSAR MEM test console ready\n");
    TestManager_LogString("Flash test sector=");
    TestManager_LogHex32((uint32)TEST_FLASH_SECTOR_ADDRESS);
    TestManager_LogString(" size=");
    TestManager_LogHex32((uint32)TEST_FLASH_SECTOR_LENGTH);
    TestManager_LogString("\n");
}

static void TestManager_LogFailureDetail(
        const char* sourceFile,
        uint32 sourceLine,
        const Det_LastErrorType* detSnapshot)
{
    TestManager_LogString("    [DETAIL] source=");
    TestManager_LogString((sourceFile != NULL_PTR) ? sourceFile : "unknown");
    TestManager_LogString(":");
    TestManager_LogUnsigned(sourceLine);

    if ((detSnapshot != NULL_PTR) && (detSnapshot->Valid == TRUE))
    {
        TestManager_LogString(" det(module=");
        TestManager_LogUnsigned(detSnapshot->ModuleId);
        TestManager_LogString(",instance=");
        TestManager_LogUnsigned(detSnapshot->InstanceId);
        TestManager_LogString(",api=");
        TestManager_LogHex32(detSnapshot->ApiId);
        TestManager_LogString(",error=");
        TestManager_LogHex32(detSnapshot->ErrorId);
        TestManager_LogString(")");
    }
    else
    {
        TestManager_LogString(" det=none");
    }

    TestManager_LogString("\n");
}

static void TestManager_LogMenu(void)
{
    TestManager_LogString("\n[MENU] Select test group\n");
    TestManager_LogString("  1 - INIT\n");
    TestManager_LogString("  2 - DET\n");
    TestManager_LogString("  3 - STATUS\n");
    TestManager_LogString("  4 - READ\n");
    TestManager_LogString("  5 - WRITE (flash sector test)\n");
    TestManager_LogString("  6 - ERASE (flash sector test)\n");
    TestManager_LogString("  7 - BLANKCHECK (flash sector test)\n");
    TestManager_LogString("  8 - JOB\n");
    TestManager_LogString("  A - ALL GROUPS\n");
    TestManager_LogString("Choice: ");
}

static TestGroupType TestManager_GetMenuSelection(char choice)
{
    TestGroupType selection = TEST_NONE;

    switch (TestManager_ToUpper(choice))
    {
        case '1':
            selection = TEST_INIT;
            break;
        case '2':
            selection = TEST_DET;
            break;
        case '3':
            selection = TEST_STATUS;
            break;
        case '4':
            selection = TEST_READ;
            break;
        case '5':
            selection = TEST_WRITE;
            break;
        case '6':
            selection = TEST_ERASE;
            break;
        case '7':
            selection = TEST_BLANKCHECK;
            break;
        case '8':
            selection = TEST_JOB;
            break;
        case 'A':
            selection = TEST_ALL_GROUPS;
            break;
        default:
            break;
    }

    return selection;
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
            TestManager_LogString("[MENU] Invalid selection. Use 1-8 or A.\n");
        }
    }

    return selection;
}

static void TestManager_RunSelectedGroup(TestGroupType group)
{
    switch (group)
    {
        case TEST_INIT:
            Init_Test();
            break;
        case TEST_DET:
            Det_Test();
            break;
        case TEST_STATUS:
            Status_Test();
            break;
        case TEST_READ:
            Read_Test();
            break;
        case TEST_WRITE:
            Write_Test();
            break;
        case TEST_ERASE:
            Erase_Test();
            break;
        case TEST_BLANKCHECK:
            BlankCheck_Test();
            break;
        case TEST_JOB:
            Job_Test();
            break;
        case TEST_NONE:
        case TEST_ALL_GROUPS:
        default:
            break;
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
    }
    else
    {
        TestManager_RunSelectedGroup(selection);
    }
}

void TestManager_BeginGroup(TestGroupType group)
{
    uint32 nextIndex = g_TestReport.GroupCount;

    g_TestReport.ActiveGroup = group;

    if (nextIndex < TEST_MAX_GROUP_REPORTS)
    {
        g_TestReport.GroupReports[nextIndex].GroupId = group;
        g_TestReport.GroupReports[nextIndex].Total   = 0u;
        g_TestReport.GroupReports[nextIndex].Passed  = 0u;
        g_TestReport.GroupReports[nextIndex].Failed  = 0u;

        g_TestReport.GroupCount = nextIndex + 1u;
        TestManager_CurrentGroupIndex = nextIndex;
    }
    else
    {
        TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
    }

    TestManager_LogGroupBanner(group);
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
    Det_LastErrorType detSnapshot = Det_GetLastError();
    uint32 result = (passed == TRUE) ? TEST_CASE_PASS : TEST_CASE_FAIL;
    uint32 nextCaseIndex = g_TestReport.CaseCount;
    uint32 storedCaseIndex = TEST_REPORT_INVALID_INDEX;

    g_TestReport.OverallTotal++;

    if (passed == TRUE)
    {
        g_TestReport.OverallPassed++;
    }
    else
    {
        g_TestReport.OverallFailed++;
    }

    if (TestManager_CurrentGroupIndex < g_TestReport.GroupCount)
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

    if (nextCaseIndex < TEST_MAX_CASE_REPORTS)
    {
        g_TestReport.CaseReports[nextCaseIndex].Sequence       = g_TestReport.OverallTotal;
        g_TestReport.CaseReports[nextCaseIndex].GroupId        = g_TestReport.ActiveGroup;
        g_TestReport.CaseReports[nextCaseIndex].CaseId         = caseId;
        g_TestReport.CaseReports[nextCaseIndex].Result         = result;
        g_TestReport.CaseReports[nextCaseIndex].Expected       = expected;
        g_TestReport.CaseReports[nextCaseIndex].Actual         = actual;
        g_TestReport.CaseReports[nextCaseIndex].SourceLine     = sourceLine;
        g_TestReport.CaseReports[nextCaseIndex].DetValid       = detSnapshot.Valid;
        g_TestReport.CaseReports[nextCaseIndex].DetModuleId    = detSnapshot.ModuleId;
        g_TestReport.CaseReports[nextCaseIndex].DetInstanceId  = detSnapshot.InstanceId;
        g_TestReport.CaseReports[nextCaseIndex].DetApiId       = detSnapshot.ApiId;
        g_TestReport.CaseReports[nextCaseIndex].DetErrorId     = detSnapshot.ErrorId;
        g_TestReport.CaseReports[nextCaseIndex].CaseName       = caseName;
        g_TestReport.CaseReports[nextCaseIndex].RequirementIds = requirementIds;
        g_TestReport.CaseReports[nextCaseIndex].SpecPages      = specPages;
        g_TestReport.CaseReports[nextCaseIndex].SourceFile     = sourceFile;
        g_TestReport.CaseCount = nextCaseIndex + 1u;
        storedCaseIndex = nextCaseIndex;
    }
    else
    {
        g_TestReport.DroppedCaseCount++;
    }

    if (passed == FALSE)
    {
        if (g_TestReport.FirstFailedCaseIndex == TEST_REPORT_INVALID_INDEX)
        {
            g_TestReport.FirstFailedCaseIndex = storedCaseIndex;
        }

        g_TestReport.LastFailedCaseIndex = storedCaseIndex;
    }

    TestManager_LogString("  [");
    TestManager_LogString((passed == TRUE) ? "PASS" : "FAIL");
    TestManager_LogString("] ");
    TestManager_LogHex32(caseId);
    TestManager_LogString(" ");

    if (caseName != NULL_PTR)
    {
        TestManager_LogString(caseName);
        TestManager_LogString(" ");
    }

    if ((requirementIds != NULL_PTR) && (requirementIds[0] != '\0'))
    {
        TestManager_LogString("req=");
        TestManager_LogString(requirementIds);
        TestManager_LogString(" ");
    }

    if ((specPages != NULL_PTR) && (specPages[0] != '\0'))
    {
        TestManager_LogString("page=");
        TestManager_LogString(specPages);
        TestManager_LogString(" ");
    }

    TestManager_LogString("exp=");
    TestManager_LogHex32(expected);
    TestManager_LogString(" act=");
    TestManager_LogHex32(actual);
    TestManager_LogString("\n");

    if (passed == FALSE)
    {
        TestManager_LogFailureDetail(sourceFile, sourceLine, &detSnapshot);
    }
}

void TestManager_RecordCase(uint32 caseId, const char* caseName, boolean passed, uint32 expected, uint32 actual)
{
    TestManager_RecordCaseTraceAt(caseId, caseName, NULL_PTR, NULL_PTR, passed,
                                  expected, actual, NULL_PTR, 0u);
}

void TestManager_EndGroup(void)
{
    TestManager_LogGroupSummary();
    TestManager_CurrentGroupIndex = TEST_INVALID_INDEX;
}

void TestManager_ReportFault(TestFaultType fault)
{
    g_TestReport.LastFault = fault;
    g_TestReport.FaultCfsr  = TEST_SCB_CFSR;
    g_TestReport.FaultHfsr  = TEST_SCB_HFSR;
    g_TestReport.FaultMmfar = TEST_SCB_MMFAR;
    g_TestReport.FaultBfar  = TEST_SCB_BFAR;

    TestManager_LogString("\n[FAULT] code=");
    TestManager_LogUnsigned((uint32)fault);
    TestManager_LogString("\n");

#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
    TestManager_LedOff();
#endif
}

void TestManager_PrepareDriver(void)
{
    Det_ResetLastError();
    Mem_Init(NULL_PTR);
    Mem_DeInit();
    Det_ResetLastError();
    Mem_Init(NULL_PTR);
    Det_ResetLastError();
}

void TestManager_ShutdownDriver(void)
{
    Det_ResetLastError();
    Mem_Init(NULL_PTR);
    Det_ResetLastError();
    Mem_DeInit();
    Det_ResetLastError();
}

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

    return (boolean)((lastError.Valid      == TRUE) &&
                     (lastError.ModuleId   == MEM_MODULE_ID) &&
                     (lastError.InstanceId == MEM_INDEX)/*Lỗi*/ &&
                     (lastError.ApiId      == apiId) &&
                     (lastError.ErrorId    == errorId));
}

MemAcc_MemJobResultType TestManager_ExecuteMainUntilDone(Mem_InstanceIdType instanceId, uint32 timeoutTicks)
{
    MemAcc_MemJobResultType result = Mem_GetJobResult(instanceId);
    uint32                  ticks  = 0u;

    while ((result == MEM_JOB_PENDING) && (ticks < timeoutTicks))
    {
        Mem_MainFunction();
        result = Mem_GetJobResult(instanceId);
        ticks++;
    }

    return result;
}

boolean TestManager_IsFlashTestAreaSafe(void)
{
    boolean   sectorMatch   = FALSE;
    boolean   writeFits     = FALSE;
    boolean   imageIsBefore = FALSE;
    uintptr_t flashImageEnd;
    uintptr_t dataInitSize;
    uint16    batchIndex;
    uint32    sectorIndex;

    dataInitSize  = (uintptr_t)&_edata - (uintptr_t)&_sdata;
    flashImageEnd = (uintptr_t)&_sidata + dataInitSize;

    for (batchIndex = 0u;
         batchIndex < Mem_ConfigData.MemInstances[TEST_FLASH_INSTANCE].MemNumberOfBatches;
         batchIndex++)
    {
        const Mem_SectorBatchConfigType* batch =
                &Mem_ConfigData.MemInstances[TEST_FLASH_INSTANCE].MemSectorBatches[batchIndex];

        for (sectorIndex = 0u; sectorIndex < batch->MemNumberOfSectors; sectorIndex++)
        {
            uint32 sectorAddress = batch->MemStartAddress +
                                   (sectorIndex * batch->MemEraseSectorSize);

            if ((sectorAddress == (uint32)TEST_FLASH_SECTOR_ADDRESS) &&
                (batch->MemEraseSectorSize == (uint32)TEST_FLASH_SECTOR_LENGTH))
            {
                sectorMatch = TRUE;
                break;
            }
        }

        if (sectorMatch == TRUE)
        {
            break;
        }
    }

    writeFits = (boolean)((TEST_FLASH_WRITE_ADDRESS + TEST_FLASH_COMPARE_LENGTH) <=
                          (TEST_FLASH_SECTOR_ADDRESS + TEST_FLASH_SECTOR_LENGTH));

    imageIsBefore = (boolean)((uintptr_t)TEST_FLASH_SECTOR_ADDRESS >= flashImageEnd);

    return (boolean)((sectorMatch == TRUE) && (writeFits == TRUE) && (imageIsBefore == TRUE));
}

boolean TestManager_EraseTestSectorAndWait(void)
{
    boolean                success = FALSE;
    Std_ReturnType         retVal;
    MemAcc_MemJobResultType result;

    if (TestManager_IsFlashTestAreaSafe() == TRUE)
    {
        retVal = Mem_Erase(TEST_FLASH_INSTANCE, TEST_FLASH_SECTOR_ADDRESS, TEST_FLASH_SECTOR_LENGTH);

        if (retVal == E_OK)
        {
            result = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
            success = (boolean)((result == MEM_JOB_OK) &&
                                (TestManager_IsFlashRangeErased(TEST_FLASH_WRITE_ADDRESS, TEST_FLASH_COMPARE_LENGTH) == TRUE));
        }
    }

    return success;
}

void TestManager_FillPattern(Mem_DataType* buffer, Mem_LengthType length, uint8 seed)
{
    Mem_LengthType index;

    for (index = 0u; index < length; index++)
    {
        buffer[index] = (Mem_DataType)(seed + (uint8)(index * 13u) + 1u);
    }
}

boolean TestManager_BufferEquals(const Mem_DataType* lhs, const Mem_DataType* rhs, Mem_LengthType length)
{
    boolean        matches = TRUE;
    Mem_LengthType index;

    for (index = 0u; index < length; index++)
    {
        if (lhs[index] != rhs[index])
        {
            matches = FALSE;
            break;
        }
    }

    return matches;
}

void TestManager_CopyFromFlash(Mem_AddressType sourceAddress, Mem_DataType* dest, Mem_LengthType length)
{
    Mem_LengthType index;

    for (index = 0u; index < length; index++)
    {
        dest[index] = *(const volatile Mem_DataType*)((uintptr_t)sourceAddress + (uintptr_t)index);
    }
}

boolean TestManager_IsFlashRangeErased(Mem_AddressType address, Mem_LengthType length)
{
    boolean        erased = TRUE;
    Mem_LengthType index;

    for (index = 0u; index < length; index++)
    {
        if (*(const volatile Mem_DataType*)((uintptr_t)address + (uintptr_t)index) != (Mem_DataType)MEM_ERASED_VALUE)
        {
            erased = FALSE;
            break;
        }
    }

    return erased;
}

void TestManager_Run(void)
{
    TestManager_PlatformInit();

#if (TEST_ENABLE_UART_MENU == STD_ON) && (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_DebuggerAttached == FALSE)
    {
        TestManager_LogRunBanner();
        TestManager_LogString("[MENU] Interactive UART selection is enabled.\n");

        for (;;)
        {
            TestGroupType selection = TestManager_SelectGroupInteractive();

            TestManager_ResetReport();
            TestManager_LogString("[RUN] selected=");
            TestManager_LogString(TestManager_GetGroupName(selection));
            TestManager_LogString("\n");

            TestManager_RunSelection(selection);
            TestManager_Finish();
            TestManager_LogString("[MENU] Run complete. Select next group.\n");
        }
    }
#endif

    TestManager_ResetReport();
    TestManager_LogRunBanner();
    TestManager_RunSelection((TestGroupType)ACTIVE_TEST_GROUP);
    TestManager_Finish();
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

#if (TEST_ENABLE_UART_MENU == STD_ON) && (TEST_ENABLE_UART_OUTPUT == STD_ON)
    if (TestManager_DebuggerAttached == FALSE)
    {
#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
        if ((g_TestReport.OverallFailed == 0u) && (g_TestReport.LastFault == TEST_FAULT_NONE))
        {
            TestManager_LedOn();
        }
        else
        {
            TestManager_LedOff();
        }
#endif
        return;
    }
#endif

    for (;;)
    {
#if (TEST_ENABLE_LED_OUTPUT == STD_ON)
        if ((g_TestReport.OverallFailed == 0u) && (g_TestReport.LastFault == TEST_FAULT_NONE))
        {
            TestManager_LedOn();
            TestManager_Delay(400000u);
        }
        else
        {
            TestManager_LedToggle();
            TestManager_Delay(120000u);
        }
#endif
    }
}
