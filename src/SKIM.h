#pragma once

// dllmain.cpp : Defines the entry point for the DLL application.
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_NON_CONFORMING_SWPRINTFS

#pragma warning (disable: 4091)

#include "stdafx.h"
#include <cstdint>

extern HINSTANCE g_hInstance;

//bool child = false;
//wchar_t startup_dir [MAX_PATH + 1] = { };

#include "product.h"
#include "branch.h"

const wchar_t* SKIM_SteamUtil_GetSteamIntallDir  (void);
const wchar_t* SKIM_SteamUtil_FindAppInstallPath (uint32_t appid);

bool SKIM_Depends_TestKB2533623      (SK_ARCHITECTURE arch);
bool SKIM_Depends_TestVisualCRuntime (SK_ARCHITECTURE arch);

bool SKIM_Util_IsAdmin           (void);
bool SKIM_Util_IsProcessRunning  (const wchar_t* wszProcName);
bool SKIM_Util_IsDirectory       (const wchar_t* wszPath);
bool SKIM_Util_IsDLLFromProduct  (const wchar_t* wszName, const wchar_t* wszProductName);

bool SKIM_Util_CreateDirectories (const wchar_t* wszPath);
bool SKIM_Util_GetDocumentsDir   (wchar_t*       buf,          uint32_t* pdwLen);
void SKIM_Util_MoveFileNoFail    (const wchar_t* wszOld, const wchar_t*  wszNew);
void SKIM_Util_DeleteConfigFiles (sk_product_t*  product);


// Threads
unsigned int __stdcall SKIM_MigrateProduct   (LPVOID user);
unsigned int __stdcall SKIM_InstallProduct   (LPVOID user);
unsigned int __stdcall SKIM_UninstallProduct (LPVOID user);
unsigned int __stdcall SKIM_UpdateProduct    (LPVOID user);

DWORD                       SKIM_CountProductBranches  (sk_product_t *pProduct);
SKIM_BranchManager::Branch* SKIM_GetProductBranchByIdx (sk_product_t *pProduct, int idx);
bool                        SKIM_DetermineInstallState (sk_product_t& product);
std::wstring                SKIM_SummarizeRenderAPI    (sk_product_t& product);

int                         SKIM_GetProductIdx          (sk_product_t* prod);
sk_product_t*               SKIM_GetProductByIdx        (int idx);
std::vector <sk_product_t*> SKIM_GetInstallableProducts (void);

RECT SKIM_GetHWNDRect    (HWND hWnd);
RECT SKIM_GetClientRect  (HWND hWnd);
RECT SKIM_GetDlgItemRect (HWND hDlg, UINT nIDDlgItem);

void SKIM_OnBranchSelect  (void);
void SKIM_OnProductSelect (void);
bool SKIM_ConfirmClose    (void);
void SKIM_Exit            (void);

enum {
  SKIM_STOP_INJECTION          = WM_USER + 0x122,
  SKIM_STOP_INJECTION_AND_EXIT = WM_USER + 0x123,
  SKIM_START_INJECTION         = WM_USER + 0x124,
  SKIM_RESTART_INJECTION       = WM_USER + 0x125
};

constexpr uint8_t
__stdcall
SKIM_GetBitness (void)
{
#ifdef _WIN64
  return 64;
#else
  return 32;
#endif
}

#define SKIM_RunOnce(x)    { static bool first = true; if (first) { (x); first = false; } }

#define SKIM_RunIf32Bit(x)         { SKIM_GetBitness () == 32  ? (x) :  0; }
#define SKIM_RunIf64Bit(x)         { SKIM_GetBitness () == 64  ? (x) :  0; }
#define SKIM_RunLHIfBitness(b,l,r)   SKIM_GetBitness () == (b) ? (l) : (r)

//
// PRIVATE
//
INT_PTR
CALLBACK
Main_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam );

sk_product_t* SKIM_FindProductByAppID    (uint32_t appid);
void          SKIM_DisableGlobalInjector (void);


unsigned int __stdcall SKIM_MigrateGlobalInjector (LPVOID user);
unsigned int __stdcall SKIM_InstallGlobalInjector (LPVOID user);