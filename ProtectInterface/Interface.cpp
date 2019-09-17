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
    ULONG Ioctl,
    PVOID IoctlInfo
)
noexcept
{
    try 
    {
        auto Handle = GetDeviceHandle();
        DWORD bytes = 0;

        switch (Ioctl)
        {
        case IOCTL_PROTECT_ADD:
            PPROTECT_INPUT pInput = static_cast<PPROTECT_INPUT>(IoctlInfo);
            ULONG InputLength = sizeof(PROTECT_INPUT);

            DeviceIoControl(
                Handle,
                Ioctl,
                pInput,
                InputLength,
                NULL,
                0,
                &bytes,
                NULL);
            break;
        
        case IOCTL_PROTECT_CLEAR:
            DeviceIoControl(
                Handle,
                Ioctl,
                NULL,
                0,
                NULL,
                0,
                &bytes,
                NULL);
            break;

        case IOCTL_PROTECT_ENUM:
            break;
        }
    }
    catch (runtime_error& e)
    {
        e.what();
        cout << "Last error code: 0x" << hex << GetLastError() << endl;
    }
}