/******************************************************************************
 * FILE: Det_Test.c
 * MÔ TẢ: Test DET cho AUTOSAR Mem driver
 ******************************************************************************/

#include "Det_Test.h"
#include "TestManager.h"

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: section 7.3.1, p.23; [SWS_Mem_00052], p.25
 * - Kiểm tra suy ra: API được gọi trước khởi tạo phải bị từ chối với MEM_E_UNINIT.
 */
static void DET_001(void)
{
    Mem_DataType    buffer[4];
    Std_ReturnType  retVal;

    TestManager_PrepareDriver();
    Mem_DeInit();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0201u, "ReadBeforeInit",
                                "Section7.3.1,SWS_Mem_00052",
                                "p.23,25",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_READ, MEM_E_UNINIT) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00005], p.36
 * - Kiểm tra Mem_Read() từ chối destination buffer NULL với MEM_E_PARAM_POINTER.
 */
static void DET_002(void)
{
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, (Mem_AddressType)FLASH_IP_BASE_ADDRESS, NULL_PTR, 4u);

    TestManager_RecordCaseTrace(0x0202u, "ReadNullPointer",
                                "SWS_Mem_10012,SWS_Mem_00005",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) && (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_POINTER) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

/* Truy vết:
 * - AUTOSAR_CP_SWS_MemoryDriver: [SWS_Mem_10012], [SWS_Mem_00006], p.36
 * - Kiểm tra Mem_Read() từ chối địa chỉ không hợp lệ với MEM_E_PARAM_ADDRESS.
 */
static void DET_003(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE, TEST_FLASH_INVALID_ADDRESS, buffer, (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0203u, "ReadInvalidAddress",
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
static void DET_004(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)0u);

    TestManager_RecordCaseTrace(0x0204u, "ReadInvalidLength",
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
static void DET_005(void)
{
    Mem_DataType   buffer[4];
    Std_ReturnType retVal;

    TestManager_PrepareDriver();
    TestManager_ResetDetState();

    retVal = Mem_Read(TEST_FLASH_INVALID_INSTANCE,
                      (Mem_AddressType)FLASH_IP_BASE_ADDRESS,
                      buffer,
                      (Mem_LengthType)sizeof(buffer));

    TestManager_RecordCaseTrace(0x0205u, "ReadInvalidInstance",
                                "SWS_Mem_10012,SWS_Mem_00004",
                                "p.36",
                                (boolean)((retVal == E_NOT_OK) &&
                                          (TestManager_CheckDet(MEM_SID_READ, MEM_E_PARAM_INSTANCE_ID) == TRUE)),
                                (uint32)E_NOT_OK,
                                (uint32)retVal);
}

void Det_Test(void)
{
    TestManager_BeginGroup(TEST_DET);

    DET_001();
    DET_002();
    DET_003();
    DET_004();
    DET_005();

    TestManager_EndGroup();
}
