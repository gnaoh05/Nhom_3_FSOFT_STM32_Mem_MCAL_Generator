# AUTOSAR MEM Requirement Traceability

## Scope

- Project: `AUTOSAR_MEM`
- Target: `STM32F401RE`
- Test style: register-level on target, no HAL flash service usage
- Document basis:
  - `AUTOSAR_CP_SWS_MemoryDriver.pdf` (AUTOSAR CP R25-11, Doc ID 1018)
  - `Refenrence manual Stm32f401re.pdf`

This file tracks the requirements covered by the STM32 target test suite in `Test/Testcase`.
The suite is intended to cover all requirements that are both:

- implemented by this project, and
- meaningfully testable at runtime on the current STM32F401 internal-flash port.

Requirements that are configuration-only, compile-time-only, review-only, or not applicable to this memory technology are listed separately.

## Runtime Coverage Summary

- Runtime testcase count: `49`
- Covered by target execution:
  - API definition coverage
  - DET coverage
  - job-state coverage
  - asynchronous execution coverage
  - destructive flash write/erase/blank-check coverage
  - unsupported optional service coverage

## Testcase Inventory

- `0x0101` `JobResultAfterInit`
- `0x0102` `VersionInfoMatches`
- `0x0103` `InitRejectsNonNullConfig`
- `0x0104` `VersionInfoRejectsNullPointer`
- `0x0105` `DeInitTransitionsToUninit`
- `0x0201` `ReadBeforeInit`
- `0x0202` `ReadNullPointer`
- `0x0203` `ReadInvalidAddress`
- `0x0204` `ReadInvalidLength`
- `0x0205` `ReadInvalidInstance`
- `0x0301` `InitialJobResultOk`
- `0x0302` `AcceptedReadSetsPending`
- `0x0303` `ReadCompletesWithOk`
- `0x0304` `GetJobResultInvalidInstance`
- `0x0305` `RejectedReadKeepsJobResultOk`
- `0x0401` `ReadValidAddress`
- `0x0402` `ReadRejectsInvalidAddress`
- `0x0403` `ReadRejectsInvalidLength`
- `0x0404` `ReadRejectsInvalidInstance`
- `0x0405` `ReadRejectsPendingRequest`
- `0x0501` `WriteValidPattern`
- `0x0502` `WriteRejectsInvalidAddress`
- `0x0503` `WriteRejectsInvalidInstance`
- `0x0504` `WriteRejectsNullPointer`
- `0x0505` `WriteRejectsInvalidLength`
- `0x0506` `WriteRejectsPendingJob`
- `0x0601` `EraseValidSector`
- `0x0602` `EraseRejectsMisalignedRequest`
- `0x0603` `EraseRejectsInvalidAddress`
- `0x0604` `EraseRejectsInvalidInstance`
- `0x0605` `EraseRejectsInvalidLength`
- `0x0606` `EraseRejectsPendingJob`
- `0x0701` `BlankSectorReturnsOk`
- `0x0702` `NonBlankSectorReturnsInconsistent`
- `0x0703` `BlankCheckRejectsInvalidInstance`
- `0x0704` `BlankCheckRejectsInvalidAddress`
- `0x0705` `BlankCheckRejectsInvalidLength`
- `0x0706` `BlankCheckRejectsPendingJob`
- `0x0801` `RejectSecondPendingJob`
- `0x0802` `PropagateErrorCancelsJob`
- `0x0803` `HwSpecificServiceNotAvailable`
- `0x0804` `SuspendNotAvailable`
- `0x0805` `ResumeNotAvailable`
- `0x0806` `PropagateErrorInvalidInstance`
- `0x0807` `HwSpecificRejectsInvalidInstance`
- `0x0808` `HwSpecificRejectsNullDataPtr`
- `0x0809` `HwSpecificRejectsNullLengthPtr`
- `0x0810` `SuspendRejectsInvalidInstance`
- `0x0811` `ResumeRejectsInvalidInstance`

## Requirement Coverage Matrix

| Requirement | Coverage | Testcases | Notes |
| --- | --- | --- | --- |
| `SWS_Mem_10008` | Runtime | `0x0101`, `0x0103` | `Mem_Init()` API behavior |
| `SWS_Mem_00001` | Runtime | `0x0101`, `0x0301` | Initial job result becomes `MEM_JOB_OK` |
| `SWS_Mem_00087` | Runtime | `0x0103` | `configPtr` must be `NULL_PTR` |
| `SWS_Mem_10009` | Runtime | `0x0102`, `0x0104` | `Mem_GetVersionInfo()` API behavior |
| `SWS_Mem_00002` | Runtime | `0x0104` | `Mem_GetVersionInfo(NULL_PTR)` DET |
| `SWS_Mem_10018` | Runtime | `0x0105` | `Mem_DeInit()` API behavior |
| `SWS_Mem_00079` | Runtime plus review | `0x0105` | Runtime verifies de-initialized state; full cancellation of in-flight flash operations remains hardware-limited on STM32F401 single-bank flash |
| `Section7.3.1` | Runtime | `0x0105`, `0x0201` | APIs other than allowed exceptions require initialization |
| `SWS_Mem_00052` | Runtime | `0x0105`, `0x0201`, `0x0202`, `0x0203`, `0x0204`, `0x0205`, `0x0304`, `0x0402`, `0x0403`, `0x0404`, `0x0502`, `0x0503`, `0x0504`, `0x0505`, `0x0602`, `0x0603`, `0x0604`, `0x0605`, `0x0703`, `0x0704`, `0x0705`, `0x0806`, `0x0807`, `0x0808`, `0x0809`, `0x0810`, `0x0811` | Development-error classification exercised by target tests |
| `SWS_Mem_10011` | Runtime | `0x0301`, `0x0304` | `Mem_GetJobResult()` API behavior |
| `SWS_Mem_00029` | Runtime | `0x0301`, `0x0302`, `0x0305` | Driver tracks current job result |
| `SWS_Mem_00090` | Runtime | `0x0304` | Invalid instance ID in `Mem_GetJobResult()` |
| `SWS_Mem_00059` | Runtime | `0x0305`, `0x0801` | Unprocessable job requests are rejected with `E_NOT_OK` |
| `SWS_Mem_10012` | Runtime | `0x0202`, `0x0203`, `0x0204`, `0x0205`, `0x0302`, `0x0303`, `0x0305`, `0x0401`, `0x0402`, `0x0403`, `0x0404`, `0x0405`, `0x0801` | `Mem_Read()` API behavior |
| `SWS_Mem_00004` | Runtime | `0x0205`, `0x0404` | Invalid instance ID in `Mem_Read()` |
| `SWS_Mem_00005` | Runtime | `0x0202` | `Mem_Read(NULL_PTR)` DET |
| `SWS_Mem_00006` | Runtime | `0x0203`, `0x0402` | Invalid address in `Mem_Read()` |
| `SWS_Mem_00072` | Runtime | `0x0204`, `0x0305`, `0x0403` | Invalid length in `Mem_Read()` |
| `SWS_Mem_00007` | Runtime | `0x0405`, `0x0801` | `Mem_Read()` while previous job is pending |
| `SWS_Mem_00030` | Runtime | `0x0302` | Accepted request changes state to `MEM_JOB_PENDING` |
| `SWS_Mem_10010` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | `Mem_MainFunction()` scheduled processing |
| `SWS_Mem_00066` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | Asynchronous services are executed in `Mem_MainFunction()` |
| `SWS_Mem_00067` | Runtime | `0x0303`, `0x0401`, `0x0501`, `0x0601`, `0x0701` | Successful jobs finish with `MEM_JOB_OK` |
| `SWS_Mem_10013` | Runtime | `0x0501`, `0x0502`, `0x0503`, `0x0504`, `0x0505`, `0x0506` | `Mem_Write()` API behavior |
| `SWS_Mem_00009` | Runtime | `0x0503` | Invalid instance ID in `Mem_Write()` |
| `SWS_Mem_00010` | Runtime | `0x0504` | `Mem_Write(NULL_PTR)` DET |
| `SWS_Mem_00011` | Runtime | `0x0502` | Invalid address in `Mem_Write()` |
| `SWS_Mem_00012` | Runtime | `0x0505` | Invalid length in `Mem_Write()` |
| `SWS_Mem_00013` | Runtime | `0x0506` | `Mem_Write()` while previous job is pending |
| `SWS_Mem_10014` | Runtime | `0x0601`, `0x0602`, `0x0603`, `0x0604`, `0x0605`, `0x0606` | `Mem_Erase()` API behavior |
| `SWS_Mem_00015` | Runtime | `0x0604` | Invalid instance ID in `Mem_Erase()` |
| `SWS_Mem_00016` | Runtime | `0x0602`, `0x0603` | Invalid address in `Mem_Erase()` |
| `SWS_Mem_00017` | Runtime | `0x0605` | Invalid length in `Mem_Erase()` |
| `SWS_Mem_00018` | Runtime | `0x0606` | `Mem_Erase()` while previous job is pending |
| `SWS_Mem_00035` | Runtime | `0x0602` | No erase alignment adjustment; misaligned sector request is rejected |
| `SWS_Mem_10016` | Runtime | `0x0701`, `0x0702`, `0x0703`, `0x0704`, `0x0705`, `0x0706` | `Mem_BlankCheck()` API behavior |
| `SWS_Mem_00022` | Runtime | `0x0703` | Invalid instance ID in `Mem_BlankCheck()` |
| `SWS_Mem_00023` | Runtime | `0x0704` | Invalid address in `Mem_BlankCheck()` |
| `SWS_Mem_00024` | Runtime | `0x0705` | Invalid length in `Mem_BlankCheck()` |
| `SWS_Mem_00025` | Runtime | `0x0706` | `Mem_BlankCheck()` while previous job is pending |
| `SWS_Mem_00076` | Runtime | `0x0702` | Completed job with unexpected result reports `MEM_INCONSISTENT` |
| `SWS_Mem_00057` | Runtime | `0x0801` | One job at a time per driver instance |
| `SWS_Mem_10015` | Runtime | `0x0802`, `0x0806` | `Mem_PropagateError()` API behavior |
| `SWS_Mem_00061` | Runtime | `0x0802` | `Mem_PropagateError()` sets `MEM_ECC_UNCORRECTED` and cancels the current job |
| `SWS_Mem_00020` | Runtime | `0x0806` | Invalid instance ID in `Mem_PropagateError()` |
| `SWS_Mem_10017` | Runtime | `0x0803`, `0x0807`, `0x0808`, `0x0809` | `Mem_HwSpecificService()` API behavior |
| `SWS_Mem_00070` | Runtime | `0x0803` | Unsupported optional service returns `E_MEM_SERVICE_NOT_AVAIL` |
| `SWS_Mem_00026` | Runtime | `0x0807` | Invalid instance ID in `Mem_HwSpecificService()` |
| `SWS_Mem_00027` | Runtime | `0x0808`, `0x0809` | Null pointer handling in `Mem_HwSpecificService()` |
| `SWS_Mem_10024` | Runtime | `0x0804`, `0x0810` | `Mem_Suspend()` API behavior |
| `SWS_Mem_00082` | Runtime | `0x0804`, `0x0805` | Unsupported suspend/resume returns `E_MEM_SERVICE_NOT_AVAIL` |
| `SWS_Mem_00091` | Runtime | `0x0810` | Invalid instance ID in `Mem_Suspend()` |
| `SWS_Mem_10025` | Runtime | `0x0805`, `0x0811` | `Mem_Resume()` API behavior |
| `SWS_Mem_00092` | Runtime | `0x0811` | Invalid instance ID in `Mem_Resume()` |
| `SWS_Mem_00088` | Runtime plus review | `0x0501`, `0x0601`, `0x0701`, `0x0702` | Runtime tests confirm upper-layer read-back and blank-check strategy; proving the driver does not self-verify is an inspection activity |

## Review-Only Or Not-Applicable Requirements

| Requirement | Status | Reason |
| --- | --- | --- |
| `SWS_Mem_00031` | Review only | A deterministic target-runtime fault injection path for “accepted request later becomes hardware-failed” is not implemented in this STM32F401 reference test suite. |
| `SWS_Mem_00080` | Not applicable on this port | STM32F401 internal flash suspend is not supported by this reference driver; `SWS_Mem_00082` governs the observable runtime behavior. |
| `SWS_Mem_00081` | Not applicable on this port | STM32F401 internal flash resume is not supported by this reference driver; `SWS_Mem_00082` governs the observable runtime behavior. |
| `SWS_Mem_00083` | Not applicable on this port | Requirement only applies when suspend support exists. |
| `SWS_Mem_00084` | Not applicable on this port | Requirement only applies when resume support exists. |

## STM32F401 Execution Notes

- Destructive flash tests are restricted to one reserved sector:
  - `TEST_FLASH_SECTOR_ADDRESS = 0x08060000`
  - `TEST_FLASH_SECTOR_LENGTH = 0x00020000`
- The framework checks that the linked image end stays below that sector before write/erase/blank-check tests are allowed.
- Non-destructive DET and pending-job tests deliberately avoid calling `Mem_MainFunction()` after job acceptance, so they can verify queued-job behavior without touching flash contents.
