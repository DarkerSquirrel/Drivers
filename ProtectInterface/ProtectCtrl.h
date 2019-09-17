#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <exception>
#include "../Protect/UserKernelBridge.h"

class SCHandle
{
private:
    SC_HANDLE Handle_;
    SCHandle& operator=(const SCHandle&) = delete;
    SCHandle& operator=(SCHandle&& Other) 
    {
        if (this == &Other)
            return *this;

        Handle_ = Other.Handle_;
        Other.Handle_ = nullptr;
        return *this;
    };

public:
    SCHandle(SC_HANDLE Handle) : Handle_(Handle) {}
    SCHandle(SCHandle&& Handle) : Handle_(nullptr) {}
    ~SCHandle()
    {
        if (Handle_ != nullptr)
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