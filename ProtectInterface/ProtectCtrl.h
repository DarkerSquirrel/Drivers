#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include "../Protect/UserKernelBridge.h"

BOOL 
InstallDriver(
    _In_ LPCWSTR DriverInstallDirectory
);

BOOL
UninstallDriver(
);