#include "Protect.h"
#include "UserKernelBridge.h"
#include "IOCTL.h"
#include "Misc.h"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)          DRIVER_DISPATCH DeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE)           DRIVER_DISPATCH DeviceClose;
_Dispatch_type_(IRP_MJ_CLEANUP)         DRIVER_DISPATCH DeviceCleanup;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)  DRIVER_DISPATCH DeviceControl;

DRIVER_UNLOAD DriverUnload;

BOOLEAN CreateRoutineEnabled = FALSE;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{

    NTSTATUS Status;
    
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING DeviceName;
    UNICODE_STRING DosDevicesName;
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&DosDevicesName, DOS_DEVICES_NAME);
    
    PDEVICE_OBJECT pDeviceObject = NULL;
    BOOLEAN SymLinkCreated = FALSE;

    Status = IoCreateDevice(
        DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject
    );

    if (!NT_SUCCESS(Status))
        goto Exit;

    KeInitializeGuardedMutex(&CallbackMutex);
    KeInitializeGuardedMutex(&ProcessWatchListMutex);
    KeInitializeGuardedMutex(&PidWatchListMutex);

    InitializeListHead(&ProcessWatchList);
    InitializeListHead(&PidWatchList);

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DeviceClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = DeviceCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload                         = DriverUnload;

    Status = IoCreateSymbolicLink(&DosDevicesName, &DeviceName);

    if (!NT_SUCCESS(Status))
        goto Exit;
    
    SymLinkCreated = TRUE;

    Status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutine, FALSE);

    if (!NT_SUCCESS(Status))
        goto Exit;

    CreateRoutineEnabled = TRUE;

    Status = RegisterCallbacks();

    if (!NT_SUCCESS(Status))
        goto Exit;

Exit:
    if (!NT_SUCCESS(Status))
    {
        if (CreateRoutineEnabled)
        {
            PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutine, TRUE);
            CreateRoutineEnabled = FALSE;
        }
       
        if (CallbackInstalled)
            UnRegisterCallbacks();
        if (SymLinkCreated)
            IoDeleteSymbolicLink(&DosDevicesName);
        if (pDeviceObject != NULL)
            IoDeleteDevice(pDeviceObject);
    }

    return Status;
}

NTSTATUS
DeviceCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDeviceObject);

    PIO_STACK_LOCATION pIRPStack;
    pIRPStack = IoGetCurrentIrpStackLocation(pIRP);

    switch (pIRPStack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_PROTECT_ADD:
        Status = IOCTLAddProcessToWatchList(pDeviceObject, pIRP);
        break;
    case IOCTL_PROTECT_CLEAR:
        Status = IOCTLClearWatchList(pDeviceObject, pIRP);
        break;
    case IOCTL_PROTECT_ENUM:
        Status = IOCTLEnumerateWatchList(pDeviceObject, pIRP);
        break;
    default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    pIRP->IoStatus.Status = Status;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT pDriverObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DosDevicesName;
    RtlInitUnicodeString(&DosDevicesName, DOS_DEVICES_NAME);

    ClearWatchList();
    UnRegisterCallbacks();

    Status = IoDeleteSymbolicLink(&DosDevicesName);
    
    if (!NT_SUCCESS(Status))
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload: IoDeleteSymbolicLink failed with 0x%x", Status);
    
    if (pDriverObject->DeviceObject != NULL)
        IoDeleteDevice(pDriverObject->DeviceObject);

    if (CreateRoutineEnabled)
    {
        PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutine, TRUE);
        CreateRoutineEnabled = FALSE;
    }
}