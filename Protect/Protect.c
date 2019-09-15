#include "Protect.h"
#include "UserKernelBridge.h"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)          DRIVER_DISPATCH DeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE)           DRIVER_DISPATCH DeviceClose;
_Dispatch_type_(IRP_MJ_CLEANUP)         DRIVER_DISPATCH DeviceCleanup;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)  DRIVER_DISPATCH DeviceControl;

DRIVER_UNLOAD DriverUnload;

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
    
    PDEVICE_OBJECT pDeviceObject;
    BOOLEAN SymLinkCreated = FALSE;
    BOOLEAN NotifyRoutineSet = FALSE;

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

    PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutine,FALSE);

Exit:
    if (!NT_SUCCESS(Status))
    {
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
ControlProtect(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _In_ PPROTECT_INPUT pInput
)
{
    UNREFERENCED_PARAMETER(pDeviceObject);
    UNREFERENCED_PARAMETER(pInput);
    return STATUS_SUCCESS;
}

NTSTATUS
ControlRemoveProtect(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _In_ PPROTECT_INPUT pIRP
)
{
    UNREFERENCED_PARAMETER(pDeviceObject);
    UNREFERENCED_PARAMETER(pIRP);
    return STATUS_SUCCESS;
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
   
    ULONG InputBufferLength = pIRPStack->Parameters.DeviceIoControl.InputBufferLength;
    
    if (InputBufferLength == 0)
    {
        Status = STATUS_INVALID_BUFFER_SIZE;
        return Status;
    }

    if (InputBufferLength < sizeof(PROTECT_INPUT))
    {
        Status = STATUS_BUFFER_OVERFLOW;
        return Status;
    }

    PPROTECT_INPUT pInput = (PPROTECT_INPUT)pIRP->AssociatedIrp.SystemBuffer;

    switch (pIRPStack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_PROTECT:
        Status = ControlProtect(pDeviceObject, pInput);
        break;
    case IOCTL_PROTECT_REMOVE:
        Status = ControlRemoveProtect(pDeviceObject, pInput);
        break;
    case IOCTL_PROTECT_ENUM:
        Status = 1;
    }

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

    Status = IoDeleteSymbolicLink(&DosDevicesName);
    
    if (!NT_SUCCESS(Status))
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload: IoDeleteSymbolicLink failed with 0x%x", Status);
    
    IoDeleteDevice(pDriverObject->DeviceObject);
}

