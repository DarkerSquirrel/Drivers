#pragma once

NTSTATUS
IOCTLAddProcessToWatchList(
    _In_ PPROTECT_INPUT pInput
);

NTSTATUS
IOCTLEnumerateWatchList(
    _In_ UINT64 OutputLen,
    _Out_ PENUMERATE_PROCESS_INFO OutputBuffer
);

NTSTATUS
IOCTLClearWatchList(
);