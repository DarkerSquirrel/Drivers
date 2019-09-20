#include "ProtectCtrl.h"

using namespace std;

const string help =
"Arguments supported:\n"
"-install: Installs the driver\n"
"-uninstall: Uninstalls the driver\n"
"-protect <name>: Marks <name> as protected, preventing any operations to it\n"
"-enum: Retrieves the currently protected names and pids\n"
"-clear: Clears the currently protected names and pids";

int 
wmain(
    _In_ int argc, 
    _In_ LPWSTR *argv
)
{
    if (argc <= 1)
        goto ErrorExit;

    if (wcscmp(argv[1], L"-install") == 0)
    {
        try
        {
            InstallDriver(DEFAULT_INSTALL_PATH);
            cout << "Installed successfully" << endl;
        }
        catch (runtime_error& err)
        {
            cout << err.what() << "\n"
                "Last Error: 0x" << hex << GetLastError() << endl;
        }
    }
    else if (wcscmp(argv[1], L"-uninstall") == 0)
    {
        try
        {
            UninstallDriver();
            cout << "Uninstalled successfully" << endl;
        }
        catch (runtime_error& err)
        {
            cout << err.what() << "\n"
                "Last Error: 0x"  << hex << GetLastError() << endl;
        }
    }
    else if (wcscmp(argv[1], L"-protect") == 0)
    {
        if (argc <= 2)
            goto ErrorExit;

        ProtectAdd(argv[2]);
    }
    else if (wcscmp(argv[1], L"-clear") == 0)
    {
        ProtectClear();
    }
    else if (wcscmp(argv[1], L"-enum") == 0)
    {
        ProtectEnum();
    }
    else if (wcscmp(argv[1], L"-h") == 0 || wcscmp(argv[1], L"-help") == 0)
    {
        cout << help << endl;
    }
    else
    {
        cout << help << endl;
    }
    
    return 0;

ErrorExit:
    cout << help << endl;
    return 1;
}