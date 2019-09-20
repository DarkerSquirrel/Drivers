#include "Protect.h"

#include "Misc.h"

VOID 
ClearWatchList(
)
{
    PAGED_CODE();

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    KeAcquireGuardedMutex(&PidWatchListMutex);

    CurrentWatchCount = 0;

    while (!IsListEmpty(&ProcessWatchList))
    {
        PLIST_ENTRY Removed = RemoveHeadList(&ProcessWatchList);
        PWATCH_PROCESS_ENTRY WatchProcess = CONTAINING_RECORD(Removed, WATCH_PROCESS_ENTRY, List);
        ExFreePoolWithTag(WatchProcess, LIST_POOL_TAG);
    }

    while (!IsListEmpty(&PidWatchList))
    {
        PLIST_ENTRY Removed = RemoveHeadList(&PidWatchList);
        PWATCH_PID_ENTRY WatchPid = CONTAINING_RECORD(Removed, WATCH_PID_ENTRY, List);
        ExFreePoolWithTag(WatchPid, PID_POOL_TAG);
    }

    KeReleaseGuardedMutex(&PidWatchListMutex);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);
}

VOID
RemovePidFromWatchList(
    _In_ HANDLE ProcessId
)
{
    PAGED_CODE();

    KeAcquireGuardedMutex(&PidWatchListMutex);
    if (IsListEmpty(&PidWatchList))
        goto ReleaseMutex;

    PLIST_ENTRY CurrEntry = PidWatchList.Flink;

    while (CurrEntry != &PidWatchList)
    {
        HANDLE CurrProcId = CONTAINING_RECORD(CurrEntry, WATCH_PID_ENTRY, List)->ProcessId;
        if (CurrProcId == ProcessId)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, 
                "Removing PID: %p from watch list\n", ProcessId);
            RemoveEntryList(CurrEntry);
            break;
        }
        CurrEntry = CurrEntry->Flink;
    }

ReleaseMutex:
    KeReleaseGuardedMutex(&PidWatchListMutex);
}