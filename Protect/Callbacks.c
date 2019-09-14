#include "Protect.h"
#include "UserKernelBridge.h"

#define PROCESS_CREATE_PROCESS  0x0001
#define PROCESS_CREATE_THREAD   0x0002
#define PROCESS_VM_READ         0x0010
#define PROCESS_VM_WRITE        0x0020

KGUARDED_MUTEX CallbackMutex;

KGUARDED_MUTEX ProcessWatchListMutex;
LIST_ENTRY ProcessWatchList;
UINT32 CurrentWatchCount = 0;

OB_PREOP_CALLBACK_STATUS
PreOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreOpInfo
)
{
    PREGISTRATION_INFO RegContext = (PREGISTRATION_INFO)RegistrationContext;
    
    ACCESS_MASK PermissionsToRemove = 0;
    PermissionsToRemove |= PROCESS_DUP_HANDLE;
    PermissionsToRemove |= PROCESS_VM_READ;
    PermissionsToRemove |= PROCESS_VM_WRITE;

    PACCESS_MASK ModifiedPermissions = NULL;
    
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
        goto Exit;
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "PreOpCallback: Attempt to access %ls from PID: 0x%p", RegContext->ProtectedName, PsGetCurrentProcessId());

    // Actually perform the filtering
    if (ModifiedPermissions != NULL)
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
    UNREFERENCED_PARAMETER(RegistrationContext);
    UNREFERENCED_PARAMETER(PostOpInfo);
    // Nothing to do here
}

