// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_SECURE_NO_WARNINGS

#pragma warning (disable: 4091)
#pragma warning (disable: 4723)

#define NOMINMAX

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN		    // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

#include <sal.h>

#include <process.h>

#include <tlhelp32.h>
#include <WindowsX.h>
#include <Richedit.h>

#include <atlbase.h>

#include <ShellAPI.h>

#include <Shlwapi.h>
#include <Shlobj.h>
#include <shobjidl.h>
#include <shlguid.h>

#include <CommCtrl.h>
#include <MMSystem.h>

#include <WinVer.h>

#pragma comment (lib,    "winmm.lib"   )
#pragma comment (lib,    "shlwapi.lib" )
#pragma comment (lib,    "shell32.lib" )
#pragma comment (lib,    "Ole32.lib"   )
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib"  )
#pragma comment (lib,    "comctl32.lib")
#pragma comment (lib,    "msi.lib"     )
#pragma comment (lib,    "version.lib" )

#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")

#include <cstdint>

// TODO: reference additional headers your program requires here
