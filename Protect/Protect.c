#pragma once
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
    
    UNICODE_STRING DeviceName;
    UNICODE_STRING DosDevicesName;
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    RtlInitUnicodeString(&DosDevicesName, DOS_DEVICES_NAME);
    
    PDEVICE_OBJECT pDeviceObject;
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

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DeviceClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = DeviceCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload                         = DriverUnload;

    Status = IoCreateSymbolicLink(&DosDevicesName, &DeviceName);

    if (!NT_SUCCESS(Status))
        goto Exit;
    
    SymLinkCreated = TRUE;

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
    _In_ DEVICE_OBJECT DeviceObject,
    _In_ PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceClose(
    _In_ DEVICE_OBJECT DeviceObject,
    _In_ PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceCleanup(
    _In_ DEVICE_OBJECT DeviceObject,
    _In_ PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    pIRP->IoStatus.Status = Status;
    pIRP->IoStatus.Information = 0;
    IoCompleteRequest(pIRP, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
DeviceControl(
    _In_ DEVICE_OBJECT DeviceObject,
    _In_ PIRP pIRP
)
{
    NTSTATUS Status = STATUS_SUCCESS;
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

    switch (pInput->Operation)
    {
    case IOCTL_PROTECT:
        // Status = ProtectFunc
        break;
    case IOCTL_REMOVE_PROTECT:
        // Status = RemoveProtectFunc
        break;
    }

    return Status;
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT pDriverObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DosDevicesName;
    RtlInitUnicodeString(&DosDevicesName, DOS_DEVICES_NAME);

    Status = IoDeleteSymbolicLink(&DosDevicesName);
    
    if (!NT_SUCCESS(Status))
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnload: IoDeleteSymbolicLink failed with 0x%x", Status);
    
    IoDeleteDevice(pDriverObject->DeviceObject);

    return Status;
}