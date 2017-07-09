#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_NON_CONFORMING_SWPRINTFS

#pragma warning (disable: 4091)

#include "stdafx.h"

#include "resource.h"

#include <cstdint>

#include <string>
#include <memory>
#include <algorithm>

#include <ShellAPI.h>
#include <CommCtrl.h>

#include <Shlobj.h>
#include <Shlwapi.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <atlbase.h>

//HMODULE hModGlobal = 0;//LoadLibrary (L"SpecialK64.dll");

typedef void   (WINAPI *SKX_InstallCBTHook_pfn)  (void);
typedef void   (WINAPI *SKX_RemoveCBTHook_pfn)   (void);
typedef bool   (WINAPI *SKX_IsHookingCBT_pfn)    (void);
typedef size_t (WINAPI *SKX_GetInjectedPIDs_pfn) (DWORD*, size_t);

//SKX_RemoveCBTHook_pfn   SKX_RemoveCBTHook   = nullptr;
//SKX_InstallCBTHook_pfn  SKX_InstallCBTHook  = nullptr;
//SKX_IsHookingCBT_pfn    SKX_IsHookingCBT    = nullptr;
//SKX_GetInjectedPIDs_pfn SKX_GetInjectedPIDs = nullptr;

HMODULE      SKIM_GlobalInject_Load      (void);
BOOL         SKIM_GlobalInject_Free      (void);
bool         SKIM_GlobalInject_Start     (void);
std::wstring SKIM_GetStartupShortcut     (void);
std::wstring SKIM_GetStartMenuShortcut   (void);
bool         SKIM_IsLaunchedAtStartup    (void);
bool         SKIM_SetStartupInjection    (bool enable, wchar_t* wszExecutable);
bool         SKIM_SetStartMenuLink       (bool enable, wchar_t* wszExecutable);
bool         SKIM_GlobalInject_Stop      (bool confirm = true);
bool         SKIM_GlobalInject_Stop      (HWND hWndDlg, bool confirm = true);
bool         SKIM_GlobalInject_Start     (HWND hWndDlg);
bool         SKIM_GlobalInject_StartStop (HWND hWndDlg, bool confirm = true);
size_t       SKIM_SummarizeInjectedPIDs  (std::wstring& out);
void         SKIM_StopInjectingAndExit   (HWND hWndDlg, bool confirm = true);
int          SKIM_GetInjectorState       (void);