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

HMODULE hModGlobal = 0;//LoadLibrary (L"SpecialK64.dll");

#include "injection.h"
#include "system_tray.h"
#include "SKIM.h"

SKX_RemoveCBTHook_pfn   SKX_RemoveCBTHook   = nullptr;
SKX_InstallCBTHook_pfn  SKX_InstallCBTHook  = nullptr;
SKX_IsHookingCBT_pfn    SKX_IsHookingCBT    = nullptr;
SKX_GetInjectedPIDs_pfn SKX_GetInjectedPIDs = nullptr;

HMODULE
SKIM_GlobalInject_Load (void)
{
  if (hModGlobal == 0)
    hModGlobal = LoadLibraryW (L"SpecialK64.dll");

  if (hModGlobal != 0)
  {
    SKX_RemoveCBTHook   = (SKX_RemoveCBTHook_pfn)
      GetProcAddress      (hModGlobal, "SKX_RemoveCBTHook");

    SKX_InstallCBTHook  = (SKX_InstallCBTHook_pfn)
      GetProcAddress      (hModGlobal, "SKX_InstallCBTHook");

    SKX_IsHookingCBT    = (SKX_IsHookingCBT_pfn)
      GetProcAddress      (hModGlobal, "SKX_IsHookingCBT");

    SKX_GetInjectedPIDs = (SKX_GetInjectedPIDs_pfn)
      GetProcAddress      (hModGlobal, "SKX_GetInjectedPIDs");
  }

  return hModGlobal;
}

BOOL
SKIM_GlobalInject_Free (void)
{
  if (hModGlobal != 0)
  {
    if (FreeLibrary (hModGlobal))
      hModGlobal = 0;
  }

  if (hModGlobal == 0)
  {
    SKX_RemoveCBTHook   = nullptr;
    SKX_InstallCBTHook  = nullptr;
    SKX_IsHookingCBT    = nullptr;
    SKX_GetInjectedPIDs = nullptr;
  }

  if (hModGlobal == 0)
    return TRUE;

  while (! FreeLibrary (hModGlobal))
      ;

  hModGlobal = 0;

  return TRUE;
}

bool
SKIM_GlobalInject_Start (void)
{
  if (SKIM_GlobalInject_Load ())
  {
    if (GetFileAttributes (L"SpecialK32.dll") != INVALID_FILE_ATTRIBUTES)
    {
      if (GetFileAttributes (L"SpecialK32.pid") == INVALID_FILE_ATTRIBUTES)
      {
        wchar_t wszPath     [MAX_PATH * 2] = { };
        wchar_t wszFullPath [MAX_PATH * 2] = { };

        GetCurrentDirectoryW     (MAX_PATH * 2 - 1, wszPath);
        GetSystemWow64DirectoryW (wszFullPath, MAX_PATH * 2 -1);

        PathAppendW   (wszFullPath, L"rundll32.exe");
        ShellExecuteW (nullptr, L"open", wszFullPath, L"SpecialK32.dll,RunDLL_InjectionManager Install", wszPath, SW_HIDE);

        while (GetFileAttributes (L"SpecialK32.pid") == INVALID_FILE_ATTRIBUTES)
        {
          Sleep (133UL);
        }
      }
    }

    if (! SKX_IsHookingCBT ())
    {
      SKX_InstallCBTHook ();

      if (SKX_IsHookingCBT ())
        return true;
    }
  }

  return false;
}

#include <shobjidl.h>
#include <shlguid.h>
#include <atlbase.h>

bool
SKIM_GetProgramsDir (wchar_t* buf, uint32_t* pdwLen)
{
  CComHeapPtr <wchar_t> str;
  CHandle               hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return false;

  if ( SUCCEEDED (
         SHGetKnownFolderPath (
           FOLDERID_Programs, 0, hToken, &str
         )
       )
     )
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0) {
      wcsncpy (buf, str, *pdwLen);
    }

    return true;
  }

  return false;
}

bool
SKIM_GetStartupDir (wchar_t* buf, uint32_t* pdwLen)
{
  CComHeapPtr <wchar_t> str;
  CHandle               hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken.m_h))
    return false;

  if ( SUCCEEDED (
         SHGetKnownFolderPath (
           FOLDERID_Startup, 0, hToken, &str
         )
       )
     )
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0) {
      wcsncpy (buf, str, *pdwLen);
    }

    return true;
  }

  return false;
}

std::wstring
SKIM_GetStartupShortcut (void)
{
  wchar_t wszLink [MAX_PATH * 2] = { };
  DWORD    dwLen = MAX_PATH * 2 - 1;

  SKIM_GetStartupDir (wszLink, (uint32_t *)&dwLen);

  PathAppend (wszLink, L"SKIM64.lnk");

  return wszLink;
}

std::wstring
SKIM_GetStartMenuShortcut (void)
{
  wchar_t wszLink [MAX_PATH * 2] = { };
  DWORD    dwLen = MAX_PATH * 2 - 1;

  SKIM_GetProgramsDir (wszLink, (uint32_t *)&dwLen);

  PathAppend (wszLink, L"Special K\\SKIM64.lnk");

  return wszLink;
}

bool
SKIM_IsLaunchedAtStartup (void)
{
  std::wstring link_file = SKIM_GetStartupShortcut ();

  HRESULT              hr  = E_FAIL; 
  CComPtr <IShellLink> psl = nullptr;

  hr = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

  if (SUCCEEDED (hr))
  {
    CComPtr <IPersistFile> ppf = nullptr;

    hr = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);

    if (SUCCEEDED (hr))
    {
      hr = ppf->Load (link_file.c_str (), STGM_READ);

      if (SUCCEEDED (hr))
      {
        return true;
      }
    }
  }

  return false;
}

bool
SKIM_SetStartupInjection (bool enable, wchar_t* wszExecutable)
{
  if (enable && (! SKIM_IsLaunchedAtStartup ()))
  {
    std::wstring link_file = SKIM_GetStartupShortcut ();

    HRESULT              hr  = E_FAIL; 
    CComPtr <IShellLink> psl = nullptr;

    hr = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (SUCCEEDED (hr))
    {
      std::unique_ptr <wchar_t> work_dir (_wcsdup (wszExecutable));
      PathRemoveFileSpecW (work_dir.get ());

      psl->SetShowCmd          (SW_NORMAL);
      psl->SetPath             (wszExecutable);
      psl->SetWorkingDirectory (work_dir.get ());
      psl->SetDescription      (L"Start Special K Injection with Windows");
      psl->SetArguments        (L"+Inject");
      psl->SetIconLocation     (wszExecutable, 1);

      CComPtr <IPersistFile> ppf = nullptr;

      hr = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);

      if (SUCCEEDED (hr))
      {
        hr = ppf->Save (link_file.c_str (), TRUE);

        if (SUCCEEDED (hr))
        {
          SKIM_Tray_UpdateStartup ();
          return true;
        }
      }
    }

    return false;
  }

  else if ((! enable) && SKIM_IsLaunchedAtStartup ())
  {
    bool ret = DeleteFileW (SKIM_GetStartupShortcut ().c_str ());

    SKIM_Tray_UpdateStartup ();

    return ret;
  }

  else
  {
    SKIM_Tray_UpdateStartup ();
    return true;
  }
}

bool
SKIM_SetStartMenuLink (bool enable, wchar_t* wszExecutable)
{
  if (enable)
  {
    std::wstring link_file = SKIM_GetStartMenuShortcut ();

    SKIM_Util_CreateDirectories (link_file.c_str ());

    HRESULT              hr  = E_FAIL; 
    CComPtr <IShellLink> psl = nullptr;

    hr = CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (SUCCEEDED (hr))
    {
      std::unique_ptr <wchar_t> work_dir (_wcsdup (wszExecutable));
      PathRemoveFileSpecW (work_dir.get ());

      psl->SetShowCmd          (SW_NORMAL);
      psl->SetPath             (wszExecutable);
      psl->SetWorkingDirectory (work_dir.get ());
      psl->SetDescription      (L"Special K Install Manager");
      psl->SetArguments        (L"");
      psl->SetIconLocation     (wszExecutable, 0);

      CComPtr <IPersistFile> ppf = nullptr;

      hr = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);

      if (SUCCEEDED (hr))
      {
        hr = ppf->Save (link_file.c_str (), TRUE);

        if (SUCCEEDED (hr))
        {
          return true;
        }
      }
    }

    return false;
  }

  else if ((! enable))
  {
    bool ret =
      DeleteFileW (SKIM_GetStartMenuShortcut ().c_str ());

    if (ret)
    {
      std::unique_ptr <wchar_t> start_menu_path (
        _wcsdup (SKIM_GetStartMenuShortcut ().c_str ())
      );

            PathRemoveFileSpec (start_menu_path.get ());
      ret = RemoveDirectoryW   (start_menu_path.get ());
    }

    return ret;
  }

  return false;
}

bool
SKIM_GlobalInject_Stop (bool confirm)
{
  if (GetFileAttributes (L"SpecialK32.dll") != INVALID_FILE_ATTRIBUTES)
  {
    if (GetFileAttributes (L"SpecialK32.pid") != INVALID_FILE_ATTRIBUTES)
    {
      wchar_t wszPath     [MAX_PATH * 2] = { };
      wchar_t wszFullPath [MAX_PATH * 2] = { };

      GetCurrentDirectoryW     (MAX_PATH * 2 - 1, wszPath);
      GetSystemWow64DirectoryW (wszFullPath, MAX_PATH * 2 -1);

      PathAppendW   (wszFullPath, L"rundll32.exe");
      ShellExecuteW (nullptr, L"open", wszFullPath, L"SpecialK32.dll,RunDLL_InjectionManager Remove", wszPath, SW_HIDE);

      int tries = 0;

      while (GetFileAttributes (L"SpecialK32.pid") != INVALID_FILE_ATTRIBUTES)
      {
        Sleep (133UL);
        ++tries;

        if (tries > 3)
          DeleteFileW (L"SpecialK32.pid");
      }
    }
  }

  if (SKIM_GlobalInject_Load ())
  {
    if (SKX_IsHookingCBT ())
    {
      SKX_RemoveCBTHook ();

      //return false;
    }
  }

  return true;
}

bool
SKIM_GlobalInject_Stop (HWND hWndDlg, bool confirm)
{
  if (SKIM_GlobalInject_Stop (confirm))
  {
    SetWindowText (GetDlgItem (hWndDlg, IDC_MANAGE_CMD), L"Start Injecting");

    SKIM_Tray_Stop ();

    return true;
  }

  return false;
}

bool
SKIM_GlobalInject_Start (HWND hWndDlg)
{
  if (SKIM_GlobalInject_Start ())
  {
    SetWindowText (GetDlgItem (hWndDlg, IDC_MANAGE_CMD), L"Stop Injecting");

    SKIM_Tray_Start ();

    return true;
  }

  return false;
}

bool
SKIM_GlobalInject_StartStop (HWND hWndDlg, bool confirm)
{
  if (SKIM_GetInjectorState ())
  {
    return SKIM_GlobalInject_Stop (hWndDlg, confirm);
  }

  else
  {
    return SKIM_GlobalInject_Start (hWndDlg);
  }
}

size_t
SKIM_SummarizeInjectedPIDs (std::wstring& out)
{
  size_t count = SKX_GetInjectedPIDs ?
                   SKX_GetInjectedPIDs (nullptr, 0) : 0;

  DWORD   dwPIDs      [128]      = { };
  wchar_t wszFileName [MAX_PATH] = { };

  if (SKX_GetInjectedPIDs)
  {
    SKX_GetInjectedPIDs (dwPIDs, count + 1);

    for (int i = 0; i < count; i++)
    {
      CHandle hProc (
        OpenProcess ( PROCESS_QUERY_INFORMATION,
                        FALSE,
                          dwPIDs [i] )
      );

      if (hProc != NULL)
      {
        DWORD dwLen = MAX_PATH;
        QueryFullProcessImageName (hProc, 0x0, wszFileName, &dwLen);

        PathStripPath (wszFileName);

        out += L"\n  ";
        out += wszFileName;
      }
    }
  }

  return count;
}

void
SKIM_StopInjectingAndExit (HWND hWndDlg, bool confirm)
{
  if (SKIM_GetInjectorState ())
  {
    SKIM_GlobalInject_Stop (hWndDlg, confirm);
  }

  SKIM_Exit ();
}

// 0 = Removed
// 1 = Installed
int
SKIM_GetInjectorState (void)
{
#ifdef _WIN64
  HMODULE hMod = LoadLibrary (L"SpecialK64.dll");
#else
  HMODULE hMod = LoadLibrary (L"SpecialK32.dll");
#endif

  typedef bool   (WINAPI *SKX_IsHookingCBT_pfn)   (void);
  
  if (hMod != NULL)
  {
    SKX_IsHookingCBT_pfn SKX_IsHookingCBT =
      (SKX_IsHookingCBT_pfn)GetProcAddress   (hMod, "SKX_IsHookingCBT");

    int ret = 0;

    if (SKX_IsHookingCBT != nullptr)
    {
      ret = SKX_IsHookingCBT ( ) ? 1 : 0;
    }

    FreeLibrary (hMod);
  
    return ret;
  }

  return 0;
}