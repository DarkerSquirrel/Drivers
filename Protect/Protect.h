#pragma once
#include <ntddk.h>

#define MAX_PATH 256
#define LIST_POOL_TAG 'torP'
#define PID_POOL_TAG 'diPP'

extern KGUARDED_MUTEX CallbackMutex;
extern KGUARDED_MUTEX ProcessWatchListMutex;
extern KGUARDED_MUTEX PidWatchListMutex;

extern LIST_ENTRY ProcessWatchList;
extern LIST_ENTRY PidWatchList;

extern UINT32 CurrentWatchCount;

typedef struct _CALLBACK_PARAMS
{
	ACCESS_MASK AccessClear;
	ACCESS_MASK AccessSet;
} CALLBACK_PARAMS, *PCALLBACK_PARAMS;

typedef struct _REGISTRATION_INFO
{
	PVOID RegistrationHandle;

	PVOID TargetProcess;
	HANDLE TargetProcessId;

    WCHAR ProtectedName[MAX_PATH + 1];
} REGISTRATION_INFO, *PREGISTRATION_INFO;

typedef struct _CALL_CONTEXT
{
	PREGISTRATION_INFO CallbackRegistration;

	OB_OPERATION Operation;
	PVOID Object;
	POBJECT_TYPE ObjectType;
} CALL_CONTEXT, *PCALL_CONTEXT;

typedef struct _WATCH_PROCESS_ENTRY
{
    LIST_ENTRY List;
    WCHAR Name[MAX_PATH + 1];
} WATCH_PROCESS_ENTRY, *PWATCH_PROCESS_ENTRY;

typedef struct _WATCH_PID_ENTRY
{
    LIST_ENTRY List;
    HANDLE ProcessId;
} WATCH_PID_ENTRY, *PWATCH_PID_ENTRY;

VOID
RegisterCallbacks(
);

VOID
UnRegisterCallbacks(
);

OB_PREOP_CALLBACK_STATUS
PreOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreOpInfo
);

VOID
PostOpCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_POST_OPERATION_INFORMATION PostOpInfo
);

VOID
CreateProcessNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);