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

#include "system_tray.h"
#include "injection.h"

#include "APP_VERSION.H"

extern HINSTANCE g_hInstance;

// {012BFDBD-790D-4A7B-9BC4-A2632D2569D9}
static const GUID SKIM_SystemTray_UUID = 
{ 0x12bfdbd, 0x790d, 0x4a7b, { 0x9b, 0xc4, 0xa2, 0x63, 0x2d, 0x25, 0x69, 0xd9 } };

       HICON           hIconSKIM_Tray;
static HMENU           hTrayMenu       = 0;
static NOTIFYICONDATAW sys_tray_icon   = { };


HICON
SKIM_GetSmallStockIcon (SHSTOCKICONID siid)
{
  SHSTOCKICONINFO sii = { 0 };
           sii.cbSize = sizeof (sii);

  SHGetStockIconInfo ( siid,
                         SHGSI_ICON | SHGSI_SMALLICON | SHGFI_SELECTED,
                           &sii );

  return sii.hIcon;
}

void
SKIM_Tray_RefreshMenu (HWND hWndDlg, bool add)
{
  sys_tray_icon.hIcon        = hIconSKIM_Tray;
  sys_tray_icon.hBalloonIcon = hIconSKIM_Tray;
  sys_tray_icon.hWnd         = hWndDlg;
  sys_tray_icon.guidItem     = SKIM_SystemTray_UUID;
  sys_tray_icon.uFlags      |= NIF_ICON | NIF_TIP | NIF_MESSAGE |
                               /*NIF_INFO |*/ NIF_GUID | NIF_SHOWTIP;

  //wcscpy (sys_tray_icon.szInfoTitle, L"Special K Install Manager ");
  //wcscat (sys_tray_icon.szInfoTitle, SKIM_VERSION_STR_W);

  sys_tray_icon.uCallbackMessage = WM_USER | 0x0420;
  sys_tray_icon.uVersion         = NOTIFYICON_VERSION;
  sys_tray_icon.dwInfoFlags     |= NIIF_INFO | NIIF_LARGE_ICON | NIIF_RESPECT_QUIET_TIME;


  static std::wstring inject_summary;
  inject_summary = L"";

  size_t count =
    SKIM_SummarizeInjectedPIDs (inject_summary);

  if (count == 0)
    inject_summary += L"Special K Install Manager v " SKIM_VERSION_STR_W;
  else
  {
    inject_summary = std::wstring (L"Global Injection (SpecialK64.dll)") + inject_summary;
  }

  wcsncpy (sys_tray_icon.szTip, inject_summary.c_str (), 127);

  static HBITMAP hIconExit          = 0;
  static HBITMAP hIconStart         = 0;
  static HBITMAP hIconStop          = 0;
  static HBITMAP hIconStartAtLaunch = 0;
  
  auto LoadAndApplyBitmap = [](UINT resid, int idx, HMENU hMenu) ->
  HBITMAP
  {
    HBITMAP       hBitmap = LoadBitmap (g_hInstance, MAKEINTRESOURCE (resid));
    MENUITEMINFOW minfo;

    minfo.cbSize = sizeof MENUITEMINFOW;
  
    GetMenuItemInfoW (hMenu, idx, TRUE, &minfo);
  
    minfo.fMask    = MIIM_BITMAP | MIIM_FTYPE;
    minfo.fType    = MFT_STRING;
    
    minfo.hbmpItem = hBitmap;
  
    SetMenuItemInfoW (hMenu, idx, TRUE, &minfo);
  
    return hBitmap;
  };
  
  
  if (hIconExit          == 0) { hIconExit          = LoadAndApplyBitmap (IDB_EXIT,    6, hTrayMenu); }
  if (hIconStart         == 0) { hIconStart         = LoadAndApplyBitmap (IDB_START,   3, hTrayMenu); }
  if (hIconStop          == 0) { hIconStop          = LoadAndApplyBitmap (IDB_STOP,    4, hTrayMenu); }
  //if (hIconStartAtLaunch == 0) { hIconStartAtLaunch = LoadAndApplyIcon (IDI_STARTUP, 1, hTrayMenu); }


  if (add)
    Shell_NotifyIcon (NIM_ADD,      &sys_tray_icon);
  else
    Shell_NotifyIcon (NIM_MODIFY,   &sys_tray_icon);


  Shell_NotifyIcon (NIM_SETVERSION, &sys_tray_icon);


  RedrawWindow (hWndDlg, nullptr, NULL, 0x00);
}

void
SKIM_Tray_SendTo (HWND hWndDlg)
{
  SKIM_Tray_RefreshMenu (hWndDlg, true);

  ShowWindow   (hWndDlg, SW_HIDE);
  RedrawWindow (hWndDlg, nullptr, NULL, 0x00);
}

void
SKIM_Tray_RestoreFrom (HWND hWndDlg)
{
  //Shell_NotifyIcon (NIM_DELETE, &sys_tray_icon);

  ShowWindow (hWndDlg, SW_RESTORE);
}



void
SKIM_Tray_Init (HWND hWndDlg)
{
  if (hTrayMenu == 0)
  {
    hIconSKIM_Tray  = LoadIcon  (g_hInstance, MAKEINTRESOURCE (IDI_TRAY));
    hTrayMenu       = CreatePopupMenu ();

    MENUINFO minfo = { };

    minfo.cbSize = sizeof MENUINFO;
    minfo.fMask  = MIM_STYLE;

    GetMenuInfo (hTrayMenu, &minfo);

    minfo.dwStyle |= MNS_NOTIFYBYPOS;
    minfo.fMask    = MIM_STYLE;

    SetMenuInfo (hTrayMenu, &minfo);
  
    InsertMenuW (hTrayMenu, 0, MF_BYCOMMAND   | MF_DISABLED |
                               MF_MOUSESELECT | MF_RIGHTJUSTIFY, 0, L"Global Injection" );
    InsertMenuW (hTrayMenu, 1, MF_BYCOMMAND  | MF_STRING,        1, L"Start With Windows");

    InsertMenuW (hTrayMenu, 2, MF_SEPARATOR,                     2, nullptr);

    InsertMenuW (hTrayMenu, 3, MF_BYCOMMAND | MF_STRING,         3, L"  Start");
    InsertMenuW (hTrayMenu, 4, MF_BYCOMMAND | MF_STRING,         4, L"  Stop");

    InsertMenuW (hTrayMenu, 5, MF_SEPARATOR,                     5, nullptr);
    InsertMenuW (hTrayMenu, 6, MF_BYCOMMAND | MF_STRING,         6, L"Exit");

    SetMenuDefaultItem (hTrayMenu, 0, TRUE);
    HiliteMenuItem     (hWndDlg, hTrayMenu, 0, MF_BYPOSITION | MF_UNHILITE);

    MENUITEMINFOW minfo_start = { },
                  minfo_stop  = { };

    minfo_start.cbSize   = sizeof MENUITEMINFOW;
    minfo_stop.cbSize    = sizeof MENUITEMINFOW;

    GetMenuItemInfoW (hTrayMenu, 3, FALSE, &minfo_start);
    GetMenuItemInfoW (hTrayMenu, 4, FALSE, &minfo_stop);
    
    if (SKIM_GetInjectorState ())
    {
      minfo_start.fMask  = MIIM_STATE;
      minfo_start.fState = MFS_DISABLED;
    
      SetMenuItemInfoW (hTrayMenu, 3, FALSE, &minfo_start);
    
      minfo_stop.fMask  = MIIM_STATE;
      minfo_stop.fState = MFS_ENABLED;
    
      SetMenuItemInfoW (hTrayMenu, 4, FALSE, &minfo_stop);
    }
    
    else
    {
      minfo_start.fMask  = MIIM_STATE;
      minfo_start.fState = MFS_ENABLED;
    
      SetMenuItemInfoW (hTrayMenu, 3, FALSE, &minfo_start);
    
      minfo_stop.fMask  = MIIM_STATE;
      minfo_stop.fState = MFS_DISABLED;
    
      SetMenuItemInfoW (hTrayMenu, 4, FALSE, &minfo_stop);
    }

    SKIM_Tray_RefreshMenu (hWndDlg, true);
  }
}

void
SKIM_Tray_RemoveFrom (void)
{
  Shell_NotifyIcon (NIM_DELETE, &sys_tray_icon);
}

void
SKIM_Tray_UpdateStartup (void)
{
  MENUITEMINFOW minfo_startup = { };

  minfo_startup.cbSize = sizeof MENUITEMINFOW;

  GetMenuItemInfoW (hTrayMenu, 1, FALSE, &minfo_startup);
  
  if (SKIM_IsLaunchedAtStartup ())
  {
    minfo_startup.fMask  = MIIM_STATE;
    minfo_startup.fState = MFS_CHECKED | MFS_ENABLED;
  }
  else
  {
    minfo_startup.fMask  = MIIM_STATE;
    minfo_startup.fState = MFS_UNCHECKED | MFS_ENABLED;
  }
  
  SetMenuItemInfoW (hTrayMenu, 1, FALSE, &minfo_startup);
}

void
SKIM_Tray_Stop (void)
{
  MENUITEMINFOW minfo_start = { },
                minfo_stop  = { };

  minfo_start.cbSize = sizeof MENUITEMINFOW;
  minfo_stop.cbSize  = sizeof MENUITEMINFOW;

  GetMenuItemInfoW (hTrayMenu, 3, TRUE, &minfo_start);
  GetMenuItemInfoW (hTrayMenu, 4, TRUE, &minfo_stop);

  minfo_start.fMask  = MIIM_STATE;
  minfo_start.fState = MFS_ENABLED;
  
  SetMenuItemInfoW (hTrayMenu, 3, TRUE, &minfo_start);
  
  minfo_stop.fMask  = MIIM_STATE;
  minfo_stop.fState = MFS_DISABLED;
  
  SetMenuItemInfoW (hTrayMenu, 4, TRUE, &minfo_stop);
}

void
SKIM_Tray_Start (void)
{
  MENUITEMINFOW minfo_start = { },
                minfo_stop  = { };

  minfo_start.cbSize = sizeof MENUITEMINFOW;
  minfo_stop.cbSize  = sizeof MENUITEMINFOW;

  GetMenuItemInfoW (hTrayMenu, 3, TRUE, &minfo_start);
  GetMenuItemInfoW (hTrayMenu, 4, TRUE, &minfo_stop);

  minfo_start.fMask  = MIIM_STATE;
  minfo_start.fState = MFS_DISABLED;

  SetMenuItemInfoW (hTrayMenu, 3, TRUE, &minfo_start);

  minfo_stop.fMask  = MIIM_STATE;
  minfo_stop.fState = MFS_ENABLED;

  SetMenuItemInfoW (hTrayMenu, 4, TRUE, &minfo_stop);
}

void
SKIM_Tray_ProcessCommand (HWND hWndDlg, LPARAM lParam, WPARAM wParam)
{
  if ((HMENU)lParam == hTrayMenu)
  {
    // Magic Numbers Suck, STOP THIS!
    //
    switch (LOWORD (wParam))
    {
      case 0:
        SetMenuDefaultItem (hTrayMenu,          0, TRUE);
        HiliteMenuItem     (hWndDlg, hTrayMenu, 0, MF_BYPOSITION | MF_UNHILITE);
        break;

      case 1:
      {
        wchar_t wszExec [MAX_PATH * 2] = { };
        DWORD   dwLen  = MAX_PATH * 2 - 1;

        GetCurrentDirectoryW (dwLen, wszExec);
        PathAppend           (wszExec, L"SKIM64.exe");

        SKIM_SetStartupInjection (SKIM_IsLaunchedAtStartup () ? false : true, wszExec);
      } break;

      // Start
      case 3:
      {
        SKIM_GlobalInject_Start (hWndDlg);
      } break;

      // Stop
      case 4:
      {
        SKIM_GlobalInject_Stop (hWndDlg);
      } break;

      // Exit
      case 6:
      {
        if (SKIM_GlobalInject_Stop (hWndDlg))
          SKIM_StopInjectingAndExit (hWndDlg);
      } break;

      // INVALID
      default:
      {
      } break;
    }
  }
}

void
SKIM_Tray_HandleContextMenu (HWND hWndDlg)
{
  // Show that thing!
  SetForegroundWindow (hWndDlg);
  
  // get Mouse Position
  POINT pt;
  GetCursorPos (&pt);
  
  // Show Menu
  TrackPopupMenu ( hTrayMenu,
                     TPM_LEFTALIGN | TPM_LEFTBUTTON |
                     TPM_VERNEGANIMATION,
                       pt.x, pt.y,
                         0,
                           hWndDlg, nullptr );
}