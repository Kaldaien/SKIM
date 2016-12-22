/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS
#define ISOLATION_AWARE_ENABLED 1

#pragma warning (disable: 4723)


#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <limits>

#include <CommCtrl.h>
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")
#pragma comment (lib,    "winmm.lib")

#include <process.h>
#include <cstdint>

#include <cstdio>

#include <Windows.h>
#include <Wininet.h>
#pragma comment (lib, "wininet.lib")

#include "resource.h"

#undef max
#undef min

enum {
  WM_FILE_PROGRESS = WM_APP,
  WM_HASH_PROGRESS,
  WM_COMBINE_PROGRESS,
  WM_FILE_DONE,
  WM_HASH_DONE,
  WM_COMBINE_DONE,
  WM_FILE_CANCELLED
};

enum {
  STATUS_UPDATED   = 1,
  STATUS_REMINDER  = 2,
  STATUS_CANCELLED = 4,
  STATUS_FAILED    = 8
};

struct sk_dlc_package {
  wchar_t   wszCategory  [MAX_PATH];
  wchar_t   wszName      [MAX_PATH];
  wchar_t   wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t   wszHostPath  [INTERNET_MAX_PATH_LENGTH];

  wchar_t   wszLocalPath [MAX_PATH];

  uint32_t  parts;
  uint32_t* crc32c;

  // To be provided later
  bool      installed;
  HTREEITEM tree_item;
} dlc_packs [] = {
  { // 0
    L"General Textures",
    L"4K Upscale Texture Pack",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z",
    L"TSFix_Res\\inject\\99_Upscale4x.7z",

    3,
    nullptr,

    false,
    0
  },
  { // 1
    L"General Textures",
    L"UI Cleanup",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_textures/97_UICleanup.7z",
    L"TSFix_Res\\inject\\97_UICleanup.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 2
    L"General Textures",
    L"Cloud/Visual Effects Cleanup",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_textures/98_Cleanup.7z",
    L"TSFix_Res\\inject\\98_Cleanup.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 3
    L"General Textures",
    L"Colette/Genis/Botta/Yggdrasil",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_textures/98_Characters.7z",
    L"TSFix_Res\\inject\\98_Characters.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 4
    L"General Textures",
    L"Lloyd Cleanup",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_textures/98_Lloyd.7z",
    L"TSFix_Res\\inject\\98_Lloyd.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 5
    L"General Textures",
    L"Smoothed Font",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_090/01_CleanFont.7z",
    L"TSFix_Res\\inject\\01_CleanFont.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 6
    L"Button Mods",
    L"PlayStation 3 Buttons",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_090/00_PS3Buttons.7z",
    L"TSFix_Res\\inject\\00_PS3Buttons.7z",

    1,
    nullptr,

    false,
    0
  },
  { // 7
    L"Button Mods",
    L"GameCube Buttons",
    L"github.com",
    L"/Kaldaien/TSF/releases/download/tsfix_textures/00_GCButtons.7z",
    L"TSFix_Res\\inject\\00_GCButtons.7z",

    1,
    nullptr,

    false,
    0
  },
};

#include <unordered_map>
std::unordered_map <HTREEITEM, sk_dlc_package*> dlc_map;

struct sk_internet_get_t {
  wchar_t  wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t  wszHostPath  [INTERNET_MAX_PATH_LENGTH];
  wchar_t  wszLocalPath [MAX_PATH];
  wchar_t  wszAppend    [MAX_PATH]; // If non-zero length, then append
                                    //   to this file.
  uint64_t size;
  uint32_t crc32c;
  int      status;
};

struct sk_internet_head_t {
  wchar_t  wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t  wszHostPath  [INTERNET_MAX_PATH_LENGTH];
  HANDLE   hThread;
  uint64_t size;
};

#include <queue>
#include <vector>

std::queue  <sk_internet_get_t>  files_to_download;
std::queue  <sk_internet_get_t>  files_to_hash;

std::queue  <sk_internet_get_t>  files_to_combine;
std::queue  <sk_internet_get_t>  files_to_delete;

std::vector <sk_internet_head_t> files_to_lookup;

sk_internet_get_t file_to_download;
sk_internet_get_t file_to_hash;
sk_internet_get_t file_to_combine;

uint64_t           total_download_size    = 0ULL;
uint64_t           total_downloaded_bytes = 0ULL;
uint64_t           total_hashed_bytes     = 0ULL;
uint64_t           total_combined_bytes   = 0ULL;

uint32_t           file_download_size     = 0UL;
uint32_t           file_downloaded_bytes  = 0UL;
uint32_t           file_hashed_bytes      = 0UL;
uint32_t           file_combined_bytes    = 0UL;

bool final_validate = false;

HWND   hWndDownloadDialog = 0;
HWND   hWndDLCMgr         = 0;
HANDLE hWorkerThread      = 0;
HANDLE hTerminateEvent    = 0;

HWND hWndFileProgress;
HWND hWndTotalProgress;

sk_dlc_package* installing = nullptr;


bool
SKIM_CombineFiles ( const wchar_t* wszCombinedFile )
{
  HANDLE hCombinedFile = INVALID_HANDLE_VALUE,
         hPartialFile  = INVALID_HANDLE_VALUE;

  DWORD  dwBytesRead, dwBytesWritten, dwPos;

  static BYTE   buff [16384 * 1024];

  hCombinedFile =
    CreateFile ( wszCombinedFile,
                  FILE_APPEND_DATA,
                    FILE_SHARE_READ,
                      nullptr,
                        OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                            NULL);

  if (hCombinedFile == INVALID_HANDLE_VALUE)
    return false;

  while ( files_to_combine.size () > 0 ) {
    if (WaitForSingleObject (hTerminateEvent, 0) == WAIT_OBJECT_0)
      break;

    file_combined_bytes = 0UL;

    memcpy ( &file_to_combine,
               &files_to_combine.front (),
                 sizeof sk_internet_get_t );

    files_to_combine.pop ();

    sk_internet_get_t* get =
      (sk_internet_get_t *)&file_to_combine;

    auto ProgressMsg =
      [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
        LRESULT
        {
          if (hWndFileProgress != 0)
            return SendMessage ( hWndFileProgress,
                                   Msg,
                                     wParam,
                                       lParam );
          return NULL;
        };

    auto SetProgress = 
      [=](auto cur, auto max) ->
        void
        {
          ProgressMsg ( PBM_SETPOS,
                          (WPARAM)(
                            std::numeric_limits <int32_t>::max () *
                              ((double)cur / (double)max)
                          ),
                            0L );
        };

    hPartialFile =
      CreateFile ( file_to_combine.wszLocalPath,
                     GENERIC_READ,
                       0,
                         NULL,
                           OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                               NULL );

    if (hPartialFile == INVALID_HANDLE_VALUE) {
      CloseHandle (hCombinedFile);
      return false;
    }

    while ( ReadFile ( hPartialFile, buff, sizeof (buff),
                         &dwBytesRead, nullptr )
              && dwBytesRead > 0 )
    {
      dwPos =
        SetFilePointer (hCombinedFile, 0, nullptr, FILE_END);

      LockFile   (hCombinedFile, dwPos, 0,            dwBytesRead,    0);
      WriteFile  (hCombinedFile, buff,  dwBytesRead, &dwBytesWritten, nullptr);
      UnlockFile (hCombinedFile, dwPos, 0,            dwBytesRead,    0);

      SendMessage ( hWndDownloadDialog, WM_COMBINE_PROGRESS, dwBytesRead, 0x00 );

      if (WaitForSingleObject (hTerminateEvent, 0UL) == WAIT_OBJECT_0)
        break;
    }

    if (WaitForSingleObject (hTerminateEvent, 0UL) != WAIT_OBJECT_0)
      SendMessage ( hWndDownloadDialog, WM_COMBINE_DONE, 0x00, 0x00 );

    CloseHandle (hPartialFile);
  }


  if (WaitForSingleObject (hTerminateEvent, 0) != WAIT_OBJECT_0) {
    if (files_to_combine.size () == 0) {
      file_to_combine.crc32c = 0x41EDFD6F;
      file_to_combine.size   = total_download_size;
      wcscpy (file_to_combine.wszLocalPath, file_to_combine.wszAppend);

      files_to_hash.push (file_to_combine);

      final_validate = true;

      SendMessage ( hWndDownloadDialog, WM_HASH_DONE, 0x00, 0x00 );
    }
  } else {
    ResetEvent (hTerminateEvent);
  }

  CloseHandle (hCombinedFile);

  return true;
}

void
__stdcall
SK_HashProgressCallback (uint64_t current, uint64_t total)
{
  if (WaitForSingleObject (hTerminateEvent, 0) == WAIT_OBJECT_0) {
    PostMessage     (hWndDownloadDialog, WM_CLOSE, 0x00, 0x00);
    ResetEvent      (hTerminateEvent);
    hWorkerThread = 0;
    TerminateThread (GetCurrentThread (), 0x00);
  }

  sk_internet_get_t* get =
    (sk_internet_get_t *)&file_to_hash;

  auto ProgressMsg =
    [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (hWndFileProgress)
          return SendMessage ( hWndFileProgress,
                                 Msg,
                                   wParam,
                                     lParam );
        return NULL;
      };

  auto SetProgress = 
    [=](auto cur, auto max) ->
      void
      {
        ProgressMsg ( PBM_SETPOS,
                        (WPARAM)(
                          std::numeric_limits <int32_t>::max () *
                            ((double)cur / (double)max)
                        ),
                          0L );
      };

  SetProgress (current, total);

  if (current <= total)
    SendMessage ( hWndDownloadDialog, WM_HASH_PROGRESS, (WPARAM)(current - file_hashed_bytes), 0UL);
}

DWORD
WINAPI
CombineThread (LPVOID user)
{
  sk_internet_get_t* get =
    (sk_internet_get_t *)user;

  auto ProgressMsg =
    [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (hWndFileProgress != 0)
          return SendMessage ( hWndFileProgress,
                                 Msg,
                                   wParam,
                                     lParam );
        return NULL;
      };

  auto SetProgress = 
    [=](auto cur, auto max) ->
      void
      {
        ProgressMsg ( PBM_SETPOS,
                        (WPARAM)(
                          std::numeric_limits <int32_t>::max () *
                            ((double)cur / (double)max)
                        ),
                          0L );
      };

  SetProgress (0, 0);

  SKIM_CombineFiles (files_to_combine.front ().wszAppend);

  SendMessage ( hWndDownloadDialog, WM_COMBINE_DONE, 0x00, 0x00 );

  hWorkerThread = 0;

  CloseHandle (GetCurrentThread ());

  return 0;
}


DWORD
WINAPI
HashThread (LPVOID user)
{
  sk_internet_get_t* get =
    (sk_internet_get_t *)user;

  auto ProgressMsg =
    [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (hWndFileProgress)
          return SendMessage ( hWndFileProgress,
                                 Msg,
                                   wParam,
                                     lParam );
        return NULL;
      };

  auto SetProgress = 
    [=](auto cur, auto max) ->
      void
      {
        ProgressMsg ( PBM_SETPOS,
                        (WPARAM)(
                          std::numeric_limits <int32_t>::max () *
                            ((double)cur / (double)max)
                        ),
                          0L );
      };

  SetProgress (0, 0);

  typedef void     (__stdcall *SK_HashProgressCallback_pfn)(uint64_t current, uint64_t total);
  typedef uint32_t (__stdcall *SK_GetFileCRC32C_pfn)(const wchar_t*, SK_HashProgressCallback_pfn);

#ifdef _WIN64
  HMODULE hSK64 = LoadLibrary (L"installer64.dll");
#else
  HMODULE hSK64 = LoadLibrary (L"installer.dll");
#endif

  if (hSK64 != nullptr) {

    SK_GetFileCRC32C_pfn SK_GetFileCRC32C =
      (SK_GetFileCRC32C_pfn)
        GetProcAddress (hSK64, "SK_GetFileCRC32C");

    uint32_t crc32c =
      SK_GetFileCRC32C ( get->wszLocalPath, SK_HashProgressCallback );

    if (crc32c != get->crc32c) {
      wchar_t wszCRC32C [128] = { L'\0' };

      _swprintf ( wszCRC32C,
                    L"CRC32C for '%s' is 0x%08X ... expected 0x%08X",
                      get->wszLocalPath,
                        crc32c,
                          get->crc32c );

      installing->installed = false;

      // Delete and re-download
      DeleteFileW (get->wszLocalPath);

      MessageBox (hWndDownloadDialog, wszCRC32C, L"Checksum Mismatch", MB_OK);

      SendMessage (hWndDownloadDialog, WM_CLOSE, 0x00, 0x00);

      hWorkerThread = 0;

      CloseHandle (GetCurrentThread ());

      return 0;
    }


    //
    // DLC SUCCESS, delete temp files
    //
    else if (final_validate) {
      installing->installed = true;

      MessageBox ( hWndDownloadDialog,
                    L"DLC Successfully Installed",
                      L"Special K DLC Install",
                        MB_OK | MB_ICONINFORMATION );

      while (files_to_delete.size ()) {
        DeleteFileW (files_to_delete.front ().wszLocalPath);
        files_to_delete.pop ();
      }

      //DeleteFileW (L"installer64.dll");

      hWorkerThread = 0;

      SendMessage (hWndDownloadDialog, WM_CLOSE, 0x00, 0x00);

      CloseHandle (GetCurrentThread ());

      return 0;
    }
  }

  files_to_delete.push (file_to_hash);

  SendMessage ( hWndDownloadDialog, WM_HASH_DONE, 0x00, 0x00 );

  hWorkerThread = 0;

  CloseHandle (GetCurrentThread ());

  return 0;
}

DWORD
WINAPI
DownloadThread (LPVOID user)
{
  sk_internet_get_t* get =
    (sk_internet_get_t *)user;

  auto ProgressMsg =
    [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (hWndFileProgress != 0)
          return SendMessage ( hWndFileProgress,
                                 Msg,
                                   wParam,
                                     lParam );
        return NULL;
      };

  auto SetProgress = 
    [=](auto cur, auto max) ->
      void
      {
        ProgressMsg ( PBM_SETPOS,
                        (WPARAM)(
                          std::numeric_limits <int32_t>::max () *
                            ((double)cur / (double)max)
                        ),
                          0L );
      };

  SetProgress (0, 0);


  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K DLC Grabber",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    goto CLEANUP;

  DWORD dwInetCtx;

  HINTERNET hInetHost =
    InternetConnect ( hInetRoot,
                        get->wszHostName,
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  if (! hInetHost) {
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  PCWSTR rgpszAcceptTypes [] = { L"*/*", nullptr };

  HINTERNET hInetHTTPGetReq =
    HttpOpenRequest ( hInetHost,
                        nullptr,
                          get->wszHostPath,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_NO_UI          | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
                                    (DWORD_PTR)&dwInetCtx );

  if (! hInetHTTPGetReq) {
    InternetCloseHandle (hInetHost);
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  if ( HttpSendRequestW ( hInetHTTPGetReq,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) ) {

    DWORD dwContentLength     = 0;
    DWORD dwContentLength_Len = sizeof DWORD;
    DWORD dwSize;

    HttpQueryInfo ( hInetHTTPGetReq,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                        &dwContentLength,
                          &dwContentLength_Len,
                            nullptr );

    SetProgress (0, dwContentLength);

    DWORD dwTotalBytesDownloaded = 0UL;

    if ( InternetQueryDataAvailable ( hInetHTTPGetReq,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      HANDLE hGetFile =
        CreateFileW ( get->wszLocalPath,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hGetFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        if (WaitForSingleObject (hTerminateEvent, 0) == WAIT_OBJECT_0) {
          break;
        }

        DWORD    dwRead = 0;
        uint8_t *pData  = (uint8_t *)malloc (dwSize);

        if (! pData)
          break;

        if ( InternetReadFile ( hInetHTTPGetReq,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten;

          WriteFile ( hGetFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );

          dwTotalBytesDownloaded += dwRead;

          SetProgress (                        dwTotalBytesDownloaded, dwContentLength);
          SendMessage (hWndDownloadDialog, WM_FILE_PROGRESS, dwRead,                 0);
        }

        free (pData);
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetHTTPGetReq,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) {
          break;
        }
      }

      if (WaitForSingleObject (hTerminateEvent, 0) != WAIT_OBJECT_0)
        SendMessage (hWndDownloadDialog, WM_FILE_DONE, 0, 0);

      if (hGetFile != INVALID_HANDLE_VALUE)
        CloseHandle (hGetFile);
    }

    get->status = STATUS_UPDATED;
  }

  InternetCloseHandle (hInetHTTPGetReq);
  InternetCloseHandle (hInetHost);
  InternetCloseHandle (hInetRoot);

  goto END;

CLEANUP:
  get->status = STATUS_FAILED;

END:
  //if (! get->hTaskDlg)
    //delete get;

  if (WaitForSingleObject (hTerminateEvent, 0) == WAIT_OBJECT_0)
    ResetEvent (hTerminateEvent);

  hWorkerThread = 0;

  CloseHandle (GetCurrentThread ());

  return 0;
}

DWORD
WINAPI
HeaderThread (LPVOID user)
{
  sk_internet_head_t* head =
    (sk_internet_head_t *)user;

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K DLC Grabber",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    goto CLEANUP;

  DWORD dwInetCtx;

  HINTERNET hInetHost =
    InternetConnect ( hInetRoot,
                        head->wszHostName,
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  if (! hInetHost) {
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  PCWSTR rgpszAcceptTypes [] = { L"*/*", nullptr };

  HINTERNET hInetHTTPGetReq =
    HttpOpenRequest ( hInetHost,
                        nullptr,
                          head->wszHostPath,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_NO_UI          | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
                                    (DWORD_PTR)&dwInetCtx );

  if (! hInetHTTPGetReq) {
    InternetCloseHandle (hInetHost);
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  if ( HttpSendRequestW ( hInetHTTPGetReq,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) ) {

    DWORD dwContentLength     = 0;
    DWORD dwContentLength_Len = sizeof DWORD;

    HttpQueryInfo ( hInetHTTPGetReq,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                        &dwContentLength,
                          &dwContentLength_Len,
                            nullptr );

    head->size = dwContentLength;
  }

  InternetCloseHandle (hInetHTTPGetReq);
  InternetCloseHandle (hInetHost);
  InternetCloseHandle (hInetRoot);

  goto END;

CLEANUP:
  //head->size = 0;

END:
  //CloseHandle (GetCurrentThread ());

  return 0;
}

HWND hWndUpdateDlg = (HWND)INVALID_HANDLE_VALUE;

DWORD
WINAPI
SK_RealizeForegroundWindow (HWND hWndForeground)
{
  static volatile ULONG nest_lvl = 0UL;

  while (InterlockedExchangeAdd (&nest_lvl, 0))
    Sleep (125);

  InterlockedIncrementAcquire (&nest_lvl);

  // Aren't lambdas fun?! :)
  CreateThread (
    nullptr,
      0,
        [](LPVOID user)->

        DWORD
        {
          BringWindowToTop    ((HWND)user);
          SetForegroundWindow ((HWND)user);

          CloseHandle (GetCurrentThread ());

          return 0;
        },

        (LPVOID)hWndForeground,
      0x00,
    nullptr
  );

  InterlockedDecrementRelease (&nest_lvl);

  return 0UL;
}

HRESULT
CALLBACK
DownloadDialogCallback (
  _In_ HWND     hWnd,
  _In_ UINT     uNotification,
  _In_ WPARAM   wParam,
  _In_ LPARAM   lParam,
  _In_ LONG_PTR dwRefData )
{
  sk_internet_get_t* get =
    (sk_internet_get_t *)dwRefData;

 extern DWORD WINAPI SK_RealizeForegroundWindow (HWND);

  if (uNotification == TDN_TIMER)
  {
    SK_RealizeForegroundWindow (hWnd);

    if ( get->status == STATUS_UPDATED   ||
         get->status == STATUS_CANCELLED ) {
      EndDialog ( hWnd, 0 );
      return S_OK;
    }
  }

  if (uNotification == TDN_HYPERLINK_CLICKED) {
    ShellExecuteW (nullptr, L"open", (wchar_t *)lParam, nullptr, nullptr, SW_SHOWMAXIMIZED);

    return S_OK;
  }

  if (uNotification == TDN_DIALOG_CONSTRUCTED) {
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_RANGE, 0L,          MAKEWPARAM (0, 1));
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_POS,   1,           0L);
    SendMessage (hWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0L);

    SK_RealizeForegroundWindow (hWnd);

    return S_OK;
  }

  if (uNotification == TDN_CREATED) {
    SK_RealizeForegroundWindow (hWnd);

    return S_OK;
  }

  if (uNotification == TDN_BUTTON_CLICKED)
  {
    if (wParam == IDYES)
    {
      //get->hTaskDlg = hWnd;

      //_beginthreadex ( nullptr,
                         //0,
                          //DownloadThread,
                            //(LPVOID)get,
                              //0x00,
                              //nullptr );
    }

    else {
      //get->status = STATUS_CANCELLED;
      //return S_OK;
    }
  }

  if (uNotification == TDN_DESTROYED) {
    //sk_internet_get_t* get =
      //(sk_internet_get_t *)dwRefData;

    //delete get;
  }

  return S_FALSE;
}


#include <Windowsx.h>
#include <CommCtrl.h>

static volatile LONG __SK_UpdateStatus = 0;

#include <Windows.h>
#include <Richedit.h>

struct {
  HWND     hWndCtl;
  COLORREF op_color;
  wchar_t  wszOp   [32];
  wchar_t  wszName [128];

  void set (COLORREF color, const wchar_t* op, const wchar_t* name) {
    if (op_color != color || wcsncmp (op, wszOp, 32) || wcsncmp (name, wszName, 128)) {
      op_color = color;
      wcsncpy (wszOp,   op,   32);
      wcsncpy (wszName, name, 128);

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = color;
      cf2.crBackColor = RGB (10, 10,  10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndCtl, EM_SETSEL,        0,             (LPARAM)wcslen (op));
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)op);

      size_t dwEndSel = wcslen (op);

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_EFFECTS;
      cf2.dwEffects   = CFE_BOLD | CFE_AUTOBACKCOLOR;

      SendMessage (hWndCtl, EM_SETSEL,        dwEndSel,      (LPARAM)(dwEndSel + wcslen (L"\t")));
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      SendMessage (hWndCtl, EM_SETSEL,        dwEndSel,      (LPARAM)(dwEndSel + wcslen (name)));
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)name);
    }
  }
} dlc_op;

struct {
  int32_t current = 0L;

  void set (HWND hWndParent, int32_t new_pos) {
    if (current != new_pos) {
      current = new_pos;

      HWND hWndTotalProgress_ =
        GetDlgItem (hWndParent, IDC_DLC_TOTAL_PROGRESS);

      SendMessage ( hWndTotalProgress_,
                      PBM_SETPOS,
                        current,
                          0UL );
    }
  }
} dlc_total_progress;

struct {
  int32_t current = 0L;

  void set (HWND hWndParent, int32_t new_pos) {
    if (current != new_pos) {
      current = new_pos;

      HWND hWndFileProgress_ =
        GetDlgItem (hWndParent, IDC_DLC_FILE_PROGRESS);

      SendMessage ( hWndFileProgress_,
                      PBM_SETPOS,
                        current,
                          0UL );
    }
  }
} dlc_file_progress;

struct {
  HWND    hWndCtl;
  wchar_t wszFile [MAX_PATH];

  void set (const wchar_t* val) {
    if (_wcsnicmp (val, wszFile, MAX_PATH)) {
      wcsncpy (wszFile, val, MAX_PATH);

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndCtl, EM_SETSEL,        0,             -1);
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)wszFile);
    }
  }
} dlc_current_file;

struct {
  HWND    hWndCtl;
  wchar_t wszSize [64];

  void set (const wchar_t* val) {
    if (_wcsnicmp (val, wszSize, 64)) {
      wcsncpy (wszSize, val, 64);

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndCtl, EM_SETSEL,        0,             -1);
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)val);
    }
  }
} dlc_current_size;

struct {
  HWND    hWndCtl;
  wchar_t wszSize [64];

  void set (const wchar_t* val) {
    if (_wcsnicmp (val, wszSize, 64)) {
      wcsncpy (wszSize, val, 64);

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndCtl, EM_SETSEL,        0,             -1);
      SendMessage (hWndCtl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndCtl, EM_REPLACESEL,    FALSE,         (LPARAM)val);
    }
  }
} dlc_total_size;

HTREEITEM
AddItemToTree ( HWND   hwndTV,
                LPTSTR lpszItem,
                int    nLevel,
                bool   installed )
{
  static HIMAGELIST hImgList =
    ImageList_Create ( 16, 16,
                         ILC_COLOR32 | ILC_HIGHQUALITYSCALE,
                           2, 32 );

  static HINSTANCE  hShell32          = LoadLibrary (L"SHELL32.DLL");
  static HICON      hIconRoot         = LoadIcon    (hShell32, MAKEINTRESOURCE (171));
  static HICON      hIconOpenFolder   = LoadIcon    (hShell32, MAKEINTRESOURCE (235));
  static HICON      hIconNotInstalled = LoadIcon    (hShell32, MAKEINTRESOURCE (240));
  static HICON      hIconInstalled    = LoadIcon    (hShell32, MAKEINTRESOURCE (16810));

  static bool init = false;
  if (! init) {
    ImageList_AddIcon (hImgList, hIconNotInstalled);
    ImageList_AddIcon (hImgList, hIconInstalled);
    ImageList_AddIcon (hImgList, hIconRoot);
    ImageList_AddIcon (hImgList, hIconOpenFolder);

    DestroyIcon (hIconNotInstalled);
    DestroyIcon (hIconInstalled);

    FreeLibrary (hShell32);

    init = true;
  }

  TreeView_SetImageList ( hwndTV, hImgList, TVSIL_NORMAL );
  TreeView_SetImageList ( hwndTV, hImgList, TVSIL_STATE  );

         TVITEM         tvi;
         TVINSERTSTRUCT tvins;
  static HTREEITEM      hPrev         = (HTREEITEM)TVI_FIRST;
  static HTREEITEM      hPrevRootItem = NULL;
  static HTREEITEM      hPrevLev2Item = NULL;
         HTREEITEM      hti;

  tvi.mask = TVIF_TEXT | TVIF_IMAGE
             | TVIF_SELECTEDIMAGE | TVIF_PARAM;

  // Set the text of the item. 
  tvi.pszText    = lpszItem;
  tvi.cchTextMax = sizeof (tvi.pszText) / sizeof (tvi.pszText [0]);

  // Assume the item is not a parent item, so give it a 
  // document image.
  if (nLevel == 3) {
    if (installed) {
      tvi.iImage         = 1;
      tvi.iSelectedImage = 1;
    } else {
      tvi.iImage         = 0;
      tvi.iSelectedImage = 0;
    }
  } else if (nLevel == 1) {
    tvi.iImage           = 2;
    tvi.iSelectedImage   = 2;
  } else {
    tvi.iImage           = 3;
    tvi.iSelectedImage   = 3;
  }

  // Save the heading level in the item's application-defined
  // data area.
  tvi.lParam = (LPARAM)nLevel;
  tvins.item = tvi;
  tvins.hInsertAfter = hPrev;

  // Set the parent item based on the specified level.
  if (nLevel == 1)
      tvins.hParent = TVI_ROOT;
  else if (nLevel == 2)
      tvins.hParent = hPrevRootItem;
  else
      tvins.hParent = hPrevLev2Item;

  // Add the item to the tree-view control.
  hPrev =
    (HTREEITEM)SendMessage (
      hwndTV,
        TVM_INSERTITEM,
          0,
            (LPARAM)(LPTVINSERTSTRUCT)&tvins );

  if (hPrev == NULL)
      return NULL;

  // Save the handle to the item.
  if (nLevel == 1)
      hPrevRootItem = hPrev;
  else if (nLevel == 2)
      hPrevLev2Item = hPrev;

  if (nLevel > 1)
  {
      hti       = TreeView_GetParent (hwndTV, hPrev);
      tvi.mask  = 0x00;
      tvi.hItem = hti;
      TreeView_SetItem (hwndTV, &tvi);
  }

  return hPrev;
}

void
SKIM_OnTreeViewSelChange (HWND hDlg)
{
  files_to_lookup.clear   ();

  while (! files_to_download.empty ())
    files_to_download.pop ();

  while (! files_to_hash.empty ())
    files_to_hash.pop ();

  HTREEITEM hTreeItem =
    TreeView_GetSelection (GetDlgItem (hDlg, IDC_DLC_TREE));

  if (dlc_map.count (hTreeItem)) {
    ShowWindow   (GetDlgItem (hDlg, IDC_INSTALL_DLC),  SW_SHOW);
    ShowWindow   (GetDlgItem (hDlg, IDC_VALIDATE_DLC), SW_SHOW);

    sk_dlc_package* dlc_pkg =
      dlc_map [hTreeItem];

    bool installed = dlc_pkg->installed;

    EnableWindow  (GetDlgItem (hDlg, IDC_INSTALL_DLC), TRUE);
    SetWindowText (GetDlgItem (hDlg, IDC_INSTALL_DLC),
          installed ? L"Uninstall" : L"Install" );

    EnableWindow (GetDlgItem (hDlg, IDC_VALIDATE_DLC), installed);

    if (dlc_pkg->parts > 1) {
      wchar_t* wszFormattedPart =
        new wchar_t [INTERNET_MAX_PATH_LENGTH];

      for (uint32_t i = 1; i <= dlc_pkg->parts; i++) {
        if (wszFormattedPart == nullptr)
          break;

        swprintf ( wszFormattedPart,
                     L"%s.%03lu",
                       dlc_pkg->wszHostPath,
                         i );

        sk_internet_head_t* head =
          new sk_internet_head_t;

        wcsncpy (head->wszHostName, dlc_pkg->wszHostName, INTERNET_MAX_HOST_NAME_LENGTH);
        wcsncpy (head->wszHostPath, wszFormattedPart,     INTERNET_MAX_PATH_LENGTH);
        head->hThread = INVALID_HANDLE_VALUE;
        head->size    = 0ULL;

        files_to_lookup.push_back ( *head );

        delete head;

        wchar_t wszFormattedLocalPart [MAX_PATH];

        swprintf ( wszFormattedLocalPart,
                     L"%s.%03lu",
                       dlc_pkg->wszLocalPath,
                         i );

        sk_internet_get_t* get =
          new sk_internet_get_t;

        wcsncpy (get->wszHostName,  dlc_pkg->wszHostName,  INTERNET_MAX_HOST_NAME_LENGTH);
        wcsncpy (get->wszHostPath,  wszFormattedPart,      INTERNET_MAX_PATH_LENGTH);
        wcsncpy (get->wszLocalPath, wszFormattedLocalPart, MAX_PATH);
        wcsncpy (get->wszAppend, dlc_pkg->wszLocalPath,    MAX_PATH);

        get->size   = 0; // Won't know this until we fetch it

        get->crc32c = dlc_pkg->crc32c != nullptr ?
                        dlc_pkg->crc32c [i - 1] :
                          0x00;
        get->status = 0;

        files_to_download.push ( *get );

        delete get;
      }

      if (wszFormattedPart != nullptr)
        delete [] wszFormattedPart;
    }

    //
    // Single-Part DLC Package
    //
    else {
      sk_internet_head_t* head =
        new sk_internet_head_t;

      wcsncpy (head->wszHostName, dlc_pkg->wszHostName, INTERNET_MAX_HOST_NAME_LENGTH);
      wcsncpy (head->wszHostPath, dlc_pkg->wszHostPath, INTERNET_MAX_PATH_LENGTH);
      head->hThread = INVALID_HANDLE_VALUE;
      head->size    = 0ULL;

      files_to_lookup.push_back ( *head );

      delete head;

      sk_internet_get_t* get =
        new sk_internet_get_t;

      wcsncpy (get->wszHostName,  dlc_pkg->wszHostName,  INTERNET_MAX_HOST_NAME_LENGTH);
      wcsncpy (get->wszHostPath,  dlc_pkg->wszHostPath,       INTERNET_MAX_PATH_LENGTH);
      wcsncpy (get->wszLocalPath, dlc_pkg->wszLocalPath,                      MAX_PATH);
      wcsncpy (get->wszAppend,    L"",                                               1);

      get->size   = 0; // Won't know this until we fetch it

      get->crc32c = dlc_pkg->crc32c != nullptr ?
                      dlc_pkg->crc32c [0] :
                        0x00;
      get->status = 0;

      files_to_download.push ( *get );

      delete get;
    }
  }

  else {
    ShowWindow   (GetDlgItem (hDlg, IDC_INSTALL_DLC), SW_HIDE);
    EnableWindow (GetDlgItem (hDlg, IDC_INSTALL_DLC),  FALSE);

    ShowWindow   (GetDlgItem (hDlg, IDC_VALIDATE_DLC), SW_HIDE);
    EnableWindow (GetDlgItem (hDlg, IDC_VALIDATE_DLC), FALSE);
  }
}

INT_PTR
CALLBACK
DLCMgr_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  UNREFERENCED_PARAMETER (wParam);

  switch (uMsg) {
    case WM_INITDIALOG:
    {
      dlc_map.clear ();

      if (hTerminateEvent == 0) {
        hTerminateEvent =
          CreateEvent ( nullptr, TRUE, FALSE, nullptr );
      }

      int install_idx = -1;

      if (installing != nullptr) {
        for (int i = 0; i < sizeof (dlc_packs) / sizeof (sk_dlc_package); i++) {
          if (installing->tree_item == dlc_packs [i].tree_item) {
            install_idx = i;
            break;
          }
        }
      }

      if (dlc_packs [0].crc32c == nullptr)
        dlc_packs [0].crc32c = new uint32_t [4] {0xDEFC02D3, 0xA570FCE9, 0xF69DBD47, 0x41EDFD6F};

      if (dlc_packs [1].crc32c == nullptr)
        dlc_packs [1].crc32c = new uint32_t [1] { 0x5F11E49D };

      if (dlc_packs [2].crc32c == nullptr)
        dlc_packs [2].crc32c = new uint32_t [1] { 0x69DA4E58 };

      if (dlc_packs [3].crc32c == nullptr)
        dlc_packs [3].crc32c = new uint32_t [1] { 0xFA20BFDE };

      if (dlc_packs [4].crc32c == nullptr)
        dlc_packs [4].crc32c = new uint32_t [1] { 0x508C684C };

      if (dlc_packs [5].crc32c == nullptr)
        dlc_packs [5].crc32c = new uint32_t [1] { 0x92E3FD8C };

      // PS3 Buttons
      if (dlc_packs [6].crc32c == nullptr)
        dlc_packs [6].crc32c = new uint32_t [1] { 0x5E8D1335 };

      // GameCube Buttons
      if (dlc_packs [7].crc32c == nullptr)
        dlc_packs [7].crc32c = new uint32_t [1] { 0xC13A8866 };



      TreeView_DeleteAllItems (GetDlgItem (hWndDlg, IDC_DLC_TREE));

      //SIID_APPLICATION
      AddItemToTree (GetDlgItem (hWndDlg, IDC_DLC_TREE), L"Tales of Symphonia \"Fix\"", 1, false);
      //SIID_FOLDER / SIID_FOLDEROPEN

      // SIID_SETTINGS / SIID_DELETE
      AddItemToTree (GetDlgItem (hWndDlg, IDC_DLC_TREE), L"General Texture Packs",      2, false);

      for (int i = 0; i < sizeof (dlc_packs) / sizeof (sk_dlc_package); i++) {
        if (! _wcsicmp (dlc_packs [i].wszCategory, L"General Textures")) {
          if (GetFileAttributes (dlc_packs [i].wszLocalPath) != INVALID_FILE_ATTRIBUTES) {
            dlc_packs [i].installed = true;
          }

          dlc_packs [i].tree_item =
            AddItemToTree ( GetDlgItem (hWndDlg, IDC_DLC_TREE),
                              dlc_packs [i].wszName,
                                3,
                                  dlc_packs [i].installed );

          dlc_map [dlc_packs [i].tree_item] = &dlc_packs [i];
        }
      }

      AddItemToTree (GetDlgItem (hWndDlg, IDC_DLC_TREE), L"Button Mods",                2, false);

      for (int i = 0; i < sizeof (dlc_packs) / sizeof (sk_dlc_package); i++) {
        if (! _wcsicmp (dlc_packs [i].wszCategory, L"Button Mods")) {
          if (GetFileAttributes (dlc_packs [i].wszLocalPath) != INVALID_FILE_ATTRIBUTES) {
            dlc_packs [i].installed = true;
          }

          dlc_packs [i].tree_item =
            AddItemToTree ( GetDlgItem (hWndDlg, IDC_DLC_TREE),
                              dlc_packs [i].wszName,
                                3,
                                  dlc_packs [i].installed );

          dlc_map [dlc_packs [i].tree_item] = &dlc_packs [i];
        }
      }

      TreeView_SelectItem (GetDlgItem (hWndDlg, IDC_DLC_TREE), (LPARAM)TreeView_GetRoot (GetDlgItem (hWndDlg, IDC_DLC_TREE)));
      SendMessage (GetDlgItem (hWndDlg, IDC_DLC_TREE), WM_KEYDOWN, (WPARAM)VK_MULTIPLY, (LPARAM)0);

      if (installing != nullptr) {
        TreeView_SelectItem (GetDlgItem (hWndDlg, IDC_DLC_TREE), dlc_packs [install_idx].tree_item);
      }

      SKIM_OnTreeViewSelChange (hWndDlg);
    } break;

    case WM_NOTIFY:
    {
      NMHDR* hdr = (NMHDR *)lParam;

      if (hdr->idFrom == IDC_DLC_TREE) {
        switch (hdr->code) {
          case TVN_SELCHANGED:
          {
            SKIM_OnTreeViewSelChange (hWndDlg);
          } break;
        }
      }
    } break;

    case WM_COMMAND:
    {
      switch (LOWORD (wParam)) {
        case IDC_INSTALL_DLC:
        {
          unsigned int
          __stdcall
          DLCDownload_Thread (LPVOID user);

          HTREEITEM hTreeItem =
            TreeView_GetSelection (GetDlgItem (hWndDlg, IDC_DLC_TREE));

          installing = dlc_map [hTreeItem];

          if (installing->installed) {
            DeleteFileW (installing->wszLocalPath);
            installing->installed = false;

            SendMessage (hWndDlg, WM_INITDIALOG, 0x00, 0x00);
          }

          else {
            _beginthreadex ( nullptr, 0,
                               DLCDownload_Thread, (LPVOID)hWndDlg,
                                 0x00, nullptr );
          }
        } break;

        case IDC_VALIDATE_DLC:
        {
          HTREEITEM hTreeItem =
            TreeView_GetSelection (GetDlgItem (hWndDlg, IDC_DLC_TREE));

          installing = dlc_map [hTreeItem];

          while (! files_to_download.empty ()) {
            files_to_hash.push (files_to_download.front ());
            files_to_download.pop ();
          }

          if (installing->parts > 1) {
            sk_internet_get_t final_hash;

            while (! files_to_hash.empty ()) {
              final_hash = files_to_download.front ();
              files_to_hash.pop ();
            }

            final_hash.crc32c = installing->crc32c [installing->parts];
            final_hash.size   = total_download_size;
            wcscpy (final_hash.wszLocalPath, installing->wszLocalPath);
            wcscpy (final_hash.wszAppend,    L"");
            files_to_hash.push (final_hash);
          }

          unsigned int
          __stdcall
          DLCDownload_Thread (LPVOID user);

          _beginthreadex ( nullptr, 0,
                             DLCDownload_Thread, (LPVOID)hWndDlg,
                               0x00, nullptr );
        } break;
      }
    } break;

    case WM_CLOSE:
    case WM_DESTROY:
    {
      hWndDLCMgr = 0;
      EndDialog   (hWndDlg, 0x0);
      //return (INT_PTR)true;
    }

    case WM_CREATE:
    case WM_PAINT:
    case WM_SIZE:
      return (INT_PTR)false;
  }

  return (INT_PTR)false;
}

INT_PTR
CALLBACK
DLC_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  UNREFERENCED_PARAMETER (lParam);

  switch (uMsg) {
    case WM_FILE_PROGRESS:
    {
      static uint32_t last_update = 0;

      total_downloaded_bytes += (uint32_t)wParam;
      file_downloaded_bytes  += (uint32_t)wParam;

      if (timeGetTime () - last_update > 125) {
        wchar_t wszTotalSize [64] = { L'\0' };

        _swprintf ( wszTotalSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)total_downloaded_bytes / (1024.0 * 1024.0),
                          (double)total_download_size  / (1024.0 * 1024.0) );

        dlc_op.set         (RGB (22, 248, 22), L" GET ", installing->wszName);
        dlc_total_size.set (wszTotalSize);

        dlc_total_progress.set (
          hWndDlg,
            (int32_t)((double)std::numeric_limits <int32_t>::max () *
              ((double)total_downloaded_bytes / (double)total_download_size)) );

        dlc_current_file.set (file_to_download.wszLocalPath);

        wchar_t wszFileSize [64] = { L'\0' };

        _swprintf ( wszFileSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)file_downloaded_bytes   / (1024.0 * 1024.0),
                          (double)file_to_download.size / (1024.0 * 1024.0) );

        dlc_current_size.set (wszFileSize);

        last_update = timeGetTime ();
      }
    } break;

    case WM_HASH_PROGRESS:
    {
      static uint32_t last_update = 0;

      total_hashed_bytes += (unsigned long)wParam;
      file_hashed_bytes  += (unsigned long)wParam;

      if (timeGetTime () - last_update > 125) {
        wchar_t wszTotalSize [64] = { L'\0' };

        _swprintf ( wszTotalSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)total_hashed_bytes    / (1024.0 * 1024.0),
                          (double)total_download_size / (1024.0 * 1024.0) );

        dlc_op.set         (RGB (248, 248, 22), L" HASH ", installing->wszName);
        dlc_total_size.set (wszTotalSize);

        dlc_total_progress.set (
          hWndDlg,
            (int32_t)((double)std::numeric_limits <int32_t>::max () *
              ((double)total_downloaded_bytes / (double)total_download_size)) );

        dlc_current_file.set (file_to_hash.wszLocalPath);

        wchar_t wszFileSize [64] = { L'\0' };

        _swprintf ( wszFileSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)file_hashed_bytes   / (1024.0 * 1024.0),
                          (double)file_to_hash.size / (1024.0 * 1024.0) );

        dlc_current_size.set (wszFileSize);

        last_update = timeGetTime ();
      }
    } break;

    case WM_COMBINE_PROGRESS:
    {
      static uint32_t last_update = 0;

      total_combined_bytes += (unsigned long)wParam;
      file_combined_bytes  += (unsigned long)wParam;

      if (timeGetTime () - last_update > 125) {
        wchar_t wszTotalSize [64] = { L'\0' };

        _swprintf ( wszTotalSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)total_combined_bytes  / (1024.0 * 1024.0),
                          (double)total_download_size / (1024.0 * 1024.0) );

        dlc_op.set         (RGB (248, 248, 248), L" BUILD ", installing->wszName);
        dlc_total_size.set (wszTotalSize);

        dlc_total_progress.set (
          hWndDlg,
            (int32_t)((double)std::numeric_limits <int32_t>::max () *
              ((double)total_downloaded_bytes / (double)total_download_size)) );

        dlc_current_file.set (file_to_combine.wszLocalPath);

        wchar_t wszFileSize [64] = { L'\0' };

        _swprintf ( wszFileSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)file_combined_bytes    / (1024.0 * 1024.0),
                          (double)file_to_combine.size / (1024.0 * 1024.0) );

        dlc_current_size.set (wszFileSize);

        last_update = timeGetTime ();
      }
    } break;

    case WM_FILE_DONE:
    {
      file_downloaded_bytes = 0UL;

      if (files_to_download.size () > 0) {
        memcpy ( &file_to_download,
                  &files_to_download.front (),
                    sizeof sk_internet_get_t );

        files_to_download.pop ();

        extern bool SKIM_CreateDirectories ( const wchar_t* wszPath );
        SKIM_CreateDirectories (file_to_download.wszAppend);
        SKIM_CreateDirectories (file_to_download.wszLocalPath);

        if (! wcslen (file_to_download.wszAppend))
          final_validate = true;

        // TODO: Validate file size
        if (GetFileAttributes (file_to_download.wszLocalPath) == INVALID_FILE_ATTRIBUTES) {
          files_to_hash.push (file_to_download);

          hWorkerThread =
            CreateThread (nullptr, 0, DownloadThread, &file_to_download, 0x00, nullptr);
        } else {
          files_to_hash.push (file_to_download);
          total_downloaded_bytes += file_to_download.size;
          SendMessage (hWndDlg, WM_FILE_DONE, 0x00, 0x00);
        }
      }

      else {
        if (files_to_hash.size () > 0) {
          SendMessage (hWndDlg, WM_HASH_DONE, 0x00, 0x00);
        }
      }
    } break;

    case WM_HASH_DONE:
    {
      file_hashed_bytes = 0UL;

      if (files_to_hash.size () > 0) {
        memcpy ( &file_to_hash,
                   &files_to_hash.front (),
                     sizeof sk_internet_get_t );

        // TODO: Split into multiple multi-part files if need be based on the
        //         final filename.
        if (wcslen (file_to_hash.wszAppend)) {
          files_to_combine.push (file_to_hash);
        } else {
          final_validate = true;
        }

        files_to_hash.pop ();

        hWorkerThread =
          CreateThread (nullptr, 0, HashThread, &file_to_hash, 0x00, nullptr);
      }

      else if (! final_validate) {
        if (files_to_combine.size () > 0) {
          memcpy ( &file_to_combine,
                     &files_to_combine.front (),
                       sizeof sk_internet_get_t );

          hWorkerThread =
            CreateThread (nullptr, 0, CombineThread, &file_to_combine, 0x00, nullptr);
        }
      }
    } break;

    case WM_INITDIALOG:
    {
      if (hTerminateEvent != 0)
        ResetEvent (hTerminateEvent);

      total_downloaded_bytes = 0;
      file_downloaded_bytes  = 0;

      total_download_size    = 0;
      total_hashed_bytes     = 0;
      total_combined_bytes   = 0;

      hWndDownloadDialog = hWndDlg;

      CreateWindowEx ( 0, STATUSCLASSNAME,
                         nullptr,
                           WS_CHILD | WS_VISIBLE |
                           SBARS_SIZEGRIP,
                              0, 0, 0, 0,
                                hWndDlg,
                                  (HMENU)IDC_STATUS,
                                      GetModuleHandle (nullptr), nullptr);

       dlc_total_size.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("CURRENT DOWNLOAD SIZE"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_RIGHT   | ES_SAVESEL | WS_DISABLED,
                           260, 10, 193, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

       dlc_op.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("CURRENT DOWNLOAD OP"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_LEFT    | ES_SAVESEL | WS_DISABLED,
                           10, 10, 250, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

      dlc_current_file.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("SINGLE FILE NAME"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_LEFT    | ES_SAVESEL | WS_DISABLED,
                           10, 69, 250, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

      dlc_current_size.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("SINGLE FILE SIZE"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_RIGHT   | ES_SAVESEL | WS_DISABLED,
                           260, 69, 193, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

      std::vector <HANDLE> head_threads;

      for ( auto it  = files_to_lookup.begin ();
                 it != files_to_lookup.end   ();
               ++it                              ) {
        it->hThread =
          CreateThread (
            nullptr,
              0,
                HeaderThread,
                  (LPVOID)&(*it),
                    0x00,
                      nullptr
          );
        head_threads.push_back (it->hThread);
      }

      WaitForMultipleObjects ( (DWORD)head_threads.size (), &head_threads [0], TRUE, INFINITE );

      for ( auto it  = files_to_lookup.begin ();
                 it != files_to_lookup.end   ();
               ++it                              ) {
        sk_internet_get_t get =
          files_to_download.front ();

        files_to_download.pop ();

        get.size = it->size;

        files_to_download.push (get);

        total_download_size += it->size;
      }

      hWndFileProgress =
        GetDlgItem (hWndDlg, IDC_DLC_FILE_PROGRESS);

      SendMessage (hWndFileProgress, PBM_SETSTATE,    PBST_NORMAL, 0UL);
      SendMessage (hWndFileProgress, PBM_SETRANGE32,  0UL,         std::numeric_limits <int32_t>::max ());
      SendMessage (hWndFileProgress, PBM_SETPOS,      0,           0UL);

      hWndTotalProgress =
        GetDlgItem (hWndDlg, IDC_DLC_TOTAL_PROGRESS);

      SendMessage (hWndTotalProgress, PBM_SETRANGE32, 0UL,         std::numeric_limits <int32_t>::max ());
      SendMessage (hWndTotalProgress, PBM_SETPOS,     0,           0UL);
      SendMessage (hWndTotalProgress, PBM_SETSTATE,   PBST_PAUSED, 0UL);

      while (! files_to_combine.empty ())
        files_to_combine.pop ();

      while (! files_to_delete.empty ())
        files_to_delete.pop ();

      SendMessage (hWndDlg, WM_FILE_DONE, 0, 0);

      return (INT_PTR)true;
    }

    case WM_CLOSE:
    case WM_DESTROY:
    {
      files_to_lookup.clear   ();

      while (! files_to_download.empty ())
        files_to_download.pop ();

      while (! files_to_hash.empty ())
        files_to_hash.pop ();

      while (! files_to_combine.empty ())
        files_to_combine.pop ();

      while (! files_to_delete.empty ())
        files_to_delete.pop ();

      final_validate = false;

      total_download_size = 0;
      file_download_size  = 0;

      DestroyWindow (dlc_op.hWndCtl);
      DestroyWindow (dlc_total_size.hWndCtl);
      DestroyWindow (dlc_current_file.hWndCtl);
      DestroyWindow (dlc_current_size.hWndCtl);

      dlc_op.set           (RGB (0,0,0), L"", L"");
      dlc_current_file.set (L"");

      if (hWorkerThread != 0 && hTerminateEvent) {
        SetEvent (hTerminateEvent);
        hWorkerThread = 0;
      }

      hWndDownloadDialog = 0;
      EndDialog   (hWndDlg, 0x0);

      return (INT_PTR)true;
    }

    case WM_CREATE:
    case WM_PAINT:
    case WM_SIZE:
      return (INT_PTR)false;
  }

  return (INT_PTR)false;
}

unsigned int
__stdcall
DLCDlg_Thread (LPVOID user)
{
  static bool init = false;

  if (! init) {
    INITCOMMONCONTROLSEX icex;
    ZeroMemory (&icex, sizeof INITCOMMONCONTROLSEX);

    icex.dwSize = sizeof INITCOMMONCONTROLSEX;
    icex.dwICC  = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;

    InitCommonControlsEx (&icex);

    LoadLibrary (L"Msftedit.dll");

    init = true;
  }

  UNREFERENCED_PARAMETER (user);

  hWndDLCMgr =
    CreateDialog ( GetModuleHandle (nullptr),
                     MAKEINTRESOURCE (IDD_DLC_MANAGER),
                       (HWND)0,
                         DLCMgr_DlgProc );

  if (hWndDLCMgr != 0)
    SetWindowText (hWndDLCMgr, L"Manage DLC");

  MSG  msg;
  BOOL bRet;

  static bool last_downloading = false;

  while (hWndDLCMgr != 0 && (bRet = GetMessage (&msg, hWndDLCMgr, 0, 0)) != 0)
  {
    if (hWndDownloadDialog != 0) {
      last_downloading = true;
      continue;
    }

    if (last_downloading && hWndDLCMgr != 0) {
      SendMessage (hWndDLCMgr, WM_INITDIALOG, 0x00, 0x00);
      last_downloading = false;
    }

    if (bRet == -1) {
      return 0;
    }

    if (hWndDLCMgr != 0) {
      TranslateMessage (&msg);
      DispatchMessage  (&msg);
    }
  }

  extern HWND hWndMainDlg;
  ShowWindow (hWndMainDlg, SW_SHOW);

  return 0;
}

unsigned int
__stdcall
DLCDownload_Thread (LPVOID user)
{
  hWndDownloadDialog =
    CreateDialog ( GetModuleHandle (nullptr),
                     MAKEINTRESOURCE (IDD_DLC_INSTALL),
                       (HWND)user,
                         DLC_DlgProc );

  MSG  msg;
  BOOL bRet;

  while (hWndDownloadDialog != 0 && (bRet = GetMessage (&msg, hWndDownloadDialog, 0, 0)) != 0)
  {
    if (bRet == -1) {
      return 0;
    }

    if (hWndDownloadDialog != 0) {
      TranslateMessage (&msg);
      DispatchMessage  (&msg);
    }
  }

  return 0;
}