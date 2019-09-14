#include "ProtectCtrl.h"

using namespace std;

const string help = 
"Arguments supported:\n\
-install\n\
-uninstall";

int 
main(
    _In_ int argc, 
    _In_ LPCWSTR *argv
)
{
    if (argc <= 1)
    {
        cout << help << endl;
        return 1;
    }

    if (wcscmp(argv[1], L"-install"))
    {
        InstallDriver(DEFAULT_INSTALL_PATH);
    }
    else if (wcscmp(argv[1], L"-uninstall"))
    {
        UninstallDriver();
    }
    else if (wcscmp(argv[1], L"-h") || wcscmp(argv[1], L"-help"))
    {
        cout << help << endl;
    }
    
    return 0;
}