// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <Windows.h>

#include <cstdint>

static uint32_t crc32_tab[] = { 
   0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 
   0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 
   0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 
   0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 
   0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 
   0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 
   0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 
   0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 
   0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 
   0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 
   0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 
   0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 
   0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 
   0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 
   0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 
   0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 
   0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 
   0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 
   0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 
   0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 
   0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 
   0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 
   0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 
   0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 
   0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 
   0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 
   0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 
   0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 
   0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 
   0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 
   0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 
   0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 
   0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 
   0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 
   0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 
   0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 
   0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 
   0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 
   0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 
   0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 
   0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 
   0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 
   0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d 
 };

uint32_t
crc32 (uint32_t crc, const void *buf, size_t size)
{
  const uint8_t *p;

  p = (uint8_t *)buf;
  crc = crc ^ ~0U;

  while (size--)
    crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ ~0U;
}

uint32_t
file_crc32 (const wchar_t* wszFilename)
{
  HANDLE hFile =
    CreateFileW ( wszFilename,
                    GENERIC_READ,
                      FILE_SHARE_READ,
                        nullptr,
                          OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                              nullptr );

  if (hFile == 0)
    return 0;

  DWORD dwSize, dwSizeHigh, dwRead;

  // This isn't a 4+ GiB file... so who the heck cares about the high-bits?
  dwSize = GetFileSize (hFile, &dwSizeHigh);

  void* data =
    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwSize);

  if (data == nullptr) {
    CloseHandle (hFile);
    return 0;
  }

  dwRead = dwSize;

  ReadFile (hFile, data, dwSize, &dwRead, nullptr);

  uint32_t filecrc32 =
    crc32 (0x00, data, dwRead);

  HeapFree (GetProcessHeap (), 0, data);

  CloseHandle (hFile);

  return filecrc32;
}

bool
CheckTouchServices (void)
{
  bool running = false;

  SC_HANDLE svc_ctl =
    OpenSCManagerW ( nullptr,
                       nullptr,
                         SC_MANAGER_ALL_ACCESS );

  if (svc_ctl) {
    SC_HANDLE tablet_svc =
      OpenServiceW ( svc_ctl,
                       L"TabletInputService",
                         SERVICE_STOP         |
                         SERVICE_QUERY_STATUS |
                         SERVICE_CHANGE_CONFIG );

    if (tablet_svc) {
      SERVICE_STATUS status;
      QueryServiceStatus (tablet_svc, &status);

      if (status.dwCurrentState != SERVICE_STOPPED) {
        running = true;

// TSFix can do this by itself
#if 0
        ControlService (tablet_svc, SERVICE_CONTROL_STOP, &status);

        ChangeServiceConfig ( tablet_svc,
                                SERVICE_NO_CHANGE,
                                  /*SERVICE_AUTO_START :*/SERVICE_DISABLED,
                                SERVICE_NO_CHANGE,
                                  nullptr, nullptr,
                                    nullptr, nullptr,
                                      nullptr, nullptr, nullptr );
#endif
      }

      CloseServiceHandle (tablet_svc);
    }

    CloseServiceHandle (svc_ctl);
  }

  return running;
}

class iImportLibrary
{
public:
               iImportLibrary (const wchar_t* wszName) { }

  virtual bool isLoaded (void) = 0;

protected:
  //std::wstring name_;
private:
};

class DLLImport : public iImportLibrary
{
public:
  DLLImport (const wchar_t* wszName) : iImportLibrary (wszName)
  {
    module_ = nullptr;

    module_ = GetModuleHandle (wszName);

    if (module_ != nullptr)
      refs_++;

    else
      module_ = LoadLibrary (wszName);

    if (module_ != nullptr)
      refs_++;
  }

  ~DLLImport (void) {
    if (isLoaded ()) {
      refs_--;
      FreeLibrary (module_);

      module_ = nullptr;

      //assert refs_ == 0
    }
  }

  bool   isLoaded (void) { return module_ != nullptr; }

  LPVOID getProcAddress (const char* szProcName) {
    return GetProcAddress (module_, szProcName);
  }

protected:
  HMODULE module_;

private:
  uint32_t refs_;
};

typedef BOOL (__stdcall *BMF_NvAPI_IsInit_pfn)(void);
typedef void (__stdcall *BMF_NvAPI_SetAppName_pfn)(const wchar_t* wszAppName);
typedef void (__stdcall *BMF_NvAPI_SetAppFriendlyName_pfn)(const wchar_t* wszAppFriendlyName);
typedef void (__stdcall *BMF_NvAPI_SetLauncher_pfn)(const wchar_t* wszLauncherName);
typedef BOOL (__stdcall *BMF_NvAPI_AddLauncherToProf_pfn)(void);


#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <pshpack1.h>
typedef HRESULT (CALLBACK *PFTASKDIALOGCALLBACK)(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ LONG_PTR lpRefData);

typedef int TASKDIALOG_FLAGS;                         // Note: _TASKDIALOG_FLAGS is an int

typedef struct _TASKDIALOG_BUTTON
{
    int     nButtonID;
    PCWSTR  pszButtonText;
} TASKDIALOG_BUTTON;

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
    TDCBF_OK_BUTTON            = 0x0001, // selected control return value IDOK
    TDCBF_YES_BUTTON           = 0x0002, // selected control return value IDYES
    TDCBF_NO_BUTTON            = 0x0004, // selected control return value IDNO
    TDCBF_CANCEL_BUTTON        = 0x0008, // selected control return value IDCANCEL
    TDCBF_RETRY_BUTTON         = 0x0010, // selected control return value IDRETRY
    TDCBF_CLOSE_BUTTON         = 0x0020  // selected control return value IDCLOSE
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;           // Note: _TASKDIALOG_COMMON_BUTTON_FLAGS is an int

typedef struct _TASKDIALOGCONFIG
{
    UINT        cbSize;
    HWND        hwndParent;                             // incorrectly named, this is the owner window, not a parent.
    HINSTANCE   hInstance;                              // used for MAKEINTRESOURCE() strings
    TASKDIALOG_FLAGS                dwFlags;            // TASKDIALOG_FLAGS (TDF_XXX) flags
    TASKDIALOG_COMMON_BUTTON_FLAGS  dwCommonButtons;    // TASKDIALOG_COMMON_BUTTON (TDCBF_XXX) flags
    PCWSTR      pszWindowTitle;                         // string or MAKEINTRESOURCE()
    union
    {
        HICON   hMainIcon;
        PCWSTR  pszMainIcon;
    } DUMMYUNIONNAME;
    PCWSTR      pszMainInstruction;
    PCWSTR      pszContent;
    UINT        cButtons;
    const TASKDIALOG_BUTTON  *pButtons;
    int         nDefaultButton;
    UINT        cRadioButtons;
    const TASKDIALOG_BUTTON  *pRadioButtons;
    int         nDefaultRadioButton;
    PCWSTR      pszVerificationText;
    PCWSTR      pszExpandedInformation;
    PCWSTR      pszExpandedControlText;
    PCWSTR      pszCollapsedControlText;
    union
    {
        HICON   hFooterIcon;
        PCWSTR  pszFooterIcon;
    } DUMMYUNIONNAME2;
    PCWSTR      pszFooter;
    PFTASKDIALOGCALLBACK pfCallback;
    LONG_PTR    lpCallbackData;
    UINT        cxWidth;                                // width of the Task Dialog's client area in DLU's. If 0, Task Dialog will calculate the ideal width.
} 
TASKDIALOGCONFIG;
#define TD_WARNING_ICON         MAKEINTRESOURCEW(-1)
#define TD_ERROR_ICON           MAKEINTRESOURCEW(-2)
#define TD_INFORMATION_ICON     MAKEINTRESOURCEW(-3)
#define TD_SHIELD_ICON          MAKEINTRESOURCEW(-4)

#include <poppack.h>

typedef HRESULT (WINAPI *TaskDialogIndirect_pfn)(
  _In_ const TASKDIALOGCONFIG *pTaskConfig,
  _Out_opt_  int              *pnButton,
  _Out_opt_  int              *pnRadioButton,
  _Out_opt_  BOOL             *pfVerificationFlagChecked
);

typedef HRESULT (WINAPI *TaskDialog_pfn)(
  _In_opt_  HWND      hwndOwner,
  _In_opt_  HINSTANCE hInstance,
  _In_opt_  PCWSTR    pszWindowTitle,
  _In_opt_  PCWSTR    pszMainInstruction,
  _In_opt_  PCWSTR    pszContent,
  TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
  _In_opt_  PCWSTR    pszIcon,
  _Out_opt_ int      *pnButton
);


TaskDialogIndirect_pfn TaskDialogIndirect = nullptr;
TaskDialog_pfn         TaskDialog         = nullptr;

typedef HINSTANCE (WINAPI *ShellExecuteW_pfn)(
  _In_opt_ HWND    hwnd,
  _In_opt_ LPCTSTR lpOperation,
  _In_     LPCTSTR lpFile,
  _In_opt_ LPCTSTR lpParameters,
  _In_opt_ LPCTSTR lpDirectory,
  _In_     INT     nShowCmd
);

ShellExecuteW_pfn ShellExecuteW = nullptr;

int
CALLBACK
WinMain ( _In_ HINSTANCE hInstance,
          _In_ HINSTANCE hPrevInstance,
          _In_ LPSTR     lpCmdLine,
          _In_ int       nCmdShow )
{
#if 0
  DLLImport D3D9 ("d3d9.dll");

  BMF_NvAPI_IsInit_pfn BMF_NvAPI_IsInit =
    (BMF_NvAPI_IsInit_pfn)D3D9.getProcAddress ("BMF_NvAPI_IsInit");
  BMF_NvAPI_SetAppName_pfn BMF_NvAPI_SetAppName =
    (BMF_NvAPI_SetAppName_pfn)D3D9.getProcAddress ("BMF_NvAPI_SetAppName");
  BMF_NvAPI_SetAppFriendlyName_pfn BMF_NvAPI_SetAppFriendlyName =
    (BMF_NvAPI_SetAppFriendlyName_pfn)D3D9.getProcAddress ("BMF_NvAPI_SetAppFriendlyName");
  BMF_NvAPI_SetLauncher_pfn BMF_NvAPI_SetLauncher =
    (BMF_NvAPI_SetLauncher_pfn)D3D9.getProcAddress ("BMF_NvAPI_SetLauncher");
  BMF_NvAPI_AddLauncherToProf_pfn BMF_NvAPI_AddLauncherToProf =
    (BMF_NvAPI_AddLauncherToProf_pfn)D3D9.getProcAddress ("BMF_NvAPI_AddLauncherToProf");
#endif

  DLLImport Shell32  (L"Shell32.dll");
  DLLImport Comctl32 (L"Comctl32.dll");

  TaskDialogIndirect =
    (TaskDialogIndirect_pfn)
    Comctl32.getProcAddress ("TaskDialogIndirect");
  TaskDialog         =
    (TaskDialog_pfn)
      Comctl32.getProcAddress ("TaskDialog");

  ShellExecuteW =
    (ShellExecuteW_pfn)
      Shell32.getProcAddress ("ShellExecuteW");

  DWORD dwDisposition = 0x00;
  HKEY  key           = nullptr;

  LSTATUS status =
    RegCreateKeyExW ( HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                        L"Image File Execution Options\\TOS.exe",
                          0, NULL, 0x00L,
                            KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  DWORD dwProcessSize   = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH];

  HANDLE hProc = GetCurrentProcess ();
  QueryFullProcessImageNameW       (hProc, 0, wszProcessName, &dwProcessSize);

  if (status == ERROR_SUCCESS && key != nullptr) {
    wchar_t wszDest [MAX_PATH];

    GetTempPathW (MAX_PATH, wszDest);
    lstrcatW     (wszDest, L"\\d3d9.dll");

    DWORD   dwLen      = MAX_PATH;
    wchar_t wszDebugger [MAX_PATH];
    status = RegGetValueW (key, nullptr, L"debugger", RRF_RT_REG_SZ, 0x0, wszDebugger, &dwLen);

    // Already installed -- uninstall and copy the DLL
    if (status == ERROR_SUCCESS) {
      //dwDisposition & REG_OPENED_EXISTING_KEY /*|| GetFileAttributes (wszDest) != INVALID_FILE_ATTRIBUTES*/) {

      // Install the DLL
      if (GetFileAttributes (L"d3d9.dll") != INVALID_FILE_ATTRIBUTES) {
        bool installed = false;
        bool updated   = false;

        if (GetFileAttributes (wszDest) == INVALID_FILE_ATTRIBUTES) {
          CopyFileW (L"d3d9.dll", wszDest, FALSE);
          installed = true;
        } else if (file_crc32 (wszDest) != file_crc32 (L"d3d9.dll")) {
          CopyFileW (L"d3d9.dll", wszDest, FALSE);
          updated = true;
        }

        if (installed || updated) {
          int               nButtonPressed = 0;
    const TASKDIALOG_BUTTON buttons []     = { { IDCONTINUE, L"See Release Notes" } };
          TASKDIALOGCONFIG  config         = {0};

          config.cbSize             = sizeof(config);
          config.hInstance          = hInstance;
          config.dwCommonButtons    = TDCBF_OK_BUTTON;
          config.pszMainIcon        = TD_INFORMATION_ICON;

          if (installed) {
            config.pszMainInstruction = L"Tales of Symphonia Fix Installed";
            config.pszContent         = L"Ensure that tsfix_enabler.exe and "
                                        L"TOS.exe are setup to run as "
                                        L"Administrator.\n\n\t"

                                        L" >> Please DO NOT delete "
                                        L"tsfix_enabler.exe without reading "
                                        L"the correct uninstall procedure first.";

            config.pButtons           = nullptr;
            config.cButtons           = 0;
          }
          else {
            config.pszMainInstruction = L"Tales of Symphonia Fix Updated";
            config.pszContent         = L"A new version of Special K (d3d9.dll) "
                                        L"has been installed, please review the "
                                        L"release notes carefully.";

            config.pButtons           = buttons;
            config.cButtons           = ARRAYSIZE(buttons);
          };

          TaskDialogIndirect (&config, &nButtonPressed, NULL, NULL);

          if (nButtonPressed == IDCONTINUE) {
            ShellExecuteW ( NULL, L"open",
                                  L"https://www.github.com/kaldaien/TSF/releases/latest",
                            nullptr, nullptr,
                              1 );
          }
        }
      } else {
        int               nButtonPressed = 0;
  const TASKDIALOG_BUTTON buttons []     = { { IDOK, L"Okay" } };
        TASKDIALOGCONFIG  config         = {0};

        config.cbSize             = sizeof(config);
        config.hInstance          = hInstance;
        config.dwCommonButtons    = TDCBF_OK_BUTTON;
        config.pszMainIcon        = TD_INFORMATION_ICON;
        config.pszMainInstruction = L"Tales of Symphonia Fix Uninstalled";
        config.pszContent         = L"You may now delete tsfix_enabler.exe";
        config.pButtons           = nullptr;
        config.cButtons           = 0;

        TaskDialogIndirect (&config, &nButtonPressed, NULL, NULL);

        // Uninstall the DLL
        DeleteFileW (wszDest);
      }

      RegDeleteTree  (key, nullptr);
      RegSetValueExW (key, L"enabler", 0, REG_SZ, (BYTE *)wszProcessName, sizeof (wchar_t) * (lstrlenW (wszProcessName) + 1));
    }

    else {
      RegSetValueExW (key, L"enabler",  0, REG_SZ, (BYTE *)wszProcessName, sizeof (wchar_t) * (lstrlenW (wszProcessName) + 1));
      RegSetValueExW (key, L"debugger", 0, REG_SZ, (BYTE *)wszProcessName, sizeof (wchar_t) * (lstrlenW (wszProcessName) + 1));

      CloseHandle (hProc);
    }

    RegFlushKey (key);
    RegCloseKey (key);
  }

  // Insufficient Privs
  else {
        int               nButtonPressed = 0;
        TASKDIALOGCONFIG  config         = {0};

        config.cbSize             = sizeof(config);
        config.dwCommonButtons    = TDCBF_OK_BUTTON;
        config.pszMainIcon        = TD_SHIELD_ICON;
        config.pszMainInstruction = L"Tales of Symphonia Fix Unusable";
        config.pszContent         = L"This operation requires Administrative Priviliges; please run the game as administrator.";
        config.pButtons           = nullptr;
        config.cButtons           = 0;

        TaskDialogIndirect (&config, &nButtonPressed, NULL, NULL);

    return 0;
  }

  if (CheckTouchServices () && GetFileAttributesW (L"tsfix.touchsrv") == INVALID_FILE_ATTRIBUTES) {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof(config);
    config.dwCommonButtons    = TDCBF_OK_BUTTON;
    config.pszMainIcon        = TD_WARNING_ICON;
    config.pszMainInstruction = L"TSFix Must Disable TabletInputService";
    config.pszContent         = L"Touch Input Services are running and will cause the game to crash periodically,"
                                L" TSFix is going to disable the service while Tales of Symphonia is running.\n\n"
                                L" >> The services will be restored at game exit <<";
    config.pButtons           = nullptr;
    config.cButtons           = 0;

    TaskDialogIndirect (&config, &nButtonPressed, NULL, NULL);

    CreateFileW   ( L"tsfix.touchsrv", GENERIC_WRITE, 0,
                      nullptr, CREATE_NEW,
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_READONLY,
                          nullptr );
  }

#if 0
  if ( BMF_NvAPI_IsInit             != nullptr &&
       BMF_NvAPI_SetAppName         != nullptr &&
       BMF_NvAPI_SetAppFriendlyName != nullptr &&
       BMF_NvAPI_SetLauncher        != nullptr &&
       BMF_NvAPI_AddLauncherToProf  != nullptr ) {
    if (BMF_NvAPI_IsInit ()) {
      BMF_NvAPI_SetAppName         (L"TOS.exe");
      BMF_NvAPI_SetAppFriendlyName (L"Tales of Symphonia");
      BMF_NvAPI_SetLauncher        (L"TOS.exe");
      BMF_NvAPI_AddLauncherToProf  ();
    }
  }
#endif

  ShellExecuteW (NULL, L"open", L"TOS.exe", nullptr, nullptr, SW_SHOWNORMAL);

  return 0;
}