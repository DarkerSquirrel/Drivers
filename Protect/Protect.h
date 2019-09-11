#pragma once
#include <ntddk.h>

#define MAX_PATH 256

extern KGUARDED_MUTEX CallbackMutex;

typedef struct _CALLBACK_PARAMS
{
	ACCESS_MASK AccessClear;
	ACCESS_MASK AccessSet;
} CALLBACK_PARAMS, * PCALLBACK_PARAMS;

typedef struct _CALLBACK_REGISTRATION
{
	PVOID RegistrationHandle;

	PVOID TargetProcess;
	HANDLE TargetProcessId;

	ULONG RegistrationId;
} CALLBACK_REGISTRATION, * PCALLBACK_REGISTRATION;

typedef struct _CALL_CONTEXT
{
	PCALLBACK_REGISTRATION CallbackRegistration;

	OB_OPERATION Operation;
	PVOID Object;
	POBJECT_TYPE ObjectType;
} CALL_CONTEXT, * PCALL_CONTEXT;

NTSTATUS ControlProtect(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP pIRP);
NTSTATUS ControlRemoveProtect(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP pIRP);

OB_PREOP_CALLBACK_STATUS PreOpCallback(_In_ PVOID RegistrationContext, _Inout_ POB_PRE_OPERATION_INFORMATION PreOpInfo);
VOID PostOpCallback(_In_ PVOID RegistrationContext, _Inout_ POB_POST_OPERATION_INFORMATION PostOpInfo);
NTSTATUS ProtectNameCallback(_In_ PPROTECT_INPUT pProtectName); 