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

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    KeAcquireGuardedMutex(&PidWatchList);
    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;

    while (CurrEntry != NULL)
    {
        PWCHAR CurrName = CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name;
        if (wcscmp(CreateInfo->ImageFileName, CurrName) == 0)
        {
            WATCH_PID_ENTRY CurrPidEntry;
            CurrPidEntry.ProcessId = ProcessId;
            InsertHeadList(&PidWatchList, &CurrPidEntry.List);
        }
        CurrEntry = CurrEntry->Flink;
    }
    KeReleaseGuardedMutex(&PidWatchList);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);
}