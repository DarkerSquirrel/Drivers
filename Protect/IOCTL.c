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
IOCTLEnumerateWatchList(
    _In_ UINT64 OutputLen,
    _Out_ PENUMERATE_PROCESS_INFO OutputBuffer
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (OutputLen < sizeof(OutputBuffer))
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    memset(OutputBuffer, 0, sizeof(ENUMERATE_PROCESS_INFO));

    KeAcquireGuardedMutex(&ProcessWatchListMutex);

    OutputBuffer->WatchCount = CurrentWatchCount;
    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;
    int CurrCount = 0;

    while (CurrEntry != NULL)
    {
        memcpy(OutputBuffer->Names[CurrCount],
            CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name,
            MAX_PATH + 1);
        CurrEntry = CurrEntry->Flink;
    }

    KeReleaseGuardedMutex(&ProcessWatchListMutex);

Exit:
    return Status;
}

NTSTATUS
IOCTLClearWatchList(
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    KeAcquireGuardedMutex(&PidWatchListMutex);

    CurrentWatchCount = 0;
    
    if (!IsListEmpty(&ProcessWatchList))
    {
        PLIST_ENTRY Removed = RemoveHeadList(&ProcessWatchList);
        PWATCH_PROCESS_ENTRY WatchProcess = CONTAINING_RECORD(Removed, WATCH_PROCESS_ENTRY, List);
        ExFreePoolWithTag(WatchProcess, LIST_POOL_TAG);
    }

    if (!IsListEmpty(&PidWatchList))
    {
        PLIST_ENTRY Removed = RemoveHeadList(&PidWatchList);
        PWATCH_PID_ENTRY WatchPid = CONTAINING_RECORD(Removed, WATCH_PID_ENTRY, List);
        ExFreePoolWithTag(WatchPid, PID_POOL_TAG);
    }

    KeReleaseGuardedMutex(&PidWatchListMutex);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);

Exit:
    return Status;
}