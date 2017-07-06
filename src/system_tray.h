#pragma once

#include <Windows.h>

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