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
    _In_  ULONG Ioctl,
    _In_  LPVOID IoctlInput,
    _Out_ LPVOID IoctlOutput
)
noexcept
{
    try
    {
        auto Handle = GetDeviceHandle();
        DWORD bytes = 0;
        BOOL Status;

        LPVOID pInput       = IoctlInput;
        LPVOID pOutput      = IoctlOutput;
        DWORD InputLength   = 0;
        DWORD OutputLength  = 0;

        switch (Ioctl)
        {
        case IOCTL_PROTECT_ADD:
            InputLength = sizeof(PROTECT_INPUT);
            break;
        
        case IOCTL_PROTECT_CLEAR:
            break;

        case IOCTL_PROTECT_ENUM:
            OutputLength = sizeof(ENUMERATE_PROCESS_INFO);
            break;
        }

        Status = DeviceIoControl(
            Handle,
            Ioctl,
            pInput,
            InputLength,
            pOutput,
            OutputLength,
            &bytes,
            NULL);

        if (!Status)
            wcout << L"Operation failed" << endl;
    }
    catch (runtime_error& e)
    {
        cout << e.what() << "\n" << 
            "Last error code: 0x" << hex << GetLastError() << endl;
    }
}

VOID
ProtectControl(
)
{

}