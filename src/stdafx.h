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

#define _INC_MMSYSTEM

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

#pragma comment (lib,    "shlwapi.lib")
#pragma comment (lib,    "shell32.lib")
#pragma comment (lib,    "Ole32.lib")
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")

#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")

#include <cstdint>



typedef DWORD (WINAPI *timeGetTime_pfn)(void);

extern timeGetTime_pfn           timeGetTime;


typedef enum tagINSTALLSTATE
{
	INSTALLSTATE_NOTUSED      = -7,  // component disabled
	INSTALLSTATE_BADCONFIG    = -6,  // configuration data corrupt
	INSTALLSTATE_INCOMPLETE   = -5,  // installation suspended or in progress
	INSTALLSTATE_SOURCEABSENT = -4,  // run from source, source is unavailable
	INSTALLSTATE_MOREDATA     = -3,  // return buffer overflow
	INSTALLSTATE_INVALIDARG   = -2,  // invalid function argument
	INSTALLSTATE_UNKNOWN      = -1,  // unrecognized product or feature
	INSTALLSTATE_BROKEN       =  0,  // broken
	INSTALLSTATE_ADVERTISED   =  1,  // advertised feature
	INSTALLSTATE_REMOVED      =  1,  // component being removed (action state, not settable)
	INSTALLSTATE_ABSENT       =  2,  // uninstalled (or action state absent but clients remain)
	INSTALLSTATE_LOCAL        =  3,  // installed on local drive
	INSTALLSTATE_SOURCE       =  4,  // run from source, CD or net
	INSTALLSTATE_DEFAULT      =  5,  // use default, local or source
} INSTALLSTATE;

typedef UINT (WINAPI *MsiEnumRelatedProductsW_pfn)(
  _In_  LPCWSTR lpUpgradeCode,
  _In_  DWORD   dwReserved,
  _In_  DWORD   iProductIndex,
  _Out_ LPWSTR  lpProductBuf
);

typedef INSTALLSTATE (WINAPI *MsiQueryProductStateW_pfn)(
  _In_ LPCWSTR wszProduct
);

extern MsiEnumRelatedProductsW_pfn MsiEnumRelatedProductsW;
extern MsiQueryProductStateW_pfn   MsiQueryProductStateW;


#define FILE_VER_GET_LOCALISED  0x01
#define FILE_VER_GET_NEUTRAL    0x02
#define FILE_VER_GET_PREFETCHED 0x04

typedef BOOL (WINAPI *VerQueryValueW_pfn)(
 _In_  LPCVOID  pBlock,
 _In_  LPCWSTR  lpSubBlock,
 _Out_ LPVOID  *lplpBuffer,
 _Out_ PUINT    puLen
);

typedef BOOL (WINAPI *GetFileVersionInfoExW_pfn)(
  _In_       DWORD   dwFlags,
  _In_       LPCWSTR lptstrFilename,
  _Reserved_ DWORD   dwHandle,
  _In_       DWORD   dwLen,
  _Out_      LPVOID  lpData
);

extern VerQueryValueW_pfn        VerQueryValueW;
extern GetFileVersionInfoExW_pfn GetFileVersionInfoExW;


// TODO: reference additional headers your program requires here
