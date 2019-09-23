#include "Protect.h"
#include "Misc.h"
#include "UserKernelBridge.h"

VOID
CreateProcessNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Process);
 
    if (CreateInfo == NULL)
    {
        RemovePidFromWatchList(ProcessId);
        return;
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "CreateProcessNotifyRoutine: Entering with %wZ\n", CreateInfo->ImageFileName);

    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;

    WCHAR ProcessPath[MAX_PATH + 1];
    PWCHAR ProcessToken;
    memset(ProcessPath, 0, MAX_PATH + 1);

    memcpy(ProcessPath, CreateInfo->ImageFileName->Buffer, CreateInfo->ImageFileName->Length);
    ProcessToken = ProcessPath;

    while (wcschr(ProcessToken, L'\\') != NULL)
    {
        ProcessToken = wcschr(ProcessToken, L'\\') + 1;
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Process name after stripping: %ls\n", ProcessToken);

    if (ProcessPath == NULL)
        goto Exit;

    KeAcquireGuardedMutex(&ProcessWatchListMutex);

    if (IsListEmpty(&ProcessWatchList))
        goto ReleaseMutex;

    while (CurrEntry != &ProcessWatchList)
    {
        PWCHAR CurrName = CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name;
        
        if (wcscmp(ProcessToken, CurrName) == 0)
        {
            PWATCH_PID_ENTRY CurrPidEntry = ExAllocatePoolWithTag(PagedPool, sizeof(WATCH_PID_ENTRY), PID_POOL_TAG);
            
            if (CurrPidEntry == NULL)
                goto ReleaseMutex;

            CurrPidEntry->ProcessId = ProcessId;
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Match found, adding PID %p to protected list\n", ProcessId);

            KeAcquireGuardedMutex(&PidWatchListMutex);

            if (CurrentPidWatchCount >= MAX_PID_COUNT)
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL,
                    "CreateProcessNotifyRoutine: Can only support watching %u pids\n",
                    MAX_PID_COUNT);
                KeReleaseGuardedMutex(&PidWatchListMutex);
                goto ReleaseMutex;
            }

            CurrentPidWatchCount++;
            InsertHeadList(&PidWatchList, &CurrPidEntry->List);

            KeReleaseGuardedMutex(&PidWatchListMutex);
        }
        CurrEntry = CurrEntry->Flink;
    }

ReleaseMutex:
    KeReleaseGuardedMutex(&ProcessWatchListMutex);
Exit:
    return;
}