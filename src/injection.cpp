#if 1
#include "stdafx.h"

#include "resource.h"

#include <string>
#include <memory>
#include <algorithm>

HMODULE hModGlobal = nullptr;//LoadLibrary (L"SpecialK64.dll");

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
  hModGlobal = GetModuleHandleW (L"SpecialK64.dll");

  if (hModGlobal == nullptr)
    hModGlobal = LoadLibraryW (L"SpecialK64.dll");

  if (hModGlobal != nullptr)
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
  if (hModGlobal != nullptr)
  {
    if (FreeLibrary (hModGlobal))
      hModGlobal = nullptr;
  }

  if (hModGlobal == nullptr)
  {
    SKX_RemoveCBTHook   = nullptr;
    SKX_InstallCBTHook  = nullptr;
    SKX_IsHookingCBT    = nullptr;
    SKX_GetInjectedPIDs = nullptr;
  }

  if (hModGlobal == nullptr)
    return TRUE;

  //while (! FreeLibrary (hModGlobal))
  //    ;

  hModGlobal = nullptr;

  return TRUE;
}

bool
SKIM_GlobalInject_Start (void)
{
  CoInitializeEx (nullptr, COINIT_MULTITHREADED);

  if (SKIM_GlobalInject_Load ())
  {
    wchar_t wszInjectionCacheLock [MAX_PATH + 2] = { };

                                              uint32_t dwStrLen = MAX_PATH;
    SKIM_Util_GetDocumentsDir (wszInjectionCacheLock, &dwStrLen);

    PathAppendW (wszInjectionCacheLock, L"My Mods\\SpecialK\\Global\\injection.ini.lock");
    DeleteFileW (wszInjectionCacheLock);

    if (GetFileAttributes (L"SpecialK32.dll") != INVALID_FILE_ATTRIBUTES)
    {
      // Wait / clear the 32-bit injector's PID file (it might be leftover from an unclean reboot)
      //
      if (GetFileAttributes (L"SpecialK32.pid") != INVALID_FILE_ATTRIBUTES)
        SKIM_GlobalInject_Stop (false);

      if (GetFileAttributes (L"SpecialK32.pid") == INVALID_FILE_ATTRIBUTES)
      {
        wchar_t wszPath     [MAX_PATH * 2] = { };
        wchar_t wszFullPath [MAX_PATH * 2] = { };

        GetCurrentDirectoryW     (MAX_PATH * 2 - 1, wszPath);
        GetSystemWow64DirectoryW (wszFullPath, MAX_PATH * 2 -1);

        PathAppendW   (wszFullPath, L"rundll32.exe");
        ShellExecuteW (nullptr, L"open", wszFullPath,
                       L"SpecialK32.dll,RunDLL_InjectionManager Install", wszPath, SW_HIDE);

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

  hr = CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

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

    hr = CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

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

    hr = CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

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
  UNREFERENCED_PARAMETER (confirm);

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

    for (size_t i = 0; i < count; i++)
    {
      CHandle hProc (
        OpenProcess ( PROCESS_QUERY_INFORMATION,
                        FALSE,
                          dwPIDs [i] )
      );

      if (hProc != nullptr)
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

  using SKX_IsHookingCBT_pfn = bool (WINAPI *)(void);
  
  if (hMod != nullptr)
  {
    if (! SKX_IsHookingCBT)
      SKX_IsHookingCBT =
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


#else
#include "stdafx.h"

#include "resource.h"

#include <string>
#include <memory>
#include <algorithm>

HMODULE hModGlobal = 0;//LoadLibrary (L"SpecialK64.dll");

#include "injection.h"
#include "system_tray.h"
#include "SKIM.h"

SKX_RemoveCBTHook_pfn   SKX_RemoveCBTHook   = nullptr;
SKX_InstallCBTHook_pfn  SKX_InstallCBTHook  = nullptr;
SKX_IsHookingCBT_pfn    SKX_IsHookingCBT    = nullptr;
SKX_GetInjectedPIDs_pfn SKX_GetInjectedPIDs = nullptr;

CRITICAL_SECTION cs_gi = { };

BOOL
SKIM_GlobalInject_Free (void);

HMODULE
SKIM_GlobalInject_Load (void)
{
  static bool run_once = false;

  if (! run_once)
  {
    InitializeCriticalSectionAndSpinCount (&cs_gi, 500);
    run_once = true;
  }

  EnterCriticalSection (&cs_gi);

  hModGlobal = GetModuleHandleW (L"SpecialK64.dll");

  if (! hModGlobal)
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

  LeaveCriticalSection (&cs_gi);

  return hModGlobal;
}

BOOL
SKIM_GlobalInject_Free (void)
{
  EnterCriticalSection (&cs_gi);

  if (GetModuleHandleW (L"SpecialK64.dll"))
  {
    if ( FreeLibrary      (hModGlobal)        &&
         GetModuleHandleW (L"SpecialK64.dll") == nullptr )
    {
      hModGlobal = 0;
    }
  }

  else
  {
    hModGlobal = 0;
  }

  if (hModGlobal == 0)
  {
    SKX_RemoveCBTHook   = nullptr;
    SKX_InstallCBTHook  = nullptr;
    SKX_IsHookingCBT    = nullptr;
    SKX_GetInjectedPIDs = nullptr;
  }

  LeaveCriticalSection (&cs_gi);

  return (hModGlobal == 0);
}

bool
SKIM_GlobalInject_Start (void)
{
  bool bRet = false;
  CoInitializeEx (0, COINIT_MULTITHREADED);

  if (SKIM_GlobalInject_Load ())
  {
    wchar_t wszInjectionCacheLock [MAX_PATH + 2] = { };

                                              uint32_t dwStrLen = MAX_PATH;
    SKIM_Util_GetDocumentsDir (wszInjectionCacheLock, &dwStrLen);

    PathAppendW (wszInjectionCacheLock, L"My Mods\\SpecialK\\Global\\injection.ini.lock");
    DeleteFileW (wszInjectionCacheLock);

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
        bRet = true;
    }

    else
      bRet = true;
  }

  SKIM_GlobalInject_Free ();

  return bRet;
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
  bool bRet = true;

  UNREFERENCED_PARAMETER (confirm);

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

      if (SKX_IsHookingCBT ())
        bRet = false;
    }
  }

  SKIM_GlobalInject_Free ();

  return bRet;
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
  SKIM_GlobalInject_Load ();

  EnterCriticalSection (&cs_gi);

  size_t count = SKX_GetInjectedPIDs ?
                   SKX_GetInjectedPIDs (nullptr, 0) : 0;

  DWORD   dwPIDs      [128]          = { };
  wchar_t wszFileName [MAX_PATH + 2] = { };

  if (SKX_GetInjectedPIDs)
  {
    SKX_GetInjectedPIDs (dwPIDs, count + 1);

    for (size_t i = 0; i < count; i++)
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

  SKIM_GlobalInject_Free ();

  LeaveCriticalSection (&cs_gi);

  return count;
}

void
SKIM_StopInjectingAndExit (HWND hWndDlg, bool confirm)
{
  EnterCriticalSection (&cs_gi);

  if (SKIM_GetInjectorState ())
  {
    SKIM_GlobalInject_Stop (hWndDlg, confirm);
  }

  LeaveCriticalSection (&cs_gi);

  SKIM_Exit ();
}

// 0 = Removed
// 1 = Installed
int
SKIM_GetInjectorState (void)
{
  HMODULE hMod =
    SKIM_GlobalInject_Load ();

  EnterCriticalSection (&cs_gi);

  int ret = 0;

  if (hMod != nullptr)
  {
    if (! SKX_IsHookingCBT)
    {
      SKIM_GlobalInject_Free ();
      LeaveCriticalSection (&cs_gi);
      return SKIM_GetInjectorState ();
    }

    ret =
      SKX_IsHookingCBT () ? 1 : 0;

    SKIM_GlobalInject_Free ();
  }

  LeaveCriticalSection (&cs_gi);

  return ret;
}
#endif