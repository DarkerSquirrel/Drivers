#include "Protect.h"
#include "UserKernelBridge.h"
#include "IOCTL.h"

NTSTATUS
IOCTLAddProcessToWatchList(
    _In_ PPROTECT_INPUT pInput
)
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    PWATCH_PROCESS_ENTRY CurrentEntry = (PWATCH_PROCESS_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(WATCH_PROCESS_ENTRY), LIST_POOL_TAG);

    if (CurrentEntry == NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTLAddProcessToWatch: ExAllocatePool failed");
        return;
    }

    size_t InputLength = wcslen(pInput->Name);
    if (InputLength > 0 && InputLength <= MAX_PATH)
    {
        memcpy(CurrentEntry->Name, pInput->Name, InputLength);
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "IOCTLAddProcessToWatch: Input Name => %ls, Input Size => %llu",
            pInput->Name, InputLength);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    InsertTailList(&ProcessWatchList, &CurrentEntry->List);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);

Exit:
    if (!NT_SUCCESS(Status))
    {
        if (CurrentEntry != NULL)
            ExFreePoolWithTag(CurrentEntry, LIST_POOL_TAG);
    }

    return Status;
}

NTSTATUS
IOCTLEnumerateProtectedProcesses(
    _In_ UINT64 OutputLen,
    _Out_ PENUMERATE_PROCESS_INFO OutputBuffer
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (OutputLen < sizeof(PENUMERATE_PROCESS_INFO))
    return Status;
}