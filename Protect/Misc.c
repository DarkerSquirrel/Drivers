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