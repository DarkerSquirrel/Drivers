#include "Protect.h"
#include "UserKernelBridge.h"
#include "IOCTL.h"
#include "Misc.h"

NTSTATUS
IOCTLAddProcessToWatchList(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _In_ PIRP pIRP
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PPROTECT_INPUT pInput = (PPROTECT_INPUT)pIRP->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIRP);

    ASSERT(pIrpStack != NULL);

    ULONG InputLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    PWATCH_PROCESS_ENTRY CurrentEntry = NULL;

    if (InputLength < sizeof(PROTECT_INPUT))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, 
            "IOCTLAddProcessToWatchList: Naughty input, incorrect size. Got => %u Expected => %llu",
            InputLength,
            sizeof(PROTECT_INPUT)
        );

        goto Exit;
    }

    CurrentEntry = (PWATCH_PROCESS_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(WATCH_PROCESS_ENTRY), LIST_POOL_TAG);

    if (CurrentEntry == NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IOCTLAddProcessToWatchList: ExAllocatePool failed");
        goto Exit;
    }

    memcpy(CurrentEntry->Name, pInput->Name, sizeof(CurrentEntry->Name));

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    CurrentWatchCount++;
    InsertTailList(&ProcessWatchList, &CurrentEntry->List);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);

    Status = STATUS_SUCCESS;

Exit:
    if (!NT_SUCCESS(Status))
    {
        if (CurrentEntry != NULL)
            ExFreePoolWithTag(CurrentEntry, LIST_POOL_TAG);
    }

    pIRP->IoStatus.Information = 0;
    return Status;
}

NTSTATUS
IOCTLEnumerateWatchList(
    _In_    PDEVICE_OBJECT pDeviceObject,
    _Inout_ PIRP pIRP
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);

    NTSTATUS Status = STATUS_SUCCESS;

    PENUMERATE_PROCESS_INFO pOutput = (PENUMERATE_PROCESS_INFO)pIRP->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION IrpStackLocation = IoGetCurrentIrpStackLocation(pIRP);

    ASSERT(IrpStackLocation != NULL);

    ULONG OutputLen = IrpStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG CurrCount = 0;
    ULONG CurrPidIndex = 0;
    
    if (OutputLen < sizeof(ENUMERATE_PROCESS_INFO))
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    memset(pOutput, 0, sizeof(ENUMERATE_PROCESS_INFO));

    KeAcquireGuardedMutex(&ProcessWatchListMutex);
    KeAcquireGuardedMutex(&PidWatchListMutex);

    pOutput->WatchCount = CurrentWatchCount;
    pOutput->PidWatchCount = CurrentPidWatchCount;

    if (IsListEmpty(&ProcessWatchList))
        goto ReleaseMutex;

    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;

    while (CurrEntry != &ProcessWatchList)
    {
        memcpy(pOutput->Names[CurrCount],
            CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name,
            MAX_PATH + 1);
        CurrEntry = CurrEntry->Flink;
        CurrCount++;
    }

    memset(pOutput->Pids, -1, sizeof(pOutput->Pids));

    if (IsListEmpty(&PidWatchList))
        goto ReleaseMutex;

    PLIST_ENTRY CurrPidEntry = PidWatchList.Flink;

    while (CurrPidEntry != &PidWatchList)
    {
        pOutput->Pids[CurrPidIndex] = (INT64)CONTAINING_RECORD(CurrPidEntry, WATCH_PID_ENTRY, List)->ProcessId;
        CurrPidEntry = CurrPidEntry->Flink;
        CurrPidIndex++;
    }

ReleaseMutex:
    KeReleaseGuardedMutex(&PidWatchListMutex);
    KeReleaseGuardedMutex(&ProcessWatchListMutex);

Exit:
    pIRP->IoStatus.Information = CurrCount * (MAX_PATH + 1) +
        sizeof(CurrentWatchCount) +
        CurrPidIndex * sizeof(pOutput->Pids[0]);

    return Status;
}

NTSTATUS
IOCTLClearWatchList(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _In_ PIRP pIRP
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(pDeviceObject);
    UNREFERENCED_PARAMETER(pIRP);

    NTSTATUS Status = STATUS_SUCCESS;

    ClearWatchList();

    pIRP->IoStatus.Information = 0;
    return Status;
}