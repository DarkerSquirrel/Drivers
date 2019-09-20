#pragma once

#define IOCTL_PROTECT_ADD       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTECT_CLEAR     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTECT_ENUM      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEVICE_NAME	            L"\\Device\\Protect"
#define DOS_DEVICES_NAME        L"\\DosDevices\\Protect"
#define USER_DEVICE_NAME        L"\\\\.\\Protect"

#define DRIVER_NAME             L"Protect"
#define DRIVER_NAME_AND_EXT     L"Protect.sys"

#define DEFAULT_SYS_LOCATION    L"C:\\Windows\\System32\\Drivers\\"
#define DEFAULT_INSTALL_PATH    L"C:\\Windows\\System32\\Drivers\\Protect.sys"

#define MAX_WATCH_COUNT 10

typedef struct _PROTECT_INPUT
{
    WCHAR Name[MAX_PATH + 1];
} PROTECT_INPUT, *PPROTECT_INPUT;

typedef struct _ENUMERATE_PROCESS_INFO
{
    ULONG WatchCount;
    WCHAR Names[MAX_WATCH_COUNT][MAX_PATH + 1];
} ENUMERATE_PROCESS_INFO, *PENUMERATE_PROCESS_INFO;