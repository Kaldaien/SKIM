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

struct sk_internet_get_t {
  wchar_t  wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t  wszHostPath  [INTERNET_MAX_PATH_LENGTH];
  wchar_t  wszLocalPath [MAX_PATH];
  wchar_t  wszAppend    [MAX_PATH]; // If non-zero length, then append
                                    //   to this file.
  HWND     hProgressCtrl;
  HWND     hDLCDlg;
  size_t   size;
  uint32_t crc32c;
  int      status;
};

struct sk_internet_head_t {
  wchar_t wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t wszHostPath  [INTERNET_MAX_PATH_LENGTH];
  HWND    hTaskDlg;
  HANDLE  hThread;
  size_t  size;
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

  while (files_to_combine.size () > 0) {
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
          if (get->hProgressCtrl != 0)
            return SendMessage ( get->hProgressCtrl,
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

      SendMessage ( get->hDLCDlg, WM_COMBINE_PROGRESS, dwBytesRead, 0x00 );
    }

    SendMessage ( get->hDLCDlg, WM_COMBINE_DONE, 0x00, 0x00 );

    CloseHandle (hPartialFile);
  }


  if (files_to_combine.size () == 0) {
    file_to_combine.crc32c = 0x41EDFD6F;
    file_to_combine.size   = total_download_size;
    wcscpy (file_to_combine.wszLocalPath, file_to_combine.wszAppend);

    files_to_hash.push (file_to_combine);

    final_validate = true;

    SendMessage ( file_to_combine.hDLCDlg, WM_HASH_DONE, 0x00, 0x00 );
  }


  CloseHandle (hCombinedFile);

  return true;
}

void
__stdcall
SK_HashProgressCallback (uint64_t current, uint64_t total)
{
  sk_internet_get_t* get =
    (sk_internet_get_t *)&file_to_hash;

  auto ProgressMsg =
    [get](UINT Msg, WPARAM wParam, LPARAM lParam) ->
      LRESULT
      {
        if (get->hProgressCtrl != 0)
          return SendMessage ( get->hProgressCtrl,
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
    SendMessage ( get->hDLCDlg, WM_HASH_PROGRESS, (current - file_hashed_bytes), 0UL);
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
        if (get->hProgressCtrl != 0)
          return SendMessage ( get->hProgressCtrl,
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

  PostMessage ( get->hDLCDlg, WM_COMBINE_DONE, 0x00, 0x00 );

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
        if (get->hProgressCtrl != 0)
          return SendMessage ( get->hProgressCtrl,
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

  HMODULE hSK64 = LoadLibrary (L"installer64.dll");

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

    MessageBox (nullptr, wszCRC32C, L"Checksum Mismatch", MB_OK);

    // Delete and re-download
    DeleteFileW (get->wszLocalPath);

    ExitProcess (0x00);
  }


  //
  // DLC SUCCESS, delete temp files
  //
  else if (final_validate) {
    MessageBox ( NULL,
                  L"DLC Successfully Installed",
                    L"Special K DLC Install",
                      MB_OK | MB_ICONINFORMATION );

    DeleteFileW (L"TSFix_Res\\inject\\99_Upscale4x.7z.001");
    DeleteFileW (L"TSFix_Res\\inject\\99_Upscale4x.7z.002");
    DeleteFileW (L"TSFix_Res\\inject\\99_Upscale4x.7z.003");

    DeleteFileW (L"installer64.dll");

    ExitProcess (0x00);
  }

  PostMessage ( get->hDLCDlg, WM_HASH_DONE, 0x00, 0x00 );

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
        if (get->hProgressCtrl != 0)
          return SendMessage ( get->hProgressCtrl,
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

    DWORD dwContentLength;
    DWORD dwContentLength_Len = sizeof DWORD;
    DWORD dwSize;

    DWORD dwIdx = 0;

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

          SetProgress (                                dwTotalBytesDownloaded, dwContentLength);
          SendMessage (get->hDLCDlg, WM_FILE_PROGRESS, dwRead,                 0);
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

      PostMessage (get->hDLCDlg, WM_FILE_DONE, 0, 0);

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

    DWORD dwContentLength;
    DWORD dwContentLength_Len = sizeof DWORD;
    DWORD dwSize;

    DWORD dwIdx = 0;

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
    PostMessage (hWnd, TDM_SET_PROGRESS_BAR_RANGE, 0L,          MAKEWPARAM (0, 1));
    PostMessage (hWnd, TDM_SET_PROGRESS_BAR_POS,   1,           0L);
    PostMessage (hWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0L);

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

int
__stdcall
DecompressionProgressCallback (int current, int total)
{
  static int last_total = 0;

  HWND hWndProgress = 0;
    //GetDlgItem (hWndUpdateDlg, IDC_UPDATE_PROGRESS);

  if (total != last_total) {
    PostMessage (hWndProgress, PBM_SETSTATE, PBST_NORMAL, 0UL);
    PostMessage (hWndProgress, PBM_SETRANGE, 0UL, MAKEWPARAM (0, total));
    total = last_total;
  }

  PostMessage (hWndProgress, PBM_SETPOS, current, 0UL);

  return 0;
}

static volatile LONG __SK_UpdateStatus = 0;

#include <Windows.h>
#include <Richedit.h>

HWND hWndRichTotalDesc;
HWND hWndRichFileDesc;

INT_PTR
CALLBACK
DLC_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  switch (uMsg) {
    case WM_FILE_PROGRESS:
    {
      total_downloaded_bytes += wParam;
      file_downloaded_bytes  += wParam;

      wchar_t wszTotalSize [64] = { L'\0' };

      _swprintf ( wszTotalSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)total_downloaded_bytes / (1024.0 * 1024.0),
                        (double)total_download_size  / (1024.0 * 1024.0) );

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (22, 248, 22);
      cf2.crBackColor = RGB (10, 10,  10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        0,      wcslen (L" GET "));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L" GET ");

      DWORD dwEndSel = wcslen (L" GET ");

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_EFFECTS;
      cf2.dwEffects   = CFE_BOLD | CFE_AUTOBACKCOLOR;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"4K Upscale Texture Pack"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"4K Upscale Texture Pack");

      dwEndSel += wcslen (L"4K Upscale Texture Pack");

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszTotalSize));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)wszTotalSize);

      HWND hWndTotalProgress =
        GetDlgItem (hWndDlg, IDC_DLC_TOTAL_PROGRESS);

      SendMessage ( hWndTotalProgress,
                      PBM_SETPOS,
                        std::numeric_limits <int32_t>::max () *
                        ((double)total_downloaded_bytes / (double)total_download_size),
                          0UL );

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        0,      wcslen (file_to_download.wszLocalPath));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)file_to_download.wszLocalPath);

      dwEndSel = wcslen (file_to_download.wszLocalPath);

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,        (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      wchar_t wszFileSize [64] = { L'\0' };

      _swprintf ( wszFileSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)file_downloaded_bytes   / (1024.0 * 1024.0),
                        (double)file_to_download.size / (1024.0 * 1024.0) );

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszFileSize));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)wszFileSize);
    } break;

    case WM_HASH_PROGRESS:
    {
      total_hashed_bytes += (unsigned long)wParam;
      file_hashed_bytes  += (unsigned long)wParam;

      wchar_t wszTotalSize [64] = { L'\0' };

      _swprintf ( wszTotalSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)total_hashed_bytes    / (1024.0 * 1024.0),
                        (double)total_download_size / (1024.0 * 1024.0) );

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (248, 248, 22);
      cf2.crBackColor = RGB (10, 10,  10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        0,      wcslen (L" HASH "));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L" HASH ");

      DWORD dwEndSel = wcslen (L" HASH ");

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_EFFECTS;
      cf2.dwEffects   = CFE_BOLD | CFE_AUTOBACKCOLOR;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"4K Upscale Texture Pack"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"4K Upscale Texture Pack");

      dwEndSel += wcslen (L"4K Upscale Texture Pack");

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszTotalSize));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)wszTotalSize);

      HWND hWndTotalProgress =
        GetDlgItem (hWndDlg, IDC_DLC_TOTAL_PROGRESS);

      SendMessage ( hWndTotalProgress,
                      PBM_SETPOS,
                        std::numeric_limits <int32_t>::max () *
                        ((double)total_hashed_bytes / (double)total_download_size),
                          0UL );

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        0,      wcslen (file_to_hash.wszLocalPath));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)file_to_hash.wszLocalPath);

      dwEndSel = wcslen (file_to_hash.wszLocalPath);

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,        (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      wchar_t wszFileSize [64] = { L'\0' };

      _swprintf ( wszFileSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)file_hashed_bytes   / (1024.0 * 1024.0),
                        (double)file_to_hash.size / (1024.0 * 1024.0) );

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszFileSize));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)wszFileSize);
    } break;

    case WM_COMBINE_PROGRESS:
    {
      total_combined_bytes += (unsigned long)wParam;
      file_combined_bytes  += (unsigned long)wParam;

      wchar_t wszTotalSize [64] = { L'\0' };

      _swprintf ( wszTotalSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)total_combined_bytes  / (1024.0 * 1024.0),
                        (double)total_download_size / (1024.0 * 1024.0) );

      CHARFORMAT2W cf2;
      ZeroMemory (&cf2, sizeof CHARFORMAT2W);

      cf2.cbSize = sizeof CHARFORMAT2W;

      cf2.crTextColor = RGB (248, 248, 248);
      cf2.crBackColor = RGB (10, 10,  10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        0,      wcslen (L" BUILD "));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L" BUILD ");

      DWORD dwEndSel = wcslen (L" BUILD ");

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR | CFM_EFFECTS;
      cf2.dwEffects   = CFE_BOLD | CFE_AUTOBACKCOLOR;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"4K Upscale Texture Pack"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"4K Upscale Texture Pack");

      dwEndSel += wcslen (L"4K Upscale Texture Pack");

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichTotalDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszTotalSize));
      SendMessage (hWndRichTotalDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichTotalDesc, EM_REPLACESEL, FALSE, (LPARAM)wszTotalSize);

      HWND hWndTotalProgress =
        GetDlgItem (hWndDlg, IDC_DLC_TOTAL_PROGRESS);

      SendMessage ( hWndTotalProgress,
                      PBM_SETPOS,
                        std::numeric_limits <int32_t>::max () *
                        ((double)total_combined_bytes / (double)total_download_size),
                          0UL );

      cf2.crTextColor = RGB (10, 10, 10);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        0,      wcslen (file_to_combine.wszLocalPath));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)file_to_combine.wszLocalPath);

      dwEndSel = wcslen (file_to_combine.wszLocalPath);

      cf2.crTextColor = RGB (10, 10, 255);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (L"\t"));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,        (LPARAM)L"\t");

      dwEndSel += wcslen (L"\t");

      wchar_t wszFileSize [64] = { L'\0' };

      _swprintf ( wszFileSize,
                    L"%7.2f MiB / %7.2f MiB",
                      (double)file_combined_bytes    / (1024.0 * 1024.0),
                        (double)file_to_combine.size / (1024.0 * 1024.0) );

      cf2.crTextColor = RGB (248, 22, 22);
      cf2.dwMask      = CFM_BOLD | CFM_COLOR;
      cf2.dwEffects   = CFE_BOLD;

      SendMessage (hWndRichFileDesc, EM_SETSEL,        dwEndSel,      dwEndSel + wcslen (wszFileSize));
      SendMessage (hWndRichFileDesc, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
      SendMessage (hWndRichFileDesc, EM_REPLACESEL,    FALSE,         (LPARAM)wszFileSize);
    } break;

    case WM_FILE_DONE:
    {
      file_downloaded_bytes = 0UL;

      if (files_to_download.size () > 0) {
        memcpy ( &file_to_download,
                  &files_to_download.front (),
                    sizeof sk_internet_get_t );

        files_to_hash.push    (file_to_download);
        files_to_download.pop ();

        extern bool SKIM_CreateDirectories ( const wchar_t* wszPath );
        SKIM_CreateDirectories (file_to_download.wszAppend);
        SKIM_CreateDirectories (file_to_download.wszLocalPath);

        // TODO: Validate file size
        if (GetFileAttributes (file_to_download.wszLocalPath) == INVALID_FILE_ATTRIBUTES) {
          CreateThread (nullptr, 0, DownloadThread, &file_to_download, 0x00, nullptr);
        } else {
          total_downloaded_bytes += file_to_download.size;
          PostMessage (hWndDlg, WM_FILE_DONE, 0x00, 0x00);
        }
      }

      else {
        if (files_to_hash.size () > 0) {
          PostMessage (hWndDlg, WM_HASH_DONE, 0x00, 0x00);
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
        }

        files_to_hash.pop ();

        CreateThread (nullptr, 0, HashThread, &file_to_hash, 0x00, nullptr);
      }

      else if (! final_validate) {
        if (files_to_combine.size () > 0) {
          memcpy ( &file_to_combine,
                     &files_to_combine.front (),
                       sizeof sk_internet_get_t );

          CreateThread (nullptr, 0, CombineThread, &file_to_combine, 0x00, nullptr);
        }
      }
    } break;

    case WM_INITDIALOG:
    {
      HWND hWndStatus =
        CreateWindowEx ( 0, STATUSCLASSNAME,
                           nullptr,
                             WS_CHILD | WS_VISIBLE |
                             SBARS_SIZEGRIP,
                                0, 0, 0, 0,
                                  hWndDlg,
                                    (HMENU)IDC_STATUS,
                                        GetModuleHandle (nullptr), nullptr);

       hWndRichTotalDesc =
        CreateWindowEx( 0, MSFTEDIT_CLASS, TEXT("I WANT TO SUCK YOUR BYTES"),
                          WS_VISIBLE | WS_CHILD   | ES_READONLY |
                          ES_LEFT    | ES_SAVESEL | WS_DISABLED,
                          10, 10, 448, 28,
                          hWndDlg, NULL, GetModuleHandle (nullptr), NULL);

      hWndRichFileDesc =
        CreateWindowEx( 0, MSFTEDIT_CLASS, TEXT("I AM A SINGLE FILE"),
                          WS_VISIBLE | WS_CHILD   | ES_READONLY |
                          ES_CENTER  | ES_SAVESEL | WS_DISABLED,
                          10, 68, 448, 28,
                          hWndDlg, NULL, GetModuleHandle (nullptr), NULL);

      files_to_lookup.push_back (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.001",
              hWndDlg,
                INVALID_HANDLE_VALUE,
                  0 }
      );
      files_to_lookup.push_back (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.002",
              hWndDlg,
                INVALID_HANDLE_VALUE,
                  0 }
      );
      files_to_lookup.push_back (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.003",
              hWndDlg,
                INVALID_HANDLE_VALUE,
                  0 }
      );

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

      WaitForMultipleObjects ( head_threads.size (), &head_threads [0], TRUE, INFINITE );

      for ( auto it  = files_to_lookup.begin ();
                 it != files_to_lookup.end   ();
               ++it                              ) {
        total_download_size += it->size;
      }

      HWND hWndFileProgress =
        GetDlgItem (hWndDlg, IDC_DLC_FILE_PROGRESS);

      PostMessage (hWndFileProgress, PBM_SETSTATE,    PBST_NORMAL, 0UL);
      PostMessage (hWndFileProgress, PBM_SETRANGE32,  0UL,         std::numeric_limits <int32_t>::max ());
      PostMessage (hWndFileProgress, PBM_SETPOS,      0,           0UL);

      HWND hWndTotalProgress =
        GetDlgItem (hWndDlg, IDC_DLC_TOTAL_PROGRESS);

      PostMessage (hWndTotalProgress, PBM_SETRANGE32, 0UL,         std::numeric_limits <int32_t>::max ());
      PostMessage (hWndTotalProgress, PBM_SETPOS,     0,           0UL);
      PostMessage (hWndTotalProgress, PBM_SETSTATE,   PBST_PAUSED, 0UL);

      files_to_download.push (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.001",
              L"TSFix_Res\\inject\\99_Upscale4x.7z.001",
                L"TSFix_Res\\inject\\99_Upscale4x.7z",
                  hWndFileProgress,
                    hWndDlg,
                      files_to_lookup [0].size,
                        0xDEFC02D3,
                          0 }
      );
      files_to_download.push (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.002",
              L"TSFix_Res\\inject\\99_Upscale4x.7z.002",
                L"TSFix_Res\\inject\\99_Upscale4x.7z",
                  hWndFileProgress,
                    hWndDlg,
                      files_to_lookup [1].size,
                        0xA570FCE9,
                          0 }
      );
      files_to_download.push (
        { L"github.com",
            L"/Kaldaien/TSF/releases/download/tsfix_090/99_Upscale4x.7z.003",
              L"TSFix_Res\\inject\\99_Upscale4x.7z.003",
                L"TSFix_Res\\inject\\99_Upscale4x.7z",
                  hWndFileProgress,
                    hWndDlg,
                      files_to_lookup [2].size,
                        0xF69DBD47,
                          0 }
      );

      // Start fresh!
      DeleteFileW (L"TSFix_Res\\inject\\99_Upscale4x.7z");

      PostMessage (hWndDlg, WM_FILE_DONE, 0, 0);

      return (INT_PTR)true;
    }

    case WM_CLOSE:
    case WM_DESTROY:
    {
      EndDialog (hWndDlg, 0x0);
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
  CreateDialog ( GetModuleHandle (nullptr),
                   MAKEINTRESOURCE (IDD_DLC),
                    (HWND)0,
                      DLC_DlgProc );

  MSG  msg;
  BOOL bRet;

  while ((bRet = GetMessage (&msg, nullptr, 0, 0)) != 0)
  {
    if (bRet == -1) {
      return 0;
    }

    TranslateMessage (&msg);
    DispatchMessage  (&msg);
  }

  return 0;
}

HRESULT
__stdcall
SKIM_FetchOptionalDLC (const wchar_t* wszURL)
{
  int               nButtonPressed =   0;
  TASKDIALOGCONFIG  task_config    = { 0 };

  int idx = 0;

  task_config.cbSize             = sizeof (task_config);
  task_config.hInstance          = GetModuleHandle (nullptr);
  task_config.hwndParent         = GetActiveWindow ();

  task_config.pszWindowTitle   = L"Special K DLC Install";

  TASKDIALOG_BUTTON buttons [3];

  buttons [0].nButtonID     = IDYES;
  buttons [0].pszButtonText = L"Yes";

  buttons [1].nButtonID     = 0;
  buttons [1].pszButtonText = L"Remind me later (or disable)";


  task_config.pButtons           = buttons;
  task_config.cButtons           = 2;
  task_config.dwCommonButtons    = 0x00;
  task_config.nDefaultButton     = 0;

  task_config.dwFlags            = TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR   | TDF_SIZE_TO_CONTENT |
                                   TDF_CALLBACK_TIMER    | TDF_EXPANDED_BY_DEFAULT | TDF_EXPAND_FOOTER_AREA |
                                   TDF_POSITION_RELATIVE_TO_WINDOW;
  task_config.pfCallback         = DownloadDialogCallback;

  task_config.pszMainInstruction = L"Software Add-On Ready to Download";
  task_config.pszMainIcon        = TD_INFORMATION_ICON;

  task_config.pszContent         = L"Would you like to begin installation now?";

  sk_internet_get_t* get =
    new sk_internet_get_t;

  URL_COMPONENTSW    urlcomps;

  ZeroMemory (get,       sizeof *get);
  ZeroMemory (&urlcomps, sizeof URL_COMPONENTSW);

  urlcomps.dwStructSize     = sizeof URL_COMPONENTSW;

/*
  urlcomps.lpszHostName     = get->wszHostName;
  urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

  urlcomps.lpszUrlPath      = get->wszHostPath;
  urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

  if ( InternetCrackUrl ( archive.get_value (L"URL").c_str (),
                            archive.get_value (L"URL").length (),
                              0x00,
                                &urlcomps ) )
  {
    task_config.lpCallbackData = (LONG_PTR)get;

    wchar_t   wszUpdateFile [MAX_PATH] = { L'\0' };

    int nButton = 0;

    MSG  msg;
    BOOL bRet;

    if ((bRet = GetMessage (&msg, 0, 0, 0)) != 0)
    {
      if (bRet == -1)
        break;

      TranslateMessage (&msg);
      DispatchMessage  (&msg);
    }

    if (SUCCEEDED (TaskDialogIndirect (&task_config, &nButton, nullptr, nullptr))) {
      if (get->status == STATUS_UPDATED) {
        InterlockedExchangeAcquire ( &__SK_UpdateStatus, 0 );

        _beginthreadex ( nullptr,
                           0,
                             UpdateDlg_Thread,
                               nullptr,
                                 0x00,
                                   nullptr );

        LONG status = 0;

        while ( ( status =
                    InterlockedCompareExchange ( &__SK_UpdateStatus,
                                                   0,
                                                     0 )
                ) == 0
              )
          Sleep (15);

        if ( InterlockedCompareExchange ( &__SK_UpdateStatus, 0, 0 ) == 1 )
        {
           TerminateProcess (GetCurrentProcess (), 0x00);

          // Laughably, my own software is designed to prevent
          //   TerminateProcess (...) in some situations, so this
          //     fallback is necessary.
          ExitProcess      (0x00);

          return S_OK;
        }

        // Update Failed
        else {
          return E_FAIL;
        }
      }
    }

    delete get;
  }

  else {
    delete get;
    return E_FAIL;
  }
*/

  return S_FALSE;
}