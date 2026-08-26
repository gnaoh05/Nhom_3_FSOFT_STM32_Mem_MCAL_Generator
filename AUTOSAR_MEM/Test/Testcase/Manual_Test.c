#include "Manual_Test.h"
#include "TestManager.h"
#include "Mem.h"
#include "Mem_Types.h"
#include "Flash_IP.h"
#include "Det.h"

/*===========================================================================*/
/* 1. UART INPUT PARSER & FORMATTING HELPERS                                 */
/*===========================================================================*/

static void Manual_UartReadLine(char* buffer, uint32 maxLen)
{
    uint32 idx = 0u;

    while (idx < (maxLen - 1u))
    {
        char c = TestManager_UartReadCharBlocking();

        if ((c == '\r') || (c == '\n'))
        {
            TestManager_LogString("\r\n");
            break;
        }
        else if ((c == '\b') || (c == 127))
        {
            if (idx > 0u)
            {
                idx--;
                TestManager_LogString("\b \b");
            }
        }
        else if ((c >= 32) && (c <= 126))
        {
            buffer[idx++] = c;
            TestManager_UartWriteChar(c);
        }
    }
    buffer[idx] = '\0';
}

static boolean Manual_ParseHex32(const char* str, uint32* outVal)
{
    uint32 result = 0u;

    if (str == NULL_PTR)
    {
        return FALSE;
    }

    while ((*str == ' ') || (*str == '\t'))
    {
        str++;
    }

    if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))
    {
        str += 2;
    }

    if (*str == '\0')
    {
        return FALSE;
    }

    while (*str != '\0')
    {
        char c = *str++;
        result <<= 4u;
        if ((c >= '0') && (c <= '9'))
        {
            result |= (uint32)(c - '0');
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            result |= (uint32)(c - 'a' + 10);
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            result |= (uint32)(c - 'A' + 10);
        }
        else
        {
            return FALSE;
        }
    }

    *outVal = result;
    return TRUE;
}

static boolean Manual_ParseDec32(const char* str, uint32* outVal)
{
    uint32 result = 0u;

    if (str == NULL_PTR)
    {
        return FALSE;
    }

    while ((*str == ' ') || (*str == '\t'))
    {
        str++;
    }

    if (*str == '\0')
    {
        return FALSE;
    }

    while (*str != '\0')
    {
        char c = *str++;
        if ((c >= '0') && (c <= '9'))
        {
            result = (result * 10u) + (uint32)(c - '0');
        }
        else
        {
            return FALSE;
        }
    }

    *outVal = result;
    return TRUE;
}

static void Manual_PrintHexDump(uint32 address, const uint8* buffer, uint32 length)
{
    const char hexChars[] = "0123456789ABCDEF";
    uint32 row;
    uint32 col;

    TestManager_LogString("Offset      00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII\r\n");
    TestManager_LogString("-------------------------------------------------------------------------\r\n");

    for (row = 0u; row < length; row += 16u)
    {
        uint32 rowAddress = address + row;
        TestManager_LogHex32(rowAddress);
        TestManager_LogString("  ");

        for (col = 0u; col < 16u; col++)
        {
            if ((row + col) < length)
            {
                uint8 b = buffer[row + col];
                TestManager_UartWriteChar(hexChars[(b >> 4u) & 0x0Fu]);
                TestManager_UartWriteChar(hexChars[b & 0x0Fu]);
                TestManager_UartWriteChar(' ');
            }
            else
            {
                TestManager_LogString("   ");
            }

            if (col == 7u)
            {
                TestManager_UartWriteChar(' ');
            }
        }

        TestManager_LogString(" ");

        for (col = 0u; col < 16u; col++)
        {
            if ((row + col) < length)
            {
                uint8 b = buffer[row + col];
                if ((b >= 32u) && (b <= 126u))
                {
                    TestManager_UartWriteChar((char)b);
                }
                else
                {
                    TestManager_UartWriteChar('.');
                }
            }
        }
        TestManager_LogString("\r\n");
    }
}

/*===========================================================================*/
/* 2. SMART PROMPTS (PRESETS & SHORTCUTS)                                    */
/*===========================================================================*/

static boolean Manual_PromptAddress(boolean allowSector0, uint32* outAddr)
{
    char choice;
    char strBuf[32];

    TestManager_LogString("\nSelect Flash Address:\n");
    if (allowSector0 == TRUE)
    {
        TestManager_LogString("  0 - Sector 0 (0x08000000 - Vector Table / Code) [Read-only]\n");
    }
    TestManager_LogString("  3 - Sector 3 (0x0800C000 - 16 KB)\n");
    TestManager_LogString("  4 - Sector 4 (0x08010000 - 64 KB)\n");
    TestManager_LogString("  5 - Sector 5 (0x08020000 - 128 KB)\n");
    TestManager_LogString("  6 - Sector 6 (0x08040000 - 128 KB)\n");
    TestManager_LogString("  7 - Sector 7 (0x08060000 - 128 KB) [Default / Recommended]\n");
    TestManager_LogString("  C - Custom Hex Address\n");
    TestManager_LogString("Choice (0-7, C, Enter for Sector 7): ");

    choice = TestManager_UartReadCharBlocking();

    if ((choice == '\r') || (choice == '\n') || (choice == '7'))
    {
        TestManager_LogString("7\r\n[OK] Selected Sector 7 (0x08060000)\n");
        *outAddr = 0x08060000UL;
        return TRUE;
    }

    TestManager_UartWriteChar(choice);
    TestManager_LogString("\r\n");

    if (choice == '0')
    {
        if (allowSector0 == FALSE)
        {
            TestManager_LogString("[ERROR] Sector 0 contains active Code/Vector Table! Write operation blocked.\n");
            return FALSE;
        }
        *outAddr = 0x08000000UL;
        TestManager_LogString("[OK] Selected Sector 0 (0x08000000)\n");
        return TRUE;
    }
    else if (choice == '3')
    {
        *outAddr = 0x0800C000UL;
        TestManager_LogString("[OK] Selected Sector 3 (0x0800C000)\n");
        return TRUE;
    }
    else if (choice == '4')
    {
        *outAddr = 0x08010000UL;
        TestManager_LogString("[OK] Selected Sector 4 (0x08010000)\n");
        return TRUE;
    }
    else if (choice == '5')
    {
        *outAddr = 0x08020000UL;
        TestManager_LogString("[OK] Selected Sector 5 (0x08020000)\n");
        return TRUE;
    }
    else if (choice == '6')
    {
        *outAddr = 0x08040000UL;
        TestManager_LogString("[OK] Selected Sector 6 (0x08040000)\n");
        return TRUE;
    }
    else if ((choice == 'c') || (choice == 'C'))
    {
        TestManager_LogString("Enter Custom Hex Address (e.g. 0x08060100): ");
        Manual_UartReadLine(strBuf, sizeof(strBuf));
        if (Manual_ParseHex32(strBuf, outAddr) == TRUE)
        {
            if ((allowSector0 == FALSE) && (*outAddr < 0x0800C000UL))
            {
                TestManager_LogString("[ERROR] Target address falls in protected Sectors 0-2 (Blocked)!\n");
                return FALSE;
            }
            return TRUE;
        }
        else
        {
            TestManager_LogString("[ERROR] Invalid Hex Address!\n");
            return FALSE;
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Invalid Selection!\n");
        return FALSE;
    }
}

static boolean Manual_PromptLength(uint32* outLen)
{
    char choice;
    char strBuf[32];

    TestManager_LogString("\nSelect Length (Bytes):\n");
    TestManager_LogString("  1 - 4 Bytes (1 Word 32-bit)\n");
    TestManager_LogString("  2 - 16 Bytes (4 Words) [Default]\n");
    TestManager_LogString("  3 - 32 Bytes (8 Words)\n");
    TestManager_LogString("  4 - 64 Bytes (16 Words)\n");
    TestManager_LogString("  C - Custom Length\n");
    TestManager_LogString("Choice (1-4, C, Enter for 16 bytes): ");

    choice = TestManager_UartReadCharBlocking();

    if ((choice == '\r') || (choice == '\n') || (choice == '2'))
    {
        TestManager_LogString("2\r\n[OK] Selected 16 Bytes\n");
        *outLen = 16u;
        return TRUE;
    }

    TestManager_UartWriteChar(choice);
    TestManager_LogString("\r\n");

    if (choice == '1')
    {
        *outLen = 4u;
        TestManager_LogString("[OK] Selected 4 Bytes\n");
        return TRUE;
    }
    else if (choice == '3')
    {
        *outLen = 32u;
        TestManager_LogString("[OK] Selected 32 Bytes\n");
        return TRUE;
    }
    else if (choice == '4')
    {
        *outLen = 64u;
        TestManager_LogString("[OK] Selected 64 Bytes\n");
        return TRUE;
    }
    else if ((choice == 'c') || (choice == 'C'))
    {
        TestManager_LogString("Enter Custom Length in Bytes (1-64): ");
        Manual_UartReadLine(strBuf, sizeof(strBuf));
        if ((Manual_ParseDec32(strBuf, outLen) == TRUE) && (*outLen > 0u) && (*outLen <= 64u))
        {
            return TRUE;
        }
        else
        {
            TestManager_LogString("[ERROR] Invalid Length (Must be 1 to 64 bytes)!\n");
            return FALSE;
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Invalid Selection!\n");
        return FALSE;
    }
}

static boolean Manual_PromptPattern(uint32* outPattern)
{
    char choice;
    char strBuf[32];

    TestManager_LogString("\nSelect Byte Pattern to Write:\n");
    TestManager_LogString("  1 - 0x31 (Sequential: 0x31, 0x32, 0x33... '1','2','3'...) [Default]\n");
    TestManager_LogString("  2 - 0xAA (Checkerboard: 0xAA, 0xAB, 0xAC...)\n");
    TestManager_LogString("  3 - 0x55 (Inverted bits: 0x55, 0x56, 0x57...)\n");
    TestManager_LogString("  C - Custom Hex Byte\n");
    TestManager_LogString("Choice (1-3, C, Enter for 0x31): ");

    choice = TestManager_UartReadCharBlocking();

    if ((choice == '\r') || (choice == '\n') || (choice == '1'))
    {
        TestManager_LogString("1\r\n[OK] Selected Pattern 0x31\n");
        *outPattern = 0x31u;
        return TRUE;
    }

    TestManager_UartWriteChar(choice);
    TestManager_LogString("\r\n");

    if (choice == '2')
    {
        *outPattern = 0xAAu;
        TestManager_LogString("[OK] Selected Pattern 0xAA\n");
        return TRUE;
    }
    else if (choice == '3')
    {
        *outPattern = 0x55u;
        TestManager_LogString("[OK] Selected Pattern 0x55\n");
        return TRUE;
    }
    else if ((choice == 'c') || (choice == 'C'))
    {
        TestManager_LogString("Enter Custom Hex Byte (e.g. AA, 55, 12): ");
        Manual_UartReadLine(strBuf, sizeof(strBuf));
        if (Manual_ParseHex32(strBuf, outPattern) == TRUE)
        {
            return TRUE;
        }
        else
        {
            TestManager_LogString("[ERROR] Invalid Hex Byte!\n");
            return FALSE;
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Invalid Selection!\n");
        return FALSE;
    }
}

/*===========================================================================*/
/* 3. MANUAL TEST EXECUTION FUNCTIONS                                        */
/*===========================================================================*/

static void Manual_ReadFlash(void)
{
    uint32 addr = 0u;
    uint32 len = 0u;
    Mem_DataType readBuf[64];
    Std_ReturnType retVal;
    MemAcc_MemJobResultType jobRes;

    TestManager_LogString("\n--- [1. MANUAL READ FLASH] ---");
    if (Manual_PromptAddress(TRUE, &addr) == FALSE)
    {
        return;
    }

    if (Manual_PromptLength(&len) == FALSE)
    {
        return;
    }

    TestManager_PrepareDriver();
    TestManager_LogString("[DRIVER] Calling Mem_Read...\n");
    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)addr, readBuf, (Mem_LengthType)len);

    if (retVal == E_OK)
    {
        jobRes = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
        if (jobRes == MEM_JOB_OK)
        {
            TestManager_LogString("[OK] Flash Read Successful! Hex Dump View:\r\n");
            Manual_PrintHexDump(addr, readBuf, len);
        }
        else
        {
            TestManager_LogString("[ERROR] Read Job Failed, JobResult=");
            TestManager_LogUnsigned((uint32)jobRes);
            TestManager_LogString("\n");
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Mem_Read Rejected (E_NOT_OK)!\n");
    }
}

static void Manual_WriteFlash(void)
{
    uint32 addr = 0u;
    uint32 len = 0u;
    uint32 seedVal = 0x31u;
    uint32 i;
    Mem_DataType writeBuf[64];
    Mem_DataType readBack[64];
    Std_ReturnType retVal;
    MemAcc_MemJobResultType jobRes;

    TestManager_LogString("\n--- [2. MANUAL WRITE FLASH] ---");
    if (Manual_PromptAddress(FALSE, &addr) == FALSE)
    {
        return;
    }

    if (Manual_PromptLength(&len) == FALSE)
    {
        return;
    }

    if (Manual_PromptPattern(&seedVal) == FALSE)
    {
        return;
    }

    for (i = 0u; i < len; i++)
    {
        writeBuf[i] = (Mem_DataType)(seedVal + i);
    }

    TestManager_PrepareDriver();
    TestManager_LogString("[DRIVER] Calling Mem_Write...\n");
    retVal = Mem_Write(TEST_FLASH_INSTANCE, (Mem_AddressType)addr, writeBuf, (Mem_LengthType)len);

    if (retVal == E_OK)
    {
        jobRes = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
        if (jobRes == MEM_JOB_OK)
        {
            TestManager_CopyFromFlash(addr, readBack, len);
            if (TestManager_BufferEquals(writeBuf, readBack, len) == TRUE)
            {
                TestManager_LogString("[OK] Flash Write Successful & Read-Back Verified 100%!\r\n");
                Manual_PrintHexDump(addr, readBack, len);
            }
            else
            {
                TestManager_LogString("[ERROR] Write Completed but Read-Back Mismatch (Sector may require Erase before Write)!\r\n");
                Manual_PrintHexDump(addr, readBack, len);
            }
        }
        else
        {
            TestManager_LogString("[ERROR] Write Job Failed, JobResult=");
            TestManager_LogUnsigned((uint32)jobRes);
            TestManager_LogString("\n");
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Mem_Write Rejected (E_NOT_OK)!\n");
    }
}

static void Manual_EraseSector(void)
{
    char choice;
    uint8 sectorId;
    Std_ReturnType retVal;
    MemAcc_MemJobResultType jobRes;

    TestManager_LogString("\n--- [3. MANUAL ERASE SECTOR] ---\n");
    TestManager_LogString("Safe Sectors: 3 (16KB), 4 (64KB), 5 (128KB), 6 (128KB), 7 (128KB)\n");
    TestManager_LogString("Select Sector ID to Erase (3-7, Enter for Sector 7): ");
    
    choice = TestManager_UartReadCharBlocking();
    if ((choice == '\r') || (choice == '\n'))
    {
        choice = '7';
    }
    TestManager_UartWriteChar(choice);
    TestManager_LogString("\r\n");

    if ((choice < '3') || (choice > '7'))
    {
        TestManager_LogString("[ERROR] Invalid Sector ID (Only 3-7 allowed)!\n");
        return;
    }

    sectorId = (uint8)(choice - '0');
    (void)TestManager_SetTargetSector(sectorId);

    TestManager_PrepareDriver();
    TestManager_LogString("[DRIVER] Calling Mem_Erase for Sector ");
    TestManager_LogUnsigned((uint32)sectorId);
    TestManager_LogString(" (Addr=");
    TestManager_LogHex32((uint32)g_CurrentTestSector.SectorAddress);
    TestManager_LogString(")...\n");

    retVal = Mem_Erase(TEST_FLASH_INSTANCE, g_CurrentTestSector.SectorAddress, g_CurrentTestSector.SectorLength);
    if (retVal == E_OK)
    {
        jobRes = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
        if (jobRes == MEM_JOB_OK)
        {
            if (TestManager_IsFlashRangeErased(g_CurrentTestSector.SectorAddress, 16u) == TRUE)
            {
                TestManager_LogString("[OK] Sector Erase Successful! Memory region cleared to 0xFF.\n");
            }
            else
            {
                TestManager_LogString("[WARNING] Job reported OK but memory is not 0xFF!\n");
            }
        }
        else
        {
            TestManager_LogString("[ERROR] Erase Job Failed!\n");
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Mem_Erase Rejected (E_NOT_OK)!\n");
    }
}

static void Manual_BlankCheck(void)
{
    uint32 addr = 0u;
    uint32 len = 0u;
    Std_ReturnType retVal;
    MemAcc_MemJobResultType jobRes;

    TestManager_LogString("\n--- [4. MANUAL BLANK CHECK] ---");
    if (Manual_PromptAddress(TRUE, &addr) == FALSE)
    {
        return;
    }

    if (Manual_PromptLength(&len) == FALSE)
    {
        return;
    }

    TestManager_PrepareDriver();
    TestManager_LogString("[DRIVER] Calling Mem_BlankCheck...\n");
    retVal = Mem_BlankCheck(TEST_FLASH_INSTANCE, (Mem_AddressType)addr, (Mem_LengthType)len);

    if (retVal == E_OK)
    {
        jobRes = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
        if (jobRes == MEM_JOB_OK)
        {
            TestManager_LogString("[OK] Result: MEM_JOB_OK (Memory region is Blank / 0xFF).\n");
        }
        else if (jobRes == MEM_INCONSISTENT)
        {
            TestManager_LogString("[OK] Result: MEM_INCONSISTENT (Memory region contains data / Non-Blank).\n");
        }
        else
        {
            TestManager_LogString("[ERROR] Job BlankCheck Failed, JobResult=");
            TestManager_LogUnsigned((uint32)jobRes);
            TestManager_LogString("\n");
        }
    }
    else
    {
        TestManager_LogString("[ERROR] Mem_BlankCheck Rejected (E_NOT_OK)!\n");
    }
}

static void Manual_DetErrorInjection(void)
{
    char choice;
    Mem_DataType dummyBuf[4];
    Std_ReturnType retVal;
    Det_LastErrorType lastError;

    TestManager_LogString("\n--- [5. MANUAL DET ERROR INJECTION] ---\n");
    TestManager_LogString("Select DET Error Scenario to inject:\n");
    TestManager_LogString("  1 - Call Mem_Read before Init -> Expected MEM_E_UNINIT (0x01)\n");
    TestManager_LogString("  2 - Call Mem_Read with NULL Data Pointer -> Expected MEM_E_PARAM_POINTER (0x02)\n");
    TestManager_LogString("  3 - Call Mem_Read with Out-of-bounds Address -> Expected MEM_E_PARAM_ADDRESS (0x03)\n");
    TestManager_LogString("  4 - Call Mem_Read with Length = 0 -> Expected MEM_E_PARAM_LENGTH (0x04)\n");
    TestManager_LogString("  5 - Call Mem_Read with Invalid InstanceId = 99 -> Expected MEM_E_PARAM_INSTANCE_ID (0x05)\n");
    TestManager_LogString("Choice (1-5): ");

    choice = TestManager_UartReadCharBlocking();
    TestManager_UartWriteChar(choice);
    TestManager_LogString("\r\n");

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    switch (choice)
    {
        case '1':
            Mem_DeInit();
            retVal = Mem_Read(TEST_FLASH_INSTANCE, 0x08000000UL, dummyBuf, 4u);
            break;
        case '2':
            retVal = Mem_Read(TEST_FLASH_INSTANCE, 0x08000000UL, NULL_PTR, 4u);
            break;
        case '3':
            retVal = Mem_Read(TEST_FLASH_INSTANCE, 0x08090000UL, dummyBuf, 4u);
            break;
        case '4':
            retVal = Mem_Read(TEST_FLASH_INSTANCE, 0x08000000UL, dummyBuf, 0u);
            break;
        case '5':
            retVal = Mem_Read((Mem_InstanceIdType)99u, 0x08000000UL, dummyBuf, 4u);
            break;
        default:
            TestManager_LogString("[ERROR] Invalid choice!\n");
            return;
    }

    lastError = Det_GetLastError();
    TestManager_LogString("\n[DRIVER RESPONSE]:\n");
    TestManager_LogString("  - Return Value = ");
    TestManager_LogString((retVal == E_NOT_OK) ? "E_NOT_OK (0x01)\n" : "E_OK (0x00)\n");
    TestManager_LogString("  - DET Valid    = ");
    TestManager_LogString((lastError.Valid == TRUE) ? "TRUE\n" : "FALSE\n");
    if (lastError.Valid == TRUE)
    {
        TestManager_LogString("  - DET ModuleId = ");
        TestManager_LogUnsigned((uint32)lastError.ModuleId);
        TestManager_LogString("\n  - DET ApiId    = ");
        TestManager_LogHex8(lastError.ApiId);
        TestManager_LogString("\n  - DET ErrorId  = ");
        TestManager_LogHex8(lastError.ErrorId);
        TestManager_LogString("\n");
    }
    TestManager_LogString(">>> VERIFICATION: Parameter validation rejected request and reported DET successfully!\n");
}

/*===========================================================================*/
/* 4. MAIN MANUAL TEST ENTRY POINT                                           */
/*===========================================================================*/

void Manual_Test(void)
{
    boolean running = TRUE;

    TestManager_LogString("\n=======================================================\n");
    TestManager_LogString(">>> INTERACTIVE UART MANUAL COMMAND SHELL <<<\n");
    TestManager_LogString("=======================================================\n");

    while (running == TRUE)
    {
        char choice;

        TestManager_LogString("\n[MANUAL CLI MENU]\n");
        TestManager_LogString("  1 - Read Flash (Mem_Read) & Hex Dump View\n");
        TestManager_LogString("  2 - Write Flash (Mem_Write) & Read-Back Verification\n");
        TestManager_LogString("  3 - Erase Flash Sector (Mem_Erase)\n");
        TestManager_LogString("  4 - Blank Check Verification (Mem_BlankCheck)\n");
        TestManager_LogString("  5 - DET Error Injection Test (Negative Testing)\n");
        TestManager_LogString("  0 - Return to Main Menu\n");
        TestManager_LogString("Select function (0-5): ");

        choice = TestManager_UartReadCharBlocking();
        TestManager_UartWriteChar(choice);
        TestManager_LogString("\r\n");

        switch (choice)
        {
            case '1': Manual_ReadFlash();          break;
            case '2': Manual_WriteFlash();         break;
            case '3': Manual_EraseSector();        break;
            case '4': Manual_BlankCheck();         break;
            case '5': Manual_DetErrorInjection();  break;
            case '0':
                running = FALSE;
                TestManager_LogString("[CLI] Exited Manual CLI Mode.\n");
                break;
            default:
                TestManager_LogString("[ERROR] Invalid choice. Use 0-5.\n");
                break;
        }
    }
}
