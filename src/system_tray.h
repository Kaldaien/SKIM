#pragma once

#include <Windows.h>

enum
{
  MENUCMD_INJECT_AT_BOOT  = 1,
  MENUCMD_START_INJECTION = 3,
  MENUCMD_STOP_INJECTION  = 4,
  MENUCMD_EXIT            = 6,
  MENUCMD_CHECK_VERSION   = 17,
  MENUCMD_RELNOTES        = 18,
  MENUCMD_CHANGE_BRANCH   = 19,
  MENUCMD_DLC             = 21,
  MENUCMD_CONFIG          = 22,
  MENUCMD_DIR_ASSETS      = 24,
  MENUCMD_DIR_BACKUPS     = 25,
  MENUCMD_DIR_CONFIG      = 26,
  MENUCMD_DIR_LOGS        = 27,
  MENUCMD_UNINSTALL       = 29,
  MENUCMD_INSTALL         = 30
};

struct sk_product_t;

void SKIM_Tray_RefreshMenu       (HWND hWndDlg, bool add = true);
void SKIM_Tray_SendTo            (HWND hWndDlg);
void SKIM_Tray_RestoreFrom       (HWND hWndDlg);
void SKIM_Tray_Init              (HWND hWndDlg);
void SKIM_Tray_RemoveFrom        (void);
void SKIM_Tray_UpdateStartup     (void);
void SKIM_Tray_Stop              (void);
void SKIM_Tray_Start             (void);
void SKIM_Tray_ProcessCommand    (HWND hWndDlg, LPARAM lParam, WPARAM wParam);
void SKIM_Tray_HandleContextMenu (HWND hWndDlg);
void SKIM_Tray_UpdateProduct     (sk_product_t* prod);
void SKIM_Tray_SetupProductMenu  (sk_product_t* prod);

//TODO:
//
//  Installed Products Sub-Menu
//   -------------------------
//    == Product Name (Version) ==
//     # Date Installed #
//     ******************
//
//    Check for New Version
//    Release Notes
//
//    Manage/Repair Sub-Menu
//     --------------------
//      DLC
//      Config Tool
//
//      Installed Files Sub-Menu
//       ----------------------
//        Assets
//        Backups
//        Config
//        Logs
//
//    Change Branch Sub-Menu
//     --------------------
//
//    Uninstall