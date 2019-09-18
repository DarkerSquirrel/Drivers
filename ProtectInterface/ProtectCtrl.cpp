#include "ProtectCtrl.h"

using namespace std;

const string help =
"Arguments supported:\n"
"-install: Installs the driver\n"
"-uninstall: Uninstalls the driver\n"
"-protect <name>: Marks <name> as protected, preventing any operations to it"
"-enum: Retrieves the currently protected names and pids"
"-clear: Clears the currently protected names and pids";

int 
main(
    _In_ int argc, 
    _In_ LPCWSTR *argv
)
{
    if (argc <= 1)
        goto ErrorExit;

    if (wcscmp(argv[1], L"-install"))
    {
        InstallDriver(DEFAULT_INSTALL_PATH);
    }
    else if (wcscmp(argv[1], L"-uninstall"))
    {
        UninstallDriver();
    }
    else if (wcscmp(argv[1], L"-protect"))
    {
        if (argc <= 2)
            goto ErrorExit;

    }
    else if (wcscmp(argv[1], L"-clear"))
    {

    }
    else if (wcscmp(argv[1], L"-enum"))
    {

    }
    else if (wcscmp(argv[1], L"-h") || wcscmp(argv[1], L"-help"))
    {
        cout << help << endl;
    }
    
    return 0;

ErrorExit:
    cout << help << endl;
    return 1;
}