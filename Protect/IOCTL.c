#include "Protect.h"
#include "UserKernelBridge.h"
#include "IOCTL.h"

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
    
    if (OutputLen < sizeof(ENUMERATE_PROCESS_INFO))
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    memset(pOutput, 0, sizeof(ENUMERATE_PROCESS_INFO));

    KeAcquireGuardedMutex(&ProcessWatchListMutex);

    pOutput->WatchCount = CurrentWatchCount;

    if (IsListEmpty(&ProcessWatchList))
        goto ReleaseMutex;

    PLIST_ENTRY CurrEntry = ProcessWatchList.Flink;

    while (CurrEntry != NULL)
    {
        memcpy(pOutput->Names[CurrCount],
            CONTAINING_RECORD(CurrEntry, WATCH_PROCESS_ENTRY, List)->Name,
            MAX_PATH + 1);
        CurrEntry = CurrEntry->Flink;
        CurrCount++;
    }

ReleaseMutex:
    KeReleaseGuardedMutex(&ProcessWatchListMutex);

Exit:
    pIRP->IoStatus.Information = CurrCount * (MAX_PATH + 1);
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

    pIRP->IoStatus.Information = 0;
    return Status;
}