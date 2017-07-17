#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#include <cstdint>

#include <WinInet.h>
#pragma comment (lib, "wininet.lib")

#include "product.h"
#include "SKIM.h"
#include <Richedit.h>
#include "resource.h"

#include <queue>
#include <atlbase.h>

#include <CommCtrl.h>
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")
#pragma comment (lib,    "winmm.lib")

bool
__stdcall
SKIM_FetchDLCManagerDLL (  sk_product_t&  product,
                           const wchar_t *wszRemoteFile =
#ifndef _WIN64
L"installer.dll"
#else
L"installer64.dll"
#endif
)
{
UNREFERENCED_PARAMETER (product);

  DWORD dwInetCtx = 0;

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K Install Manager",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    return false;

  HINTERNET hInetGitHub =
    InternetConnect ( hInetRoot,
                        L"raw.githubusercontent.com",
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  if (! hInetGitHub) {
    InternetCloseHandle (hInetRoot);
    return false;
  }

  wchar_t wszRemoteRepoURL [MAX_PATH] = { };

  wsprintf ( wszRemoteRepoURL,
               L"/Kaldaien/SpecialK/0.8.x/%s",
                /*product.wszRepoName,*/ wszRemoteFile );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_MAKE_PERSISTENT   | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                  INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC              |
                                  INTERNET_FLAG_NEED_FILE,
                                    (DWORD_PTR)&dwInetCtx );

  if (! hInetGitHubOpen) {
    InternetCloseHandle (hInetGitHub);
    InternetCloseHandle (hInetRoot);
    return false;
  }

  if ( HttpSendRequestW ( hInetGitHubOpen,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) )
  {
    DWORD dwSize;

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      DWORD dwAttribs = GetFileAttributes (wszRemoteFile);

      if (dwAttribs == INVALID_FILE_ATTRIBUTES)
        dwAttribs = FILE_ATTRIBUTE_NORMAL;

      CHandle hVersionFile (
        CreateFileW ( wszRemoteFile,
                        GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr,
                              CREATE_ALWAYS,
                                dwAttribs |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr ) );

      while (hVersionFile != INVALID_HANDLE_VALUE && dwSize > 0)
      {
        DWORD    dwRead = 0;

        CHeapPtr <uint8_t> pData;
        if (! pData.Allocate (dwSize))
          break;

        if ( InternetReadFile ( hInetGitHubOpen,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten;

          WriteFile ( hVersionFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );
        }

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }
    }

    HttpEndRequest ( hInetGitHubOpen, nullptr, 0x00, 0 );
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot);

  return true;
}

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
  wchar_t            wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH];
  wchar_t            wszHostPath  [INTERNET_MAX_PATH_LENGTH];
  HANDLE             hThread;
  uint64_t           size;
};

enum {
  WM_FILE_PROGRESS = WM_APP,
  WM_FILE_DONE,
  WM_FILE_CANCELLED
};

enum {
  STATUS_FETCHED   = 1,
  STATUS_OTHER     = 2,
  STATUS_CANCELLED = 4,
  STATUS_FAILED    = 8
};

static std::queue  <sk_internet_get_t>  files_to_fetch;
static std::vector <sk_internet_head_t> files_to_lookup;
static sk_internet_get_t                file_to_fetch;

static uint64_t           total_fetch_size    = 0ULL;
static uint64_t           total_fetched_bytes = 0ULL;

static uint32_t           file_fetched_size   = 0UL;
static uint32_t           file_fetched_bytes  = 0UL;

static HWND               hWndDownloadDialog  = 0;
static HANDLE             hWorkerThread       = 0;
static HANDLE             hTerminateEvent     = 0;

static HWND               hWndFileProgress;
static HWND               hWndTotalProgress;

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
} fetch_op;

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
} fetch_total_progress;

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
} fetch_file_progress;

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
} fetch_current_file;

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
} fetch_current_size;

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
} fetch_total_size;

DWORD
__stdcall
HeaderThread (LPVOID user)
{
  sk_internet_head_t* head =
    (sk_internet_head_t *)user;

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K Install Manager", //L"DLC Grabber",
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

static
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
                            ((double)cur / std::max (0.0001, (double)max))
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
      CHandle hGetFile (
        CreateFileW ( get->wszLocalPath,
                        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr ) );

      while (hGetFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        if (WaitForSingleObject (hTerminateEvent, 0) == WAIT_OBJECT_0) {
          break;
        }

        DWORD    dwRead = 0;

        CHeapPtr <uint8_t> (pData);

        if (! pData.Allocate (dwSize))
          break;

        if ( InternetReadFile ( hInetHTTPGetReq,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten = 0;

          WriteFile ( hGetFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );

          dwTotalBytesDownloaded += dwRead;

          SetProgress (                        dwTotalBytesDownloaded, dwContentLength);
          SendMessage (hWndDownloadDialog, WM_FILE_PROGRESS, dwRead,                 0);
        }

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
    }

    get->status = STATUS_FETCHED;
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

INT_PTR
CALLBACK
Fetch_DlgProc (
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

      total_fetched_bytes += (uint32_t)wParam;
      file_fetched_bytes  += (uint32_t)wParam;

      if (timeGetTime () - last_update > 125) {
        wchar_t wszTotalSize [64] = { L'\0' };

        _swprintf ( wszTotalSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)total_fetched_bytes / (1024.0 * 1024.0),
                          (double)total_fetch_size  / (1024.0 * 1024.0) );

        fetch_op.set         (RGB (22, 248, 22), L" GET ", L"Special K Install DLL");
        fetch_total_size.set (wszTotalSize);

        fetch_total_progress.set (
          hWndDlg,
            (int32_t)((double)std::numeric_limits <int32_t>::max () *
              ((double)total_fetched_bytes / (double)total_fetch_size)) );

        fetch_current_file.set (file_to_fetch.wszLocalPath);

        wchar_t wszFileSize [64] = { L'\0' };

        _swprintf ( wszFileSize,
                      L"%7.2f MiB / %7.2f MiB",
                        (double)file_fetched_bytes   / (1024.0 * 1024.0),
                          (double)file_to_fetch.size / (1024.0 * 1024.0) );

        fetch_current_size.set (wszFileSize);

        last_update = timeGetTime ();
      }
    } break;

    case WM_FILE_DONE:
    {
      file_fetched_bytes = 0UL;

      if (files_to_fetch.size () > 0) {
        memcpy ( &file_to_fetch,
                  &files_to_fetch.front (),
                    sizeof sk_internet_get_t );

        files_to_fetch.pop ();

        SKIM_Util_CreateDirectories (file_to_fetch.wszAppend);
        SKIM_Util_CreateDirectories (file_to_fetch.wszLocalPath);

        hWorkerThread =
          CreateThread (nullptr, 0, DownloadThread, &file_to_fetch, 0x00, nullptr);
      }

      else if (files_to_fetch.empty ())
      {
        SendMessage (hWndDlg, WM_CLOSE, 0, 0);
      }
    } break;

    case WM_INITDIALOG:
    {
      SetWindowTextW (hWndDlg, L"SKIM Downloader");

      if (hTerminateEvent != 0)
        ResetEvent (hTerminateEvent);

      total_fetched_bytes = 0;
      file_fetched_bytes  = 0;

      total_fetch_size    = 0;

      hWndDownloadDialog = hWndDlg;

      CreateWindowEx ( 0, STATUSCLASSNAME,
                         nullptr,
                           WS_CHILD | WS_VISIBLE |
                           SBARS_SIZEGRIP,
                              0, 0, 0, 0,
                                hWndDlg,
                                  (HMENU)IDC_STATUS,
                                      GetModuleHandle (nullptr), nullptr);

       fetch_total_size.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("CURRENT DOWNLOAD SIZE"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_RIGHT   | ES_SAVESEL | WS_DISABLED,
                           260, 10, 193, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

       fetch_op.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("CURRENT DOWNLOAD OP"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_LEFT    | ES_SAVESEL | WS_DISABLED,
                           10, 10, 250, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

      fetch_current_file.hWndCtl =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("SINGLE FILE NAME"),
                           WS_VISIBLE | WS_CHILD   | ES_READONLY |
                           ES_LEFT    | ES_SAVESEL | WS_DISABLED,
                           10, 69, 250, 28,
                           hWndDlg, NULL, GetModuleHandle (nullptr), NULL );

      fetch_current_size.hWndCtl =
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
               ++it                              )
      {
        sk_internet_get_t get =
          files_to_fetch.front ();

        files_to_fetch.pop ();

        get.size   = it->size;

        files_to_fetch.push (get);

        total_fetch_size += it->size;
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

      SendMessage (hWndDlg, WM_FILE_DONE, 0, 0);

      return (INT_PTR)true;
    }

    case WM_CLOSE:
    case WM_DESTROY:
    {
      //files_to_lookup.clear   ();

      //while (! files_to_fetch.empty ())
      //  files_to_fetch.pop ();

      //while (! files_to_delete.empty ())
      //  files_to_delete.pop ();

      total_fetch_size  = 0;
      file_fetched_size = 0;

      DestroyWindow (fetch_op.hWndCtl);
      DestroyWindow (fetch_total_size.hWndCtl);
      DestroyWindow (fetch_current_file.hWndCtl);
      DestroyWindow (fetch_current_size.hWndCtl);

      fetch_op.set           (RGB (0,0,0), L"", L"");
      fetch_current_file.set (L"");

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

extern HWND hWndMainDlg;

DWORD
__stdcall
SKIM_Download_Thread (LPVOID user)
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

  hWndDownloadDialog =
    CreateDialog ( GetModuleHandle (nullptr),
                     MAKEINTRESOURCE (IDD_DLC_INSTALL),
                       (HWND)user,
                         Fetch_DlgProc );

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

bool
__stdcall
SKIM_FetchInstallerDLL (  sk_product_t& product,
                         const wchar_t *wszRemoteFile =
#ifndef _WIN64
L"installer.dll"
#else
L"installer64.dll"
#endif
)
{
  SKIM_Util_MoveFileNoFail (product.wszWrapper, L"Version/wrapper.old");

  wchar_t wszRemoteRepoURL [MAX_PATH * 2] = { };

  wsprintf ( wszRemoteRepoURL,
               L"/Kaldaien/SpecialK/0.8.x/%s",
                /*product.wszRepoName,*/ wszRemoteFile );

  static sk_internet_head_t head;
  static sk_internet_get_t  get;

  wcscpy (get.wszLocalPath, product.wszWrapper);
  wcscpy (get.wszHostName,  L"raw.githubusercontent.com");
  wcscpy (get.wszHostPath,  wszRemoteRepoURL);

  wcscpy (head.wszHostName,  L"raw.githubusercontent.com");
  wcscpy (head.wszHostPath,  wszRemoteRepoURL);

  files_to_fetch.push       (get);
  files_to_lookup.push_back (head);

  SKIM_Download_Thread (nullptr);

  Sleep (250UL);

  return true;
}

bool
__stdcall
SKIM_FetchInjectorDLL (  sk_product_t  product,
                         const wchar_t *wszRemoteFile =
#ifndef _WIN64
L"injector.dll"
#else
L"injector64.dll"
#endif
)
{
  wchar_t wszOld [MAX_PATH * 2] = { };
  swprintf (wszOld, L"Version/%s.old", wszRemoteFile);

  SKIM_Util_MoveFileNoFail (product.wszWrapper, wszOld);

  wchar_t wszRemoteRepoURL [MAX_PATH * 2] = { };

  wsprintf ( wszRemoteRepoURL,
               L"/Kaldaien/SpecialK/0.8.x/%s",
                /*product.wszRepoName,*/ wszRemoteFile );

  static sk_internet_head_t head;
  static sk_internet_get_t  get;

  wcscpy (get.wszLocalPath, product.wszWrapper);
  wcscpy (get.wszHostName,  L"raw.githubusercontent.com");
  wcscpy (get.wszHostPath,  wszRemoteRepoURL);

  wcscpy (head.wszHostName, L"raw.githubusercontent.com");
  wcscpy (head.wszHostPath, wszRemoteRepoURL);

  files_to_fetch.push       (get);
  files_to_lookup.push_back (head);

  SKIM_Download_Thread (nullptr);

  HANDLE hFile = 0;

  while (hFile == 0)
  {
    hFile = CreateFileW (product.wszWrapper, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, GetFileAttributes (product.wszWrapper), nullptr);

    if (hFile == 0)
      Sleep (133UL);
  }

  CloseHandle (hFile);

  return true;
}

bool
__stdcall
SKIM_FetchInstaller32 ( sk_product_t& product )
{
  return SKIM_FetchInstallerDLL ( product, L"installer.dll" );
}

bool
__stdcall
SKIM_FetchInstaller64 ( sk_product_t& product )
{
  return SKIM_FetchInstallerDLL ( product, L"installer64.dll" );
}



bool
__stdcall
SKIM_FetchInjector32 ( sk_product_t& product )
{
  return SKIM_FetchInjectorDLL ( product, L"injector.dll" );
}

bool
__stdcall
SKIM_FetchInjector64 ( sk_product_t& product )
{
  return SKIM_FetchInjectorDLL ( product, L"injector64.dll" );
}