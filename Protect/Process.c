#include "Protect.h"

VOID
CreateProcessNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    if (CreateInfo == NULL)
        return;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "CreateProcessNotifyRoutine: Entering with %wZ\n", CreateInfo->ImageFileName);

    UNREFERENCED_PARAMETER(Process);

    KeAcquireGuardedMutex(&ProcessWatchListMutex);

    if (IsListEmpty(&ProcessWatchList))
        goto ReleaseMutex;

    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;
    while (CurrEntry != NULL)
    {
        PWCHAR CurrName = CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name;
        if (wcscmp(CreateInfo->ImageFileName->Buffer, CurrName) == 0)
        {
            PWATCH_PID_ENTRY CurrPidEntry = ExAllocatePoolWithTag(PagedPool, sizeof(WATCH_PID_ENTRY), PID_POOL_TAG);
            
            if (CurrPidEntry == NULL)
                goto ReleaseMutex;

            CurrPidEntry->ProcessId = ProcessId;
            KeAcquireGuardedMutex(&PidWatchListMutex);
            InsertHeadList(&PidWatchList, &CurrPidEntry->List);
            KeReleaseGuardedMutex(&PidWatchListMutex);
        }
        CurrEntry = CurrEntry->Flink;
    }

ReleaseMutex:
    KeReleaseGuardedMutex(&ProcessWatchListMutex);
    return;
}