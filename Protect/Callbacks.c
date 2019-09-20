#include "Protect.h"
#include "UserKernelBridge.h"

#define PROCESS_TERMINATE       0x0001
#define PROCESS_CREATE_PROCESS  0x0080
#define PROCESS_CREATE_THREAD   0x0002
#define PROCESS_VM_READ         0x0010
#define PROCESS_VM_WRITE        0x0020

#define ALTITUDE L"1000"

KGUARDED_MUTEX CallbackMutex;
KGUARDED_MUTEX ProcessWatchListMutex;
KGUARDED_MUTEX PidWatchListMutex;

LIST_ENTRY ProcessWatchList;
LIST_ENTRY PidWatchList;

OB_CALLBACK_REGISTRATION CallbackRegistration = { 0 };
OB_OPERATION_REGISTRATION OperationRegistration[2] = { {0}, {0} };
UNICODE_STRING Altitude;
BOOLEAN CallbackInstalled = FALSE;
PVOID RegistrationHandle = NULL;

ULONG CurrentWatchCount = 0;

NTSTATUS
RegisterCallbacks(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
 
    if (CallbackInstalled)
        return Status;

    KeAcquireGuardedMutex(&CallbackMutex);

    RtlInitUnicodeString(&Altitude, ALTITUDE);
    OperationRegistration[0].ObjectType = PsProcessType;
    OperationRegistration[0].Operations |= OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    OperationRegistration[0].PreOperation = PreOpCallback;
    OperationRegistration[0].PostOperation = PostOpCallback;

    OperationRegistration[1].ObjectType = PsThreadType;
    OperationRegistration[1].Operations |= OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    OperationRegistration[1].PreOperation = PreOpCallback;
    OperationRegistration[1].PostOperation = PostOpCallback;

    CallbackRegistration.OperationRegistrationCount = 2;
    CallbackRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    CallbackRegistration.Altitude = Altitude;
    CallbackRegistration.RegistrationContext = NULL;
    CallbackRegistration.OperationRegistration = OperationRegistration;

    Status = ObRegisterCallbacks(&CallbackRegistration, &RegistrationHandle);
    
    KeReleaseGuardedMutex(&CallbackMutex);

    if (!NT_SUCCESS(Status))
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "RegisterCallbacks: ObRegisterCallbacks failed with 0x%x", Status);
    else
        CallbackInstalled = TRUE;

    return Status;
}

VOID
UnRegisterCallbacks(
)
{
    if (!CallbackInstalled)
        return;

    KeAcquireGuardedMutex(&CallbackMutex);
    ObUnRegisterCallbacks(&RegistrationHandle);
    KeReleaseGuardedMutex(&CallbackMutex);
}

OB_PREOP_CALLBACK_STATUS
PreOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreOpInfo
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    
    ACCESS_MASK PermissionsToRemove = 0;
    PermissionsToRemove |= PROCESS_DUP_HANDLE;
    PermissionsToRemove |= PROCESS_VM_READ;
    PermissionsToRemove |= PROCESS_VM_WRITE;
    PermissionsToRemove |= PROCESS_TERMINATE;

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

    KeAcquireGuardedMutex(&PidWatchListMutex);
    
    if (IsListEmpty(&PidWatchList))
        goto ReleaseMutex;

    // If a process/thread operation originates from the original process/thread
    // exit the function. Otherwise, iterate over the list of Process Ids to protect.
    PLIST_ENTRY CurrEntry = PidWatchList.Flink;
    HANDLE BlockedPid = NULL;
    BOOLEAN Found = FALSE;

    while (CurrEntry != &PidWatchList)
    {
        HANDLE CurrPid = CONTAINING_RECORD(CurrEntry, WATCH_PID_ENTRY, List)->ProcessId;
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, 
            "Current pid: %p\n", CurrPid);

        if (PreOpInfo->ObjectType == *PsProcessType)
        {
            if (PreOpInfo->Object == PsGetCurrentProcess())
                goto ReleaseMutex;

            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, 
                "Pid of handle obtained: %p\n", 
                PsGetProcessId(PreOpInfo->Object));

            if (PsGetProcessId((PEPROCESS)PreOpInfo->Object) == CurrPid)
            {
                BlockedPid = CurrPid;
                PermissionsToRemove |= PROCESS_CREATE_PROCESS;
                Found = TRUE;
                break;
            }
        }
        else if (PreOpInfo->ObjectType == *PsThreadType)
        {
            HANDLE ThreadProcId = PsGetThreadProcessId((PETHREAD)PreOpInfo->Object);
            
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, 
                "Pid of handle obtained: %p\n", ThreadProcId);

            if (ThreadProcId == PsGetCurrentProcessId())
                goto ReleaseMutex;

            if (ThreadProcId == CurrPid)
            {
                BlockedPid = CurrPid;
                PermissionsToRemove |= PROCESS_CREATE_THREAD;
                Found = TRUE;
                break;
            }
        }
        else
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "Somehow managed to get here :feelsdankman:\n");
            goto ReleaseMutex;
        }

        CurrEntry = CurrEntry->Flink;
    }
 
    if (!Found)
        goto ReleaseMutex;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, 
        "PreOpCallback: Attempt to access 0x%p from PID: 0x%p\n", 
        BlockedPid, 
        PsGetCurrentProcessId());

    // Actually perform the filtering
    if (ModifiedPermissions != NULL)
        *ModifiedPermissions &= ~PermissionsToRemove;

ReleaseMutex:
    KeReleaseGuardedMutex(&PidWatchListMutex);

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