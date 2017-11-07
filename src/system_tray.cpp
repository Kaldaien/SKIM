#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_NON_CONFORMING_SWPRINTFS

#pragma warning (disable: 4091)

#include "stdafx.h"

#include "resource.h"

#include <string>
#include <memory>
#include <algorithm>

#include "system_tray.h"
#include "injection.h"

#include "SKIM.h"

#include "APP_VERSION.H"

extern HINSTANCE g_hInstance;

// {012BFDBD-790D-4A7B-9BC4-A2632D2569D9}
static const GUID SKIM_SystemTray_UUID = 
{ 0x12bfdbd, 0x790d, 0x4a7b, { 0x9b, 0xc4, 0xa2, 0x63, 0x2d, 0x25, 0x69, 0xd9 } };

       HICON           hIconSKIM_Tray;
static HMENU           hTrayMenu              = 0;
static HMENU           hTrayProductSelectMenu = 0;
static NOTIFYICONDATAW sys_tray_icon          = { };

static HBITMAP hIconExit          = 0;
static HBITMAP hIconStart         = 0;
static HBITMAP hIconStop          = 0;
static HBITMAP hIconStartAtLaunch = 0;
static HBITMAP hIconMenu          = 0;

static HBITMAP hIconBackup        = 0;
static HBITMAP hIconBranch        = 0;
static HBITMAP hIconFolder        = 0;
static HBITMAP hIconInfo          = 0;
static HBITMAP hIconInstall       = 0;
static HBITMAP hIconLogs          = 0;
static HBITMAP hIconConfig        = 0;
static HBITMAP hIconReleaseNotes  = 0;
static HBITMAP hIconUninstall     = 0;
static HBITMAP hIconUpdate        = 0;
static HBITMAP hIconUtility       = 0;

auto ApplyBitmap =
[ ](HBITMAP hBitmap, UINT_PTR idx, HMENU hMenu)
{
  MENUITEMINFOW minfo = { };

  minfo.cbSize = sizeof MENUITEMINFOW;

  GetMenuItemInfoW (hMenu, (UINT)idx, FALSE, &minfo);

  minfo.fMask    = MIIM_BITMAP | MIIM_FTYPE;
  minfo.fType    = MFT_STRING;
  
  minfo.hbmpItem = hBitmap;

  SetMenuItemInfoW (hMenu, (UINT)idx, FALSE, &minfo);
};

auto LoadAndApplyBitmap = [&](UINT resid, UINT_PTR idx, HMENU hMenu) ->
HBITMAP
{
  HBITMAP      hBitmap = LoadBitmap (g_hInstance, MAKEINTRESOURCE (resid));
  ApplyBitmap (hBitmap, idx, hMenu);
  return       hBitmap;
};

static POINT last_menu_pt;

void
SKIM_Tray_ResetMenu (void)
{
  DestroyMenu (hTrayMenu);
  hTrayMenu = 0;

  DestroyMenu (hTrayProductSelectMenu);
  hTrayProductSelectMenu = 0;
}


HICON
SKIM_GetSmallStockIcon (SHSTOCKICONID siid)
{
  SHSTOCKICONINFO sii = { };
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
  
  if (hIconExit          == 0) { hIconExit          = LoadAndApplyBitmap (IDB_EXIT,    MENUCMD_EXIT,            hTrayMenu); }
  else                                                       ApplyBitmap (hIconExit,   MENUCMD_EXIT,            hTrayMenu);

  if (SKIM_FindProductByAppID (0)->install_state == 1)
  {
    if (hIconStart         == 0) { hIconStart         = LoadAndApplyBitmap (IDB_START,   MENUCMD_START_INJECTION, hTrayMenu); }
    else                                                       ApplyBitmap (hIconStart,  MENUCMD_START_INJECTION, hTrayMenu);

    if (hIconStop          == 0) { hIconStop          = LoadAndApplyBitmap (IDB_STOP,    MENUCMD_STOP_INJECTION,  hTrayMenu); }
    else                                                       ApplyBitmap (hIconStop,   MENUCMD_STOP_INJECTION,  hTrayMenu);
  }

  if (hIconMenu          == 0) { hIconMenu          = LoadAndApplyBitmap (IDB_MENU,    (UINT_PTR)hTrayProductSelectMenu,  hTrayMenu); }
  else                                                       ApplyBitmap (hIconMenu,   (UINT_PTR)hTrayProductSelectMenu,  hTrayMenu);

  //if (hIconStartAtLaunch == 0) { hIconStartAtLaunch = LoadAndApplyIcon (IDI_STARTUP, 1, hTrayMenu); }


  SKIM_Tray_UpdateStartup ();


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
SKIM_Tray_SetupProductMenu (sk_product_t* prod)
{
  int idx =
    SKIM_GetProductIdx (prod);

  if (prod->menus.hProductMenu != nullptr) DestroyMenu (prod->menus.hProductMenu);
  if (prod->menus.hUtilMenu    != nullptr) DestroyMenu (prod->menus.hUtilMenu);
  if (prod->menus.hBranchMenu  != nullptr) DestroyMenu (prod->menus.hBranchMenu);
  if (prod->menus.hFileMenu    != nullptr) DestroyMenu (prod->menus.hFileMenu);

  prod->menus.hProductMenu = CreatePopupMenu ();
  prod->menus.hUtilMenu    = CreatePopupMenu ();
  prod->menus.hBranchMenu  = CreatePopupMenu ();
  prod->menus.hFileMenu    = CreatePopupMenu ();

  if (prod->bHasDLC)
    AppendMenu (prod->menus.hUtilMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_DLC,    L"Manage DLC");

  if (wcslen (prod->wszConfigTool))
    AppendMenu (prod->menus.hUtilMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_CONFIG, L"Run Config Tool");

  AppendMenu (prod->menus.hProductMenu, MF_BYCOMMAND | MF_STRING | MF_DEFAULT, 128 * (idx+1), prod->wszProjectName);

  SetMenuDefaultItem (prod->menus.hProductMenu, 128 * (idx+1), FALSE);

  if (prod->install_state != 0)
  {
    int count =
      SKIM_CountProductBranches (prod);

    if (count > 1)
    {
      uint32_t prod_id = SKIM_BranchManager::singleton ()->getProduct ();
                         SKIM_BranchManager::singleton ()->setProduct (prod->uiSteamAppID);

      std::wstring current = SKIM_BranchManager::singleton ()->getCurrentBranch ()->name;

                         SKIM_BranchManager::singleton ()->setProduct (prod_id);

      for (int i = 0; i < count; i++)
      {
        SKIM_BranchManager::Branch* pBranch =
          SKIM_GetProductBranchByIdx ( prod, i );

        AppendMenu (prod->menus.hBranchMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + 96 + i, pBranch->name.c_str ());

        if (current == pBranch->name)
        {
          MENUITEMINFOW minfo_branch        = {                  };
                        minfo_branch.cbSize = sizeof MENUITEMINFOW;

          GetMenuItemInfoW (prod->menus.hBranchMenu, 128 * (idx+1) + 96 + i, FALSE, &minfo_branch);

          minfo_branch.fMask   = MIIM_STATE;
          minfo_branch.fState |= MFS_CHECKED;

          SetMenuItemInfoW (prod->menus.hBranchMenu, 128 * (idx+1) + 96 + i, FALSE, &minfo_branch);
        }
      }

      wchar_t   wszBranchSelect [MAX_PATH] = { };
      swprintf (wszBranchSelect, L"  Branch: %ws", current.c_str ());

      AppendMenu (prod->menus.hProductMenu, MF_POPUP | MF_STRING, (INT_PTR)prod->menus.hBranchMenu, wszBranchSelect);
      if (hIconBranch == 0) { hIconBranch = LoadAndApplyBitmap (IDB_BRANCH,  (INT_PTR)prod->menus.hBranchMenu, prod->menus.hProductMenu); }
      else                                         ApplyBitmap (hIconBranch, (INT_PTR)prod->menus.hBranchMenu, prod->menus.hProductMenu);


      AppendMenu (prod->menus.hProductMenu, MF_SEPARATOR,             0,                      nullptr);
    }

    //AppendMenu (prod->menus.hProductMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_RELNOTES,      L"  Release Notes");
    //if (hIconReleaseNotes == 0) { hIconReleaseNotes = LoadAndApplyBitmap (IDB_RELEASE_NOTES, 128 * (idx+1) + MENUCMD_RELNOTES, prod->menus.hProductMenu); }
    //else                                                     ApplyBitmap (hIconReleaseNotes, 128 * (idx+1) + MENUCMD_RELNOTES, prod->menus.hProductMenu);


    AppendMenu (prod->menus.hProductMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_CHECK_VERSION, L"  Check Version");
    if (hIconUpdate == 0) { hIconUpdate = LoadAndApplyBitmap (IDB_UPDATE, 128 * (idx+1) + MENUCMD_CHECK_VERSION, prod->menus.hProductMenu); }
    else                                         ApplyBitmap (hIconUpdate, 128 * (idx+1) + MENUCMD_CHECK_VERSION, prod->menus.hProductMenu);


    AppendMenu (prod->menus.hProductMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_UNINSTALL,     L"  Uninstall");
    if (hIconUninstall == 0) { hIconUninstall = LoadAndApplyBitmap (IDB_UNINSTALL,  128 * (idx+1) + MENUCMD_UNINSTALL, prod->menus.hProductMenu); }
    else                                               ApplyBitmap (hIconUninstall, 128 * (idx+1) + MENUCMD_UNINSTALL, prod->menus.hProductMenu);


    //AppendMenu (prod->menus.hFileMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_DIR_ASSETS, L"Browse Assets");
    AppendMenu (prod->menus.hFileMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_DIR_BACKUPS,L"Browse Backups");
    if (hIconBackup == 0) { hIconBackup = LoadAndApplyBitmap (IDB_BACKUP,  128 * (idx+1) + MENUCMD_DIR_BACKUPS, prod->menus.hFileMenu); }
    else                                         ApplyBitmap (hIconBackup, 128 * (idx+1) + MENUCMD_DIR_BACKUPS, prod->menus.hFileMenu);
                    
    AppendMenu (prod->menus.hFileMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_DIR_CONFIG, L"Browse Config");
    if (hIconConfig == 0) { hIconConfig = LoadAndApplyBitmap (IDB_CONFIG,  128 * (idx+1) + MENUCMD_DIR_CONFIG, prod->menus.hFileMenu); }
    else                                         ApplyBitmap (hIconConfig, 128 * (idx+1) + MENUCMD_DIR_CONFIG, prod->menus.hFileMenu);

    AppendMenu (prod->menus.hFileMenu, MF_BYCOMMAND | MF_STRING,       128 * (idx+1) + MENUCMD_DIR_LOGS, L"Browse Logs");
    if (hIconLogs == 0) { hIconLogs = LoadAndApplyBitmap (IDB_LOGS,    128 * (idx+1) + MENUCMD_DIR_LOGS, prod->menus.hFileMenu); }
    else                                     ApplyBitmap (hIconLogs, 128 * (idx+1) + MENUCMD_DIR_LOGS, prod->menus.hFileMenu);
  }
  else
  {
    AppendMenu (prod->menus.hProductMenu, MF_BYCOMMAND | MF_STRING, 128 * (idx+1) + MENUCMD_INSTALL,       L"  Install");
    //if (hIconInstall == 0) { hIconInstall = LoadAndApplyBitmap (IDB_INSTALL,  128 * (idx+1) + MENUCMD_INSTALL, prod->menus.hProductMenu); }
    //else                                           ApplyBitmap (hIconInstall, 128 * (idx+1) + MENUCMD_INSTALL, prod->menus.hProductMenu);
  }

  if (prod->install_state != 0)
    AppendMenu (prod->menus.hProductMenu, MF_SEPARATOR,             0,                      nullptr);

  if (prod->install_state != 0 && (prod->bHasDLC || wcslen (prod->wszConfigTool)))
  {
    AppendMenu (prod->menus.hProductMenu, MF_MOUSESELECT | MF_POPUP, (UINT_PTR)prod->menus.hUtilMenu, L" Utilities");
    if (hIconUtility == 0) { hIconUtility = LoadAndApplyBitmap (IDB_UTILS,    (UINT_PTR)prod->menus.hUtilMenu, prod->menus.hProductMenu); }
    else                                           ApplyBitmap (hIconUtility, (UINT_PTR)prod->menus.hUtilMenu, prod->menus.hProductMenu);
  }

  if (prod->install_state != 0)
  {
    AppendMenu (prod->menus.hProductMenu, MF_MOUSESELECT | MF_POPUP, (UINT_PTR)prod->menus.hFileMenu, L" Browse Files");
    if (hIconFolder == 0) { hIconFolder = LoadAndApplyBitmap (IDB_FOLDER,  (UINT_PTR)prod->menus.hFileMenu, prod->menus.hProductMenu); }
    else                                         ApplyBitmap (hIconFolder, (UINT_PTR)prod->menus.hFileMenu, prod->menus.hProductMenu);
  }


  //static HBITMAP hIconBackup        = 0;
  //static HBITMAP hIconBranch        = 0;
  //static HBITMAP hIconFolder        = 0;
  //static HBITMAP hIconInfo          = 0;
  //static HBITMAP hIconInstall       = 0;
  //static HBITMAP hIconLogs          = 0;
  //static HBITMAP hIconMenu          = 0;
  //static HBITMAP hIconReleaseNotes  = 0;
  //static HBITMAP hIconUninstall     = 0;
  //static HBITMAP hIconUpdate        = 0;
  //static HBITMAP hIconUtility       = 0;
  //
  //
  //
  //  if (hIconStart         == 0) { hIconStart         = LoadAndApplyBitmap (IDB_START,   MENUCMD_START_INJECTION, hTrayMenu); }
  //  else                                                       ApplyBitmap (hIconStart,  MENUCMD_START_INJECTION, hTrayMenu);
  //
  //  if (hIconStop          == 0) { hIconStop          = LoadAndApplyBitmap (IDB_STOP,    MENUCMD_STOP_INJECTION,  hTrayMenu); }
  //  else                                                       ApplyBitmap (hIconStop,   MENUCMD_STOP_INJECTION,  hTrayMenu);

}

void
SKIM_Tray_UpdateProduct (sk_product_t* prod)
{
  MENUITEMINFOW minfo_product        = { };
                minfo_product.cbSize = sizeof MENUITEMINFOW;

  GetMenuItemInfoW (hTrayMenu, (UINT)(UINT_PTR)hTrayProductSelectMenu, FALSE, &minfo_product);

  wchar_t wszProdName [256] = { };

  swprintf (wszProdName, L"Manage:  %s", prod->wszProjectName);

  minfo_product.fMask      = MIIM_STRING;
  minfo_product.fType      = MIIM_STRING;
  minfo_product.dwTypeData = wszProdName;
  minfo_product.cch        = (UINT)wcslen (wszProdName);
 
  SetMenuItemInfoW (hTrayMenu, (UINT)(UINT_PTR)hTrayProductSelectMenu, FALSE, &minfo_product);
}

void
SKIM_Tray_Init (HWND hWndDlg)
{
  if (hTrayMenu != 0)
  {
    DestroyMenu (hTrayMenu);
    hTrayMenu = 0;
  }

  if (hTrayMenu == 0)
  {
    hIconSKIM_Tray         = LoadIcon  (g_hInstance, MAKEINTRESOURCE (IDI_TRAY));
    hTrayMenu              = CreatePopupMenu ();
    hTrayProductSelectMenu = CreatePopupMenu ();

    MENUINFO minfo        = {             };
             minfo.cbSize = sizeof MENUINFO;
             minfo.fMask  = MIM_STYLE;

    GetMenuInfo (hTrayMenu, &minfo);

    minfo.dwStyle |= MNS_NOTIFYBYPOS;
    minfo.fMask    = MIM_STYLE;

    std::vector <sk_product_t *> prods =
      SKIM_GetInstallableProducts ();

    for ( auto& it : prods )
    {
      int idx =
        SKIM_GetProductIdx (it);

      SKIM_Tray_SetupProductMenu (it);

      AppendMenu  (hTrayProductSelectMenu, MF_BYCOMMAND | MF_STRING | MF_MOUSESELECT | MF_POPUP, 128 * ++idx, it->wszProjectName);
    }

    AppendMenu (hTrayMenu, MF_BYCOMMAND | MF_MOUSESELECT | MF_POPUP, (UINT_PTR)hTrayProductSelectMenu, L"Manage: ");

    if (SKIM_FindProductByAppID (0)->install_state == 1)
    {
      AppendMenu (hTrayMenu, MF_SEPARATOR,                     0, nullptr);

      AppendMenu (hTrayMenu, MF_BYCOMMAND   | MF_DISABLED     |
                             MF_MOUSESELECT, 1024, L"Global Injection" );

      AppendMenu (hTrayMenu, MF_BYCOMMAND  | MF_STRING, 
                             MENUCMD_INJECT_AT_BOOT, L"Start With Windows" );

      AppendMenu (hTrayMenu, MF_SEPARATOR,                     0, nullptr);

      AppendMenu (hTrayMenu, MF_BYCOMMAND | MF_STRING,
                             MENUCMD_START_INJECTION, L"  Start");
      AppendMenu (hTrayMenu, MF_BYCOMMAND | MF_STRING,
                             MENUCMD_STOP_INJECTION,  L"  Stop");
    }

    AppendMenu (hTrayMenu, MF_SEPARATOR,                     0, nullptr);

    AppendMenu (hTrayMenu, MF_BYCOMMAND | MF_STRING,
                            MENUCMD_EXIT,  L"  Exit");

    SetMenuDefaultItem (hTrayMenu, (UINT)(UINT_PTR)hTrayProductSelectMenu, FALSE);
    HiliteMenuItem     (hWndDlg, hTrayMenu, 0, MF_BYPOSITION | MF_UNHILITE);

    MENUITEMINFOW minfo_start = { },
                  minfo_stop  = { };

    minfo_start.cbSize   = sizeof MENUITEMINFOW;
    minfo_stop.cbSize    = sizeof MENUITEMINFOW;

    if (SKIM_FindProductByAppID (0)->install_state == 1)
    {
      GetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
      GetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION,  FALSE, &minfo_stop);
      
      if (SKIM_GetInjectorState ())
      {
        minfo_start.fMask  = MIIM_STATE;
        minfo_start.fState = MFS_DISABLED;
      
        SetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
      
        minfo_stop.fMask  = MIIM_STATE;
        minfo_stop.fState = MFS_ENABLED;
      
        SetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION, FALSE, &minfo_stop);
      }
      
      else
      {
        minfo_start.fMask  = MIIM_STATE;
        minfo_start.fState = MFS_ENABLED;
      
        SetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
      
        minfo_stop.fMask  = MIIM_STATE;
        minfo_stop.fState = MFS_DISABLED;
      
        SetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION, FALSE, &minfo_stop);
      }
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

  GetMenuItemInfoW (hTrayMenu, MENUCMD_INJECT_AT_BOOT, FALSE, &minfo_startup);
  
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
  
  SetMenuItemInfoW (hTrayMenu, MENUCMD_INJECT_AT_BOOT, FALSE, &minfo_startup);
}

void
SKIM_Tray_Stop (void)
{
  MENUITEMINFOW minfo_start = { },
                minfo_stop  = { };

  minfo_start.cbSize = sizeof MENUITEMINFOW;
  minfo_stop.cbSize  = sizeof MENUITEMINFOW;

  if (SKIM_FindProductByAppID (0)->install_state == 1)
  {
    GetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
    GetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION,  FALSE, &minfo_stop);

    minfo_start.fMask  = MIIM_STATE;
    minfo_start.fState = MFS_ENABLED;
    
    SetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
    
    minfo_stop.fMask  = MIIM_STATE;
    minfo_stop.fState = MFS_DISABLED;
    
    SetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION,  FALSE, &minfo_stop);
  }
}

void
SKIM_Tray_Start (void)
{
  MENUITEMINFOW minfo_start = { },
                minfo_stop  = { };

  minfo_start.cbSize = sizeof MENUITEMINFOW;
  minfo_stop.cbSize  = sizeof MENUITEMINFOW;

  if (SKIM_FindProductByAppID (0)->install_state == 1)
  {
    GetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);
    GetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION,  FALSE, &minfo_stop);

    minfo_start.fMask  = MIIM_STATE;
    minfo_start.fState = MFS_DISABLED;

    SetMenuItemInfoW (hTrayMenu, MENUCMD_START_INJECTION, FALSE, &minfo_start);

    minfo_stop.fMask  = MIIM_STATE;
    minfo_stop.fState = MFS_ENABLED;

    SetMenuItemInfoW (hTrayMenu, MENUCMD_STOP_INJECTION,  FALSE, &minfo_stop);
  }
}

void
SKIM_Tray_ProcessCommand (HWND hWndDlg, LPARAM lParam, WPARAM wParam)
{
  switch (LOWORD (wParam))
  {
    //case 0:
    //  SetMenuDefaultItem (hTrayMenu,          0, TRUE);
    //  HiliteMenuItem     (hWndDlg, hTrayMenu, 0, MF_BYPOSITION | MF_UNHILITE);
    //  break;

    case MENUCMD_INJECT_AT_BOOT:
    {
      wchar_t wszExec [MAX_PATH * 2] = { };
      DWORD   dwLen  = MAX_PATH * 2 - 1;

      GetCurrentDirectoryW (dwLen, wszExec);
      PathAppend           (wszExec, L"SKIM64.exe");

      SKIM_SetStartupInjection (SKIM_IsLaunchedAtStartup () ? false : true, wszExec);
    } break;

    // Start
    case MENUCMD_START_INJECTION:
    {
      SKIM_GlobalInject_Start (hWndDlg);
    } break;

    // Stop
    case MENUCMD_STOP_INJECTION:
    {
      SKIM_GlobalInject_Stop (hWndDlg);
    } break;

    // Exit
    case MENUCMD_EXIT:
    {
      //if (SKIM_GlobalInject_Stop (hWndDlg))
        SKIM_StopInjectingAndExit (hWndDlg);
    } break;

    // INVALID
    default:
    {
      if (wParam >= 128)
      {
        int idx = (INT)wParam / 128;
        int cmd = (wParam % 128);

        sk_product_t* prod = SKIM_GetProductByIdx (idx-1);
      
        extern int last_sel;
        last_sel = idx - 1;
        SendMessage (hWndDlg, WM_INITDIALOG, 0, 0);
        SKIM_BranchManager::singleton ()->setProduct ((uint32_t)prod->uiSteamAppID);
        SKIM_OnProductSelect          ();
        SKIM_OnBranchSelect           ();
        SendMessage (hWndDlg, WM_INITDIALOG, 0, 0);

        switch (cmd)
        {
          case 0:
          {
            POINT pt;
            GetCursorPos (&pt);

            BOOL ret = 128;

            while (ret != 0 && ret % 128 == 0)
            {
              ret = TrackPopupMenu ( prod->menus.hProductMenu,
                                       TPM_RIGHTALIGN      | TPM_BOTTOMALIGN | TPM_LEFTBUTTON |
                                       TPM_VERNEGANIMATION | TPM_RETURNCMD,
                                         pt.x, pt.y,
                                           0,
                                             hWndDlg, nullptr );
            }

            if (ret > 0)
            {
              SKIM_Tray_ProcessCommand (hWndDlg, lParam, ret);
            }

            return;
          } break;

          case MENUCMD_CHECK_VERSION:
            _beginthreadex (
              nullptr,
                0,
                 SKIM_UpdateProduct,
                   (LPVOID)prod,
                     0x00,
                       nullptr );
            break;

          case MENUCMD_UNINSTALL:
            SendMessage (hWndDlg, WM_COMMAND, IDC_UNINSTALL_CMD, 0);
            break;

          case MENUCMD_INSTALL:
            SendMessage (hWndDlg, WM_COMMAND, IDC_INSTALL_CMD, 0);
            break;

          case MENUCMD_DLC:
          case MENUCMD_CONFIG:
            SendMessage (hWndDlg, WM_COMMAND, IDC_MANAGE_CMD, 0);
            break;

          case MENUCMD_DIR_BACKUPS:
          {
            wchar_t wszVersionDir [MAX_PATH * 2] = { };
            lstrcatW    (wszVersionDir, SKIM_FindInstallPath (prod->uiSteamAppID));
            PathAppendW (wszVersionDir, L"Version");

            ShellExecuteW (GetActiveWindow (), L"explore", wszVersionDir, nullptr, nullptr, SW_NORMAL);
          } break;

          case MENUCMD_DIR_CONFIG:
          {
            wchar_t wszConfigDir [MAX_PATH * 2] = { };
            lstrcatW    (wszConfigDir, SKIM_FindInstallPath (prod->uiSteamAppID));

            ShellExecuteW (GetActiveWindow (), L"explore", wszConfigDir, nullptr, nullptr, SW_NORMAL);
          } break;

          case MENUCMD_DIR_LOGS:
          {
            wchar_t wszLogsDir [MAX_PATH * 2] = { };
            lstrcatW    (wszLogsDir, SKIM_FindInstallPath (prod->uiSteamAppID));
            PathAppendW (wszLogsDir, L"logs");

            ShellExecuteW (GetActiveWindow (), L"explore", wszLogsDir, nullptr, nullptr, SW_NORMAL);
          } break;

          default:
          {
            if (cmd >= 96)
            {
              static HWND
                hWndBranchSelect =
                  GetDlgItem (hWndDlg, IDC_BRANCH_SELECT);

              ComboBox_SetCurSel (hWndBranchSelect, cmd - 96);

              void
              SKIM_OnBranchSelect (void);

              SKIM_OnBranchSelect ();

              SendMessage (hWndDlg, WM_COMMAND, IDC_MIGRATE_CMD, 0);
            }
          } break;
        }

        SKIM_Tray_UpdateProduct (prod);

        return;
      }
    } break;
  }

  //SetCursorPos (last_menu_pt.x, last_menu_pt.y);

  BOOL ret =
    TrackPopupMenu ( hTrayMenu,
                       TPM_LEFTALIGN | TPM_LEFTBUTTON |
                       TPM_VERNEGANIMATION | TPM_RETURNCMD,
                         last_menu_pt.x, last_menu_pt.y,
                           0,
                             hWndDlg, nullptr );

  if (ret > 0)
  {
    SKIM_Tray_ProcessCommand (hWndDlg, lParam, ret);
  }
}

void
SKIM_Tray_HandleContextMenu (HWND hWndDlg)
{
  SetForegroundWindow (hWndDlg);
  
  POINT pt;
  GetCursorPos (&pt);

  last_menu_pt = pt;
  
  BOOL ret =
  TrackPopupMenu ( hTrayMenu,
                     TPM_LEFTALIGN | TPM_LEFTBUTTON |
                     TPM_VERNEGANIMATION  | TPM_RETURNCMD,
                       pt.x, pt.y,
                         0,
                           hWndDlg, nullptr );

  if (ret > 0)
  {
    SKIM_Tray_ProcessCommand (hWndDlg, 0, ret);
  }
}