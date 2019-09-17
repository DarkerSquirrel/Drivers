#include "ProtectCtrl.h"

using namespace std;

HANDLE
GetDeviceHandle(
)
{
    auto Handle = CreateFileW(
        USER_DEVICE_NAME,
        GENERIC_ALL,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (Handle == INVALID_HANDLE_VALUE)
        throw runtime_error("GetDeviceHandle: CreateFileW could not open file.");

    return Handle;
}

VOID
SendIOCTL(
    _In_ ULONG Ioctl,
    _In_ PVOID pInputBuffer,
    _In_ ULONG InputLength,
    _In_ PVOID pOutputBuffer,
    _In_ ULONG OutputLength
) noexcept
{

}