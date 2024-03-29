#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <exception>
#include <winioctl.h>
#include "../Protect/UserKernelBridge.h"

class SCHandle
{
private:
    SC_HANDLE Handle_;
    SCHandle& operator=(const SCHandle&) = delete;

public:
    SCHandle(SC_HANDLE Handle) noexcept : Handle_(Handle) {}
    SCHandle& operator=(SCHandle&& Other) noexcept
    {
        if (this == &Other)
            return *this;

        Handle_ = Other.Handle_;
        Other.Handle_ = NULL;
        return *this;
    };
    ~SCHandle() noexcept
    {
        if (Handle_ != NULL)
            CloseServiceHandle(Handle_);
    }
};

VOID
InstallDriver(
    _In_ LPCWSTR DriverInstallDirectory
);

VOID
UninstallDriver(
);

VOID
ProtectAdd(
    _In_ LPWSTR pProtectName
);

VOID
ProtectClear(
);

VOID
ProtectEnum(
);