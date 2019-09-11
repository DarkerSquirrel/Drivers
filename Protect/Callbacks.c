#pragma once
#include "Protect.h"
#include "UserKernelBridge.h"

#define PROCESS_CREATE_PROCESS  0x0001
#define PROCESS_CREATE_THREAD   0x0002
#define PROCESS_DUP_HANDLE      0x0040
#define PROCESS_VM_READ         0x0010
#define PROCESS_VM_WRITE        0x0020

KGUARDED_MUTEX CallbackMutex;

OB_PREOP_CALLBACK_STATUS
PreOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreOpInfo
)
{
    PCALLBACK_REGISTRATION RegContext = (PCALLBACK_REGISTRATION)RegistrationContext;
    
    ACCESS_MASK PermissionsToRemove = 0;
    PermissionsToRemove |= PROCESS_DUP_HANDLE;
    PermissionsToRemove |= PROCESS_VM_READ;
    PermissionsToRemove |= PROCESS_VM_WRITE;

    PACCESS_MASK ModifiedPermissions;
    
    switch (PreOpInfo->Operation)
    {
    case OB_OPERATION_HANDLE_CREATE:
        ModifiedPermissions = &PreOpInfo->Parameters->CreateHandleInformation.DesiredAccess;
        break;
    case OB_OPERATION_HANDLE_DUPLICATE:
        ModifiedPermissions = &PreOpInfo->Parameters->DuplicateHandleInformation.DesiredAccess;
        break;
    }

    if (PreOpInfo->ObjectType == *PsProcessType)
    {
        if (PreOpInfo->Object == PsGetCurrentProcess() ||
            PreOpInfo->Object != RegContext->TargetProcess)
            goto Exit;
        
        PermissionsToRemove |= PROCESS_CREATE_PROCESS;
    }
    else if (PreOpInfo->ObjectType == *PsThreadType)
    {
        HANDLE ThreadProcId = PsGetThreadProcessId((PETHREAD)PreOpInfo->Object);
        if (ThreadProcId != RegContext->TargetProcessId ||
            ThreadProcId == PsGetCurrentProcessId())
            goto Exit;

        PermissionsToRemove |= PROCESS_CREATE_THREAD;
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PreOpCallback: Meme ObjectType provided 0x%x", PreOpInfo->ObjectType);
        goto Exit;
    }

    // Actually perform the filtering
    *ModifiedPermissions &= ~PermissionsToRemove;
Exit:
    return OB_PREOP_SUCCESS;
}

VOID
PostOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_POST_OPERATION_INFORMATION PostOpInfo
)
{
    // Nothing to do here
    return;
}