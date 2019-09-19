#include "ProtectCtrl.h"

using namespace std;

VOID
InstallDriver(
    _In_ LPCWSTR DriverInstallPath
)
{
    try
    {
        // Ensure driver is uninstalled
        UninstallDriver();
    }
    catch (runtime_error& err)
    {
        cout << err.what() << "\n" <<
            hex << GetLastError() << endl;
    }

    // Driver is assumed to be in the same directory
    // Copy the driver into the system driver directory
    auto Status = CopyFileW(
        DRIVER_NAME_AND_EXT, 
        DriverInstallPath, 
        TRUE
    );

    if (!Status)
        throw runtime_error("InstallDriver: CopyFileW failed");

    // Open a handle to the Service Control Manager
    // Required to start a service
    auto SCMHandle = OpenSCManagerW(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
    );

    if (SCMHandle == NULL)
        throw runtime_error("InstallDriver: OpenSCManagerW failed");

    SCHandle SCMHandleWrapper(SCMHandle);

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
        throw runtime_error("InstallDriver: CreateServiceW failed");

    SCHandle ServiceHandleWrapper(ServiceHandle);

    // Start the service
    Status = StartServiceW(
        ServiceHandle,
        0, 
        NULL
    );

    if (!Status)
        throw runtime_error("InstallDriver: StartServiceW failed");
}

VOID
UninstallDriver(
)
{
    // Open a handle to the Service Control Manager
    auto SCMHandle = OpenSCManagerW(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
    );
 
    if (SCMHandle == NULL)
        throw runtime_error("UninstallDriver: OpenSCManagerW failed");

    SCHandle SCMHandleWrapper(SCMHandle);

    // Open a handle to the service
    auto ServiceHandle = OpenService(
        SCMHandle,
        DRIVER_NAME,
        SERVICE_ALL_ACCESS
    );

    if (ServiceHandle == NULL)
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            return;
        throw runtime_error("UninstallDriver: OpenService failed");
    }

    SCHandle ServiceHandleWrapper(ServiceHandle);

    SERVICE_STATUS ServiceStatus;
    
    auto Status = ControlService(
        ServiceHandle,
        SERVICE_CONTROL_STOP,
        &ServiceStatus);

    if (!Status)
        throw runtime_error("UninstallDriver: ControlService failed");
    
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
            throw runtime_error("UninstallDriver: QueryServiceStatusEx failed");

        if (ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED)
            break;
    }

    Status = DeleteService(ServiceHandle);

    if (!Status)
        throw runtime_error("UninstallDriver: DeleteService failed");
}