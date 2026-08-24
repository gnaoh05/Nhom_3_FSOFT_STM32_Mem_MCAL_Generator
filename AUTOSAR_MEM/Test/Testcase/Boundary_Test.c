#include "Boundary_Test.h"
#include "TestManager.h"

#define BOUNDARY_GUARD_LEN           16u
#define BOUNDARY_TEST_PATTERN_LEN    4u

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10014], [SWS_Mem_00066], [SWS_Mem_00067], [SWS_Mem_00088]
 * - Kiểm tra tính cô lập khi XÓA bằng phương pháp Snapshot:
 *   Chụp ảnh dữ liệu hiện tại của 2 Sector hàng xóm (trước và sau) -> Xóa Sector mục tiêu ->
 *   Xác nhận Sector mục tiêu biến thành 0xFF và dữ liệu của 2 Sector hàng xóm giữ nguyên 100%.
 */
static void BOUNDARY_001(void)
{
    Mem_DataType           beforePrev[BOUNDARY_GUARD_LEN];
    Mem_DataType           afterPrev[BOUNDARY_GUARD_LEN];
    Mem_DataType           beforeNext[BOUNDARY_GUARD_LEN];
    Mem_DataType           afterNext[BOUNDARY_GUARD_LEN];
    TestNeighborBoundsType bounds;
    Std_ReturnType         retVal;
    MemAcc_MemJobResultType jobResult;
    boolean                passed = FALSE;

    TestManager_PrepareDriver();
    bounds = TestManager_GetNeighborBounds();

    /* BƯỚC 1: Chụp ảnh trạng thái hiện tại của 2 Sector lân cận (CHỈ ĐỌC) */
    if (bounds.HasPrev == TRUE)
    {
        TestManager_CopyFromFlash(bounds.PrevTailAddress, beforePrev, BOUNDARY_GUARD_LEN);
    }
    if (bounds.HasNext == TRUE)
    {
        TestManager_CopyFromFlash(bounds.NextHeadAddress, beforeNext, BOUNDARY_GUARD_LEN);
    }

    /* BƯỚC 2: Xóa duy nhất Sector mục tiêu đang được chọn */
    retVal = Mem_Erase(TEST_FLASH_INSTANCE, TEST_FLASH_SECTOR_ADDRESS, TEST_FLASH_SECTOR_LENGTH);

    if (retVal == E_OK)
    {
        jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);
        if (jobResult == MEM_JOB_OK)
        {
            boolean targetErased = TestManager_IsFlashRangeErased(TEST_FLASH_SECTOR_ADDRESS, 16u);
            boolean prevIntact   = TRUE;
            boolean nextIntact   = TRUE;

            /* BƯỚC 3: Đọc lại trạng thái của 2 Sector lân cận sau khi xóa */
            if (bounds.HasPrev == TRUE)
            {
                TestManager_CopyFromFlash(bounds.PrevTailAddress, afterPrev, BOUNDARY_GUARD_LEN);
                prevIntact = TestManager_BufferEquals(beforePrev, afterPrev, BOUNDARY_GUARD_LEN);
            }
            if (bounds.HasNext == TRUE)
            {
                TestManager_CopyFromFlash(bounds.NextHeadAddress, afterNext, BOUNDARY_GUARD_LEN);
                nextIntact = TestManager_BufferEquals(beforeNext, afterNext, BOUNDARY_GUARD_LEN);
            }

            /* BƯỚC 4: Xác nhận mục tiêu đã xóa sạch và hàng xóm không bị xóa nhầm */
            if ((targetErased == TRUE) && (prevIntact == TRUE) && (nextIntact == TRUE))
            {
                passed = TRUE;
            }
        }
    }

    TestManager_RecordCaseTrace(0x0901u, "EraseNeighborIsolation",
                                "SWS_Mem_10014,SWS_Mem_00088", "p.15,39",
                                passed, 1u, (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00066], [SWS_Mem_00067], [SWS_Mem_00088]
 * - Kiểm tra ghi mép biên dưới (Lower Boundary): Ghi vào 4 byte đầu tiên của Sector mục tiêu,
 *   xác nhận ghi thành công và không làm ảnh hưởng đến các ô nhớ lân cận.
 */
static void BOUNDARY_002(void)
{
    Mem_DataType            writeData[BOUNDARY_TEST_PATTERN_LEN] = {0xAAu, 0x55u, 0xAAu, 0x55u};
    Mem_DataType            readBack[BOUNDARY_TEST_PATTERN_LEN];
    Mem_AddressType         headAddress;
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    if (TestManager_IsFlashTestAreaSafe() == TRUE)
    {
        headAddress = TEST_FLASH_SECTOR_ADDRESS;

        retVal = Mem_Write(TEST_FLASH_INSTANCE,
                           headAddress,
                           writeData,
                           BOUNDARY_TEST_PATTERN_LEN);

        if (retVal == E_OK)
        {
            jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

            if (jobResult == MEM_JOB_OK)
            {
                TestManager_CopyFromFlash(headAddress, readBack, BOUNDARY_TEST_PATTERN_LEN);
                if (TestManager_BufferEquals(writeData, readBack, BOUNDARY_TEST_PATTERN_LEN) == TRUE)
                {
                    /* Kiểm tra 4 byte kế tiếp vẫn ở trạng thái xóa (0xFF) */
                    if (TestManager_IsFlashRangeErased((Mem_AddressType)(headAddress + 4u), 4u) == TRUE)
                    {
                        passed = TRUE;
                    }
                }
            }
        }
    }

    TestManager_RecordCaseTrace(0x0902u, "WriteLowerBoundary",
                                "SWS_Mem_10013,SWS_Mem_00066,SWS_Mem_00088", "p.15,38",
                                passed, 1u, (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00066], [SWS_Mem_00067], [SWS_Mem_00088]
 * - Kiểm tra ghi mép biên trên (Upper Boundary): Ghi vào đúng 4 byte cuối cùng của Sector mục tiêu,
 *   xác nhận ghi thành công tại mép cuối cùng.
 */
static void BOUNDARY_003(void)
{
    Mem_DataType            writeData[BOUNDARY_TEST_PATTERN_LEN] = {0x5Au, 0xA5u, 0x5Au, 0xA5u};
    Mem_DataType            readBack[BOUNDARY_TEST_PATTERN_LEN];
    Mem_AddressType         tailAddress;
    Std_ReturnType          retVal;
    MemAcc_MemJobResultType jobResult = MEM_JOB_FAILED;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    if (TestManager_IsFlashTestAreaSafe() == TRUE)
    {
        tailAddress = (Mem_AddressType)(TEST_FLASH_SECTOR_ADDRESS + TEST_FLASH_SECTOR_LENGTH - 4u);

        retVal = Mem_Write(TEST_FLASH_INSTANCE,
                           tailAddress,
                           writeData,
                           BOUNDARY_TEST_PATTERN_LEN);

        if (retVal == E_OK)
        {
            jobResult = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

            if (jobResult == MEM_JOB_OK)
            {
                TestManager_CopyFromFlash(tailAddress, readBack, BOUNDARY_TEST_PATTERN_LEN);
                if (TestManager_BufferEquals(writeData, readBack, BOUNDARY_TEST_PATTERN_LEN) == TRUE)
                {
                    passed = TRUE;
                }
            }
        }
    }

    TestManager_RecordCaseTrace(0x0903u, "WriteUpperBoundary",
                                "SWS_Mem_10013,SWS_Mem_00066,SWS_Mem_00088", "p.15,38",
                                passed, 1u, (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00066], [SWS_Mem_00067]
 * - Kiểm tra đọc chính xác tại 2 mép biên (Đầu Sector và Cuối Sector).
 */
static void BOUNDARY_004(void)
{
    Mem_DataType            readHead[4];
    Mem_DataType            readTail[4];
    Mem_DataType            refHead[4];
    Mem_DataType            refTail[4];
    Mem_AddressType         headAddress;
    Mem_AddressType         tailAddress;
    Std_ReturnType          retValHead;
    Std_ReturnType          retValTail;
    MemAcc_MemJobResultType jobResultHead;
    MemAcc_MemJobResultType jobResultTail;
    boolean                 passed = FALSE;

    TestManager_PrepareDriver();

    headAddress = TEST_FLASH_SECTOR_ADDRESS;
    tailAddress = (Mem_AddressType)(TEST_FLASH_SECTOR_ADDRESS + TEST_FLASH_SECTOR_LENGTH - 4u);

    TestManager_CopyFromFlash(headAddress, refHead, 4u);
    TestManager_CopyFromFlash(tailAddress, refTail, 4u);

    retValHead = Mem_Read(TEST_FLASH_INSTANCE, headAddress, readHead, 4u);
    jobResultHead = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

    retValTail = Mem_Read(TEST_FLASH_INSTANCE, tailAddress, readTail, 4u);
    jobResultTail = TestManager_ExecuteMainUntilDone(TEST_FLASH_INSTANCE, TEST_MAINFUNCTION_TIMEOUT);

    if ((retValHead == E_OK) && (jobResultHead == MEM_JOB_OK) &&
        (retValTail == E_OK) && (jobResultTail == MEM_JOB_OK))
    {
        if ((TestManager_BufferEquals(readHead, refHead, 4u) == TRUE) &&
            (TestManager_BufferEquals(readTail, refTail, 4u) == TRUE))
        {
            passed = TRUE;
        }
    }

    TestManager_RecordCaseTrace(0x0904u, "ReadBoundaryEdges",
                                "SWS_Mem_10012,SWS_Mem_00066,SWS_Mem_00067", "p.15,36",
                                passed, 1u, (passed == TRUE) ? 1u : 0u);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10013], [SWS_Mem_00011], [SWS_Mem_00052]
 * - Kiểm tra Mem_Write() từ chối địa chỉ nằm ngoài dải Flash hợp lệ (vượt biên bộ nhớ).
 */
static void BOUNDARY_005(void)
{
    Mem_DataType   dummyData[4] = {0x01u, 0x02u, 0x03u, 0x04u};
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Write(TEST_FLASH_INSTANCE,
                       TEST_FLASH_INVALID_ADDRESS,
                       dummyData,
                       (Mem_LengthType)sizeof(dummyData));

    TestManager_RecordCaseTrace(0x0905u, "BoundaryRejectsInvalidAddress",
                                "SWS_Mem_10013,SWS_Mem_00011,SWS_Mem_00052", "p.25,38",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_WRITE, MEM_E_PARAM_ADDRESS) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

void Boundary_Test(void)
{
    TestManager_BeginGroup(TEST_BOUNDARY);

    BOUNDARY_001();
    BOUNDARY_002();
    BOUNDARY_003();
    BOUNDARY_004();
    BOUNDARY_005();

    TestManager_EndGroup();
}
