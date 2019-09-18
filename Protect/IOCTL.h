#pragma once

NTSTATUS
IOCTLAddProcessToWatchList(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _In_ PIRP pIRP
);

NTSTATUS
IOCTLEnumerateWatchList(
    _In_ PDEVICE_OBJECT pDeviceObject,
    _Inout_ PIRP pIRP
);

NTSTATUS
IOCTLClearWatchList(
    _In_ PDEVICE_OBJECT pDeviceObject, 
    _In_ PIRP pIRP
);