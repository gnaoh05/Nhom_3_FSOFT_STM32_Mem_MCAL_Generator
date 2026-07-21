/**********************************************************************************************************************
 *  FILE:         Mem_IPW.c
 *  MODULE:       Mem_IPW (Memory Driver - IP Wrapper Layer)
 *  DESCRIPTION:  Implementation of the IP-Wrapper layer for the STM32F401RE. Delegates every hardware
 *                operation to Flash_IP (Flash_IP.h/.c), the bare-metal register-level driver for the
 *                internal Flash memory. This keeps Mem.c fully hardware-agnostic per [SWS_Mem_00035], and
 *                confines all STM32F401RE specifics (registers, sector geometry, IRQ handling) to Flash_IP.
 *
 *                Only Mem driver instance 0 exists (see Mem_Cfg.h - MEM_INSTANCE_COUNT == 1), mapped 1:1
 *                to the single internal Flash bank of the STM32F401RE.
 *
 *                Job result bookkeeping:
 *                Flash_IP only tracks the status of Program/Erase (the operations it runs asynchronously
 *                via interrupt). Read and BlankCheck complete synchronously within this layer (Flash is
 *                memory-mapped), so their result is computed immediately and stored locally rather than
 *                read back from Flash_IP_GetStatus(). Mem_Ipw_GetJobResult() picks the right source based
 *                on which kind of operation was triggered last.
 *********************************************************************************************************************/

#include "Mem_IPW.h"
#include "Flash_IP.h"

/*======================================================================================================================
 *  [SWS_Mem_00033] static configuration check / [SWS_Mem_00060] multi-instance note
 *  Flash_IP.c models exactly one physical device: the single internal Flash bank of the STM32F401RE (one
 *  FLASH_CR/FLASH_SR register set, one set of module-static state variables). [SWS_Mem_00060] requires
 *  the Mem driver to "support multiple instances of the same memory device" - that requirement is
 *  satisfiable only for EXTERNAL memory devices of which several identical physical chips can exist (e.g.
 *  two identical SPI Flash ICs), never for a single MCU's own internal Flash controller, which is a
 *  hardware singleton by construction. If MEM_INSTANCE_COUNT were silently left >1 while every instance
 *  is still backed by this same Flash_IP, both instances would alias the same physical memory and corrupt
 *  each other's jobs. Fail at compile time instead of at runtime. To genuinely add a second Mem driver
 *  instance, back it with a DIFFERENT Mem_IPW/xxx_IP pair (e.g. an external SPI Flash driver), not with a
 *  second copy of this one. */
#if (MEM_INSTANCE_COUNT > 1u)
#error "Mem_IPW.c backs every Mem driver instance with the single STM32F401RE internal Flash bank (Flash_IP.c). MEM_INSTANCE_COUNT must be 1 - see the SWS_Mem_00060 note in Mem_IPW.c."
#endif

/*======================================================================================================================
 *  Local state
 *====================================================================================================================*/
typedef enum
{
    MEM_IPW_RESULT_SRC_NONE = 0,
    MEM_IPW_RESULT_SRC_FLASH_IP,  /* result comes from Flash_IP_GetStatus() - Program / Erase (async)   */
    MEM_IPW_RESULT_SRC_LOCAL      /* result already computed synchronously - Read / BlankCheck           */
} Mem_Ipw_ResultSourceType;

static Mem_Ipw_ResultSourceType Mem_Ipw_ResultSource[MEM_INSTANCE_COUNT];
static MemAcc_MemJobResultType  Mem_Ipw_LocalResult[MEM_INSTANCE_COUNT];

/*======================================================================================================================
 *  Local helpers
 *====================================================================================================================*/

/* This IPW currently supports exactly one instance (instanceId == 0, the internal Flash).
 * Kept as a function (rather than a macro) so it is easy to extend if additional Mem driver
 * instances (e.g. an external SPI Flash Mem_IPW/Flash_IP pair) are added later. */
static boolean Mem_Ipw_IsInstanceSupported(Mem_InstanceIdType instanceId)
{
    return (boolean)(instanceId < (Mem_InstanceIdType)MEM_INSTANCE_COUNT);
}

static MemAcc_MemJobResultType Mem_Ipw_MapFlashIpStatus(Flash_IP_StatusType status)
{
    MemAcc_MemJobResultType result;

    switch (status)
    {
        case FLASH_IP_BUSY:
            result = MEM_JOB_PENDING;
            break;
        case FLASH_IP_OK:
            result = MEM_JOB_OK;
            break;
        case FLASH_IP_ERROR:
            result = MEM_JOB_FAILED;
            break;
        case FLASH_IP_IDLE:
        default:
            result = MEM_JOB_OK;
            break;
    }

    return result;
}

/*======================================================================================================================
 *  Lifecycle
 *====================================================================================================================*/

void Mem_Ipw_Init(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_Init();

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_NONE;
        Mem_Ipw_LocalResult[instanceId]  = MEM_JOB_OK;
    }
}

void Mem_Ipw_DeInit(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_DeInit(); /* [SWS_Mem_00079] */

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_NONE;
    }
}

/*======================================================================================================================
 *  Asynchronous memory operations
 *====================================================================================================================*/

Std_ReturnType Mem_Ipw_Read(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     sourceAddress,
        Mem_DataType*       destinationDataPtr,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* Flash is memory-mapped: the read completes synchronously. The result is nevertheless only
         * consumed by Mem.c/Mem_MainFunction() on the following cycle, consistent with the asynchronous
         * API contract of [SWS_Mem_10012]. */
        retVal = Flash_IP_Read((uint32)sourceAddress, destinationDataPtr, (uint32)length);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_LOCAL;
            Mem_Ipw_LocalResult[instanceId]  = MEM_JOB_OK; /* [SWS_Mem_00067] */
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_Write(
        Mem_InstanceIdType   instanceId,
        Mem_AddressType      targetAddress,
        const Mem_DataType*  sourceDataPtr,
        Mem_LengthType       length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        retVal = Flash_IP_ProgramStart((uint32)targetAddress, sourceDataPtr, (uint32)length);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_FLASH_IP;
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_Erase(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    /* Alignment has already been validated synchronously by Mem.c (Mem_CheckEraseAlignment(), which calls
     * Mem_Ipw_IsEraseAligned() below) before this trigger function is ever reached from Mem_MainFunction().
     * The check is repeated here defensively (cheap, and protects any other caller of this IPW layer). */
    if ((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) &&
        (Mem_Ipw_IsEraseAligned(instanceId, targetAddress, length) == TRUE))
    {
        uint8 sector = Flash_IP_GetSectorFromAddress((uint32)targetAddress);

        retVal = Flash_IP_EraseSectorStart(sector);

        if (retVal == E_OK)
        {
            Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_FLASH_IP;
        }
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_BlankCheck(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     targetAddress,
        Mem_LengthType      length)
{
    Std_ReturnType retVal = E_NOT_OK;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* Flash is memory-mapped and there is no dedicated hardware blank-check on this MCU, so the
         * check is performed synchronously by reading back and comparing against the erased value. */
        uint32  i;
        uint8   byte;
        boolean isBlank = TRUE;

        for (i = 0u; i < (uint32)length; i++)
        {
            (void)Flash_IP_Read((uint32)targetAddress + i, &byte, 1u);

            if (byte != (uint8)FLASH_IP_ERASED_VALUE)
            {
                isBlank = FALSE;
                break;
            }
        }

        Mem_Ipw_ResultSource[instanceId] = MEM_IPW_RESULT_SRC_LOCAL;
        Mem_Ipw_LocalResult[instanceId]  = (isBlank == TRUE) ? MEM_JOB_OK : MEM_INCONSISTENT; /* [SWS_Mem_00076] */

        retVal = E_OK;
    }

    return retVal;
}

Std_ReturnType Mem_Ipw_HwSpecificService(
        Mem_InstanceIdType   instanceId,
        Mem_HwServiceIdType  hwServiceId,
        Mem_DataType*        dataPtr,
        Mem_LengthType*      lengthPtr)
{
    /* No hardware specific services (e.g. read protection level, option byte access, 96-bit unique ID
     * read at 0x1FFF7A10) are implemented for this reference port, see [SWS_Mem_00070]. Add cases per
     * hwServiceId in Flash_IP.c and dispatch to them here if needed. */
    (void)instanceId;
    (void)hwServiceId;
    (void)dataPtr;
    (void)lengthPtr;

    return E_MEM_SERVICE_NOT_AVAIL;
}

/*======================================================================================================================
 *  Suspend / Resume
 *  The STM32F401 (single Flash bank) has no hardware suspend/resume mechanism for program/erase
 *  operations (that capability exists only on dual-bank / newer STM32 families), see [SWS_Mem_00082].
 *====================================================================================================================*/

Std_ReturnType Mem_Ipw_Suspend(Mem_InstanceIdType instanceId)
{
    (void)instanceId;
    return E_MEM_SERVICE_NOT_AVAIL;
}

Std_ReturnType Mem_Ipw_Resume(Mem_InstanceIdType instanceId)
{
    (void)instanceId;
    return E_MEM_SERVICE_NOT_AVAIL;
}

/*======================================================================================================================
 *  Scheduling / job result retrieval
 *====================================================================================================================*/

void Mem_Ipw_MainFunction(Mem_InstanceIdType instanceId)
{
    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        Flash_IP_MainFunction(); /* reserved - completion is interrupt driven, see Flash_IP.h */
    }
}

MemAcc_MemJobResultType Mem_Ipw_GetJobResult(Mem_InstanceIdType instanceId)
{
    MemAcc_MemJobResultType result = MEM_JOB_FAILED;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        if (Mem_Ipw_ResultSource[instanceId] == MEM_IPW_RESULT_SRC_LOCAL)
        {
            result = Mem_Ipw_LocalResult[instanceId];
        }
        else
        {
            result = Mem_Ipw_MapFlashIpStatus(Flash_IP_GetStatus());
        }
    }

    return result;
}

/*======================================================================================================================
 *  Address / length validation helpers
 *====================================================================================================================*/

boolean Mem_Ipw_IsAddressValid(Mem_InstanceIdType instanceId, Mem_AddressType address)
{
    boolean valid = FALSE;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        valid = (boolean)((address >= (Mem_AddressType)FLASH_IP_BASE_ADDRESS) &&
                           (address <  (Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE)));
    }

    return valid;
}

boolean Mem_Ipw_IsLengthValid(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length)
{
    boolean valid = FALSE;

    if ((Mem_Ipw_IsInstanceSupported(instanceId) == TRUE) && (length > 0u))
    {
        Mem_AddressType endAddress = address + (Mem_AddressType)length;

        valid = (boolean)(endAddress <= (Mem_AddressType)(FLASH_IP_BASE_ADDRESS + FLASH_IP_TOTAL_SIZE));
    }

    return valid;
}

boolean Mem_Ipw_IsEraseAligned(
        Mem_InstanceIdType instanceId,
        Mem_AddressType     address,
        Mem_LengthType      length)
{
    boolean valid = FALSE;

    if (Mem_Ipw_IsInstanceSupported(instanceId) == TRUE)
    {
        /* The STM32F401RE Flash controller can only erase whole sectors, so a valid Mem_Erase() request
         * must have an address/length pair that exactly matches one configured sector (see
         * Flash_IP_Cfg.c). MemAcc would normally be responsible for splitting a larger logical erase
         * request into per-sector calls per [SWS_Mem_00035]; since this project has no MemAcc, the
         * caller of Mem_Erase() is responsible for issuing one exactly-sector-sized request per sector. */
        uint8 sector = Flash_IP_GetSectorFromAddress((uint32)address);

        valid = (boolean)((sector != 0xFFu) &&
                           (address == (Mem_AddressType)Flash_IP_GetSectorStartAddress(sector)) &&
                           (length  == (Mem_LengthType)Flash_IP_GetSectorSize(sector)));
    }

    return valid;
}