/* Copyright (C) 2015, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#ifndef PROCESSTHREADSAPI_WRAPPERS_H
#define PROCESSTHREADSAPI_WRAPPERS_H

#include <windows.h>

#undef OpenProcessToken
#define OpenProcessToken    wrap_OpenProcessToken
#define GetCurrentProcess   wrap_GetCurrentProcess
#define SetThreadPriority   wrap_SetThreadPriority
#define GetCurrentThread    wrap_GetCurrentThread
#define CreateThread        wrap_CreateThread

WINBOOL wrap_SetThreadPriority(HANDLE hThread, int nPriority);

HANDLE wrap_GetCurrentThread(VOID);

HANDLE wrap_GetCurrentProcess(VOID);

HANDLE wrap_CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
                         SIZE_T dwStackSize,
                         LPTHREAD_START_ROUTINE lpStartAddress,
                         PVOID lpParameter,
                         DWORD dwCreationFlags,
                         LPDWORD lpThreadId);

BOOL wrap_OpenProcessToken(HANDLE ProcessHandle,
                           DWORD DesiredAccess,
                           PHANDLE TokenHandle);

/**
 * @brief This function loads the expect and will_return calls for the wrapper of SetThreadPriority
 */
void expect_SetThreadPriority_call(HANDLE handle, int priority, int ret);
#endif
