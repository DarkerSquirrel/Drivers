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
    _In_        ULONG Ioctl,
    _In_opt_    LPVOID IoctlInput,
    _Out_opt_   LPVOID IoctlOutput
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
            pOutput = nullptr;
            break;
        
        case IOCTL_PROTECT_CLEAR:
            break;

        case IOCTL_PROTECT_ENUM:
            pInput = nullptr;
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
        {
            cout << "Operation failed" << endl;
            return;
        }

        cout << "Operation completed successfully" << endl;
        
        if (pOutput != nullptr)
            cout << "Output contains: 0x" << hex << bytes << " bytes" << endl;
    }
    catch (runtime_error& e)
    {
        cout << e.what() << "\n" << 
            "Last error code: 0x" << hex << GetLastError() << endl;
    }
}

VOID
ProtectAdd(
    _In_ LPWSTR pProtectName
)
{
    SendIOCTL(IOCTL_PROTECT_ADD, reinterpret_cast<LPVOID>(pProtectName), nullptr);
}

VOID
ProtectClear(
)
{
    SendIOCTL(IOCTL_PROTECT_CLEAR, nullptr, nullptr);
}

VOID
ProtectEnum(
)
{
    ENUMERATE_PROCESS_INFO EnumerationInfo;
    memset(&EnumerationInfo, 0, sizeof(ENUMERATE_PROCESS_INFO));

    SendIOCTL(IOCTL_PROTECT_ENUM, nullptr, reinterpret_cast<LPVOID>(&EnumerationInfo));

    cout << "Watching for: " << EnumerationInfo.WatchCount << " processes\n";
    
    for (ULONG i = 0; i < EnumerationInfo.WatchCount; i++)
    {
        wcout << EnumerationInfo.Names[i] << endl;
    }

    cout << "Protecting: " << EnumerationInfo.PidWatchCount << "processes\n";

    for (ULONG i = 0; i < EnumerationInfo.PidWatchCount; i++)
    {
        cout << EnumerationInfo.Pids[i] << endl;
    }
}