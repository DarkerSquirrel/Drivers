#include "ProtectCtrl.h"

using namespace std;

BOOL InstallDriver(
    _In_ LPCWSTR DriverInstallPath
)
{
    // Ensure driver is uninstalled
    if (!UninstallDriver())
        return FALSE;

    // Driver is assumed to be in the same directory
    // Copy the driver into the system driver directory
    auto Status = CopyFileW(
        DRIVER_NAME_AND_EXT, 
        DriverInstallPath, 
        TRUE
    );

    if (!Status)
    {
        cout << "InstallDriver: CopyFileW failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    // Open a handle to the Service Control Manager
    // Required to start a service
    auto SCMHandle = OpenSCManagerW(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
    );

    if (SCMHandle == NULL)
    {
        cout << "InstallDriver: OpenSCManagerW failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    // Create the service
    auto ServiceHandle = CreateServiceW(
        SCMHandle,
        DRIVER_NAME,
        DRIVER_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DriverInstallPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (ServiceHandle == NULL)
    {
        cout << "InstallDriver: CreateServiceW failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    // Start the service
    Status = StartServiceW(
        ServiceHandle,
        0, 
        NULL
    );

    if (!Status)
    {
        cout << "InstallDriver: StartServiceW failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    if (SCMHandle)
        CloseServiceHandle(SCMHandle);
    if (ServiceHandle)
        CloseServiceHandle(ServiceHandle);

    return TRUE;
}

BOOL UninstallDriver(
)
{
    // Open a handle to the Service Control Manager
    auto SCMHandle = OpenSCManagerW(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
    );
 
    if (SCMHandle == NULL)
    {
        cout << "UninstallDriver: OpenSCManagerW failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    // Open a handle to the service
    auto ServiceHandle = OpenService(
        SCMHandle,
        DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    if (ServiceHandle == NULL)
    {
        cout << "UninstallDriver: OpenService failed with " <<
            hex << GetLastError() << endl;

        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            return TRUE;

        return FALSE;
    }

    SERVICE_STATUS ServiceStatus;
    
    auto Status = ControlService(
        ServiceHandle, 
        SERVICE_CONTROL_STOP, 
        &ServiceStatus);

    if (!Status)
    {
        cout << "UninstallDriver: ControlService failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    SERVICE_STATUS_PROCESS ServiceStatusProcess;
    DWORD Bytes;

    while (TRUE)
    {
        auto QueryStatus = QueryServiceStatusEx(
            ServiceHandle,
            SC_STATUS_PROCESS_INFO,
            (BYTE *)&ServiceStatusProcess,
            sizeof(ServiceStatusProcess),
            &Bytes
        );

        if (!QueryStatus)
        {
            cout << "UninstallDriver: QueryServiceStatusEx failed with " <<
                hex << GetLastError() << endl;
            return FALSE;
        }

        if (ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED)
            break;
    }

    Status = DeleteService(ServiceHandle);

    if (!Status)
    {
        cout << "UninstallDriver: DeleteService failed with " <<
            hex << GetLastError() << endl;
        return FALSE;
    }

    if (ServiceHandle)
        CloseServiceHandle(ServiceHandle);
    if (SCMHandle)
        CloseServiceHandle(SCMHandle);

    return TRUE;
}