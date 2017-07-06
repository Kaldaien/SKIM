// dllmain.cpp : Defines the entry point for the DLL application.
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_NON_CONFORMING_SWPRINTFS

#pragma warning (disable: 4091)

#include "stdafx.h"

#include <ShellAPI.h>

#include <CommCtrl.h>

#include <process.h>
#include <tlhelp32.h>

#include <Shlobj.h>
#pragma comment (lib, "shell32.lib")
#pragma comment (lib, "Ole32.lib")

#include <Shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

#include <cstdint>

#include <string>
#include <memory>
#include <algorithm>

#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")
#include "winbase.h"

#include <WindowsX.h>
#include <time.h>

#include <Msi.h>
#pragma comment (lib, "msi.lib")

#include <MMSystem.h>
#include <Winver.h>
//#pragma comment (lib, "Mincore_Downlevel.lib") // Windows 8     (Delay-Load)
#pragma comment (lib, "version.lib")             // Windows 2000+ (Normal Import)
#include <Richedit.h>

#define NOMINMAX

#include "system_tray.h"
#include "injection.h"

#include "Resource.h"

#include "AppInit_DLLs.h"
#include "APP_VERSION.H"
#include "ini.h"
#include "branch.h"
#include "network.h"
#include "SKIM.h"


HICON       hIconSKIM_LG;
HICON       hIconSKIM_SM;

HINSTANCE g_hInstance;

bool child = false;
wchar_t startup_dir [MAX_PATH + 1] = { };

unsigned int
__stdcall
SKIM_FeatureUnsupportedMessage (LPVOID user);

sk_product_t products [] =

{
  {
#ifndef _WIN64
    L"SpecialK32.dll",
#else
    L"SpecialK64.dll",
#endif
    L"", L"Special K", // DLL ProductName
    L"Special K",
    L"Special K (Global Injector)",
    L"SpecialK/0.8.x",
    L"8A7FSUFJ6KB2U",
    0,
#ifdef _WIN64
    SK_BOTH_BIT,
#else
    SK_32_BIT,
#endif
    L"Applies Special K's non-game-specific features to all Steam games "
    L"launched on your system.\r\n\r\n"

    L"Includes Steam achievement unlock sound, "
    L"mouse cursor management and various framerate enhancements "
    L"for DDraw/D3D8/9/11/12/GL games.\r\n\r\n",
    0
  },

  {
    L"dxgi.dll",
    L"", L"Special K", // DLL ProductName
    L"NieR: Automata™",
    L"\"FAR\" (Fix Automata Res.)",
    L"FAR",
    L"H6SDVFMHZVUR6",
    524220,
    SK_64_BIT,
    //SK_BOTH_BIT,
    L"Fixes NieR: Automata™'s wonky resolution problems.",
    0
  },

  {
    L"d3d9.dll",
    L"tbfix.dll", L"Tales of Berseria \"Fix\"", // DLL ProductName
    L"Tales of Berseria",
    L"Tales of Berseria \"Fix\"",
    L"TBF",
    L"ALPEVA3UX74LL",
    429660,
    SK_64_BIT,
    //SK_BOTH_BIT,
    L"Adds texture mod support, custom button icons, improves "
    L"Namco's framerate limiter, enhances shadow quality, "
    L"adds anti-aliasing and input remapping support.",
    0
  },

  {
    L"d3d9.dll",
    L"tbfix.dll", L"Tales of Berseria \"Fix\"", // DLL ProductName
    L"Tales of Berseria (Demo)",
    L"Tales of Berseria (Demo) \"Fix\"",
    L"TBF",
    L"ALPEVA3UX74LL",
    550350,
    SK_64_BIT,
    //SK_BOTH_BIT,
    L"Adds texture mod support, custom button icons, improves "
    L"Namco's framerate limiter, enhances shadow quality, "
    L"adds anti-aliasing and input remapping support.",
    0
  },

  {
    L"d3d9.dll",
    L"tzfix.dll", L"Tales of Zestiria \"Fix\"", // DLL ProductName
    L"Tales of Zestiria",
    L"Tales of Zestiria \"Fix\"",
    L"TZF",
    L"X3AAFX8LJWNTU",
    351970,
    SK_32_BIT,
    //SK_BOTH_BIT,
    L"Adds 60 FPS support, enhances shadow quality, fixes texture aliasing,"
    L" adds aspect ratio correction and fixes multi-channel / high sample-rate audio stability.",
    0
  },

  {
    L"d3d9.dll",
    L"tsfix.dll", L"Tales of Symphonia \"Fix\"", // DLL ProductName
    L"Tales of Symphonia",
    L"Tales of Symphonia \"Fix\"",
    L"TSF",
    L"WNZ6CYRYN3NVJ",
    372360,
    SK_32_BIT,
    //SK_BOTH_BIT,
    L"Adds MSAA, fixes Namco's framerate limiter, supports 4K textures, "
    L"fixes input-related problems.",
    0
  },

  {
    L"dxgi.dll",
    L"", L"",
#ifdef _WIN64
    L"Fallout 4",
    L"Fallout 4 \"Works\"",
    L"FO4W",
#else
    L"",
    L"",
    L"",
#endif
    L"", // N/A
    377160,
    SK_64_BIT,
    //SK_BOTH_BIT,
#ifdef _WIN64
    L"Improves framepacing.\r\n\r\n"
    L"  (Use the Global Injector; Plug-In is built-in)",
#else
    L"",
#endif
    0
  },

  {
    L"OpenGL32.dll",
    L"PrettyPrinny.dll", L"Pretty Prinny", // DLL ProductName
    L"Disgaea PC",
    L"Pretty Prinny",
    L"PrettyPrinny",
    L"UYL32Y8H4K5H2",
    405900,
    SK_32_BIT,
    //SK_BOTH_BIT,
    L"Improves framepacing, optimizes post-processing, removes the 720p "
    L"resolution lock, adds borderless window and MSAA, supports custom "
    L"button icons.",
    0
  },

  {
    L"dxgi.dll",
    L"", L"Special K", // DLL ProductName
#ifdef _WIN64
    L"Dark Souls III",
    L"Souls \"Unsqueezed\"",
    L"SoulsUnsqueezed",
#else
    L"",
    L"",
    L"",
#endif
    L"L9FJSV8WXMWRU",
    374320,
    SK_64_BIT,
#ifdef _WIN64
    L"Adds support for non-16:9 aspect ratios, texture memory optimizations "
    L"and multi-monitor rendering.\r\n",
#else
    L"",
#endif
    0
  },

  {
    L"dxgi.dll",
    L"UnX.dll", L"Untitled Project X", // DLL ProductName
    L"Final Fantasy X / X-2 HD Remaster",
    L"\"Untitled\" Project X",
    L"UnX",
    L"6TDLFVRGXLU92",
    359870,
    SK_32_BIT,
    //SK_BOTH_BIT,
    L"Adds dual-audio support, texture modding cutscene skipping in FFX, "
    L"cursor management, Intel GPU bypass, fullscreen exclusive mode, "
    L"maps all PC-specific extra features to gamepad.\n",
    0
  }
};

const wchar_t*
SKIM_SteamUtil_GetInstallDir (void)
{
         DWORD   len         = MAX_PATH;
  static wchar_t wszSteamPath [MAX_PATH];

  LSTATUS status =
    RegGetValueW ( HKEY_CURRENT_USER,
                     L"SOFTWARE\\Valve\\Steam\\",
                       L"SteamPath",
                         RRF_RT_REG_SZ,
                           NULL,
                             wszSteamPath,
                               (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return wszSteamPath;
  else
    return nullptr;
}

bool
SKIM_Util_CreateDirectories ( const wchar_t* wszPath )
{
  wchar_t* wszSubDir = _wcsdup (wszPath), *iter;

  wchar_t* wszLastSlash     = wcsrchr (wszSubDir, L'/');
  wchar_t* wszLastBackslash = wcsrchr (wszSubDir, L'\\');

  if (wszLastSlash > wszLastBackslash)
    *wszLastSlash = L'\0';
  else if (wszLastBackslash != nullptr)
    *wszLastBackslash = L'\0';
  else {
    free (wszSubDir);
    return false;
  }

  for (iter = wszSubDir; *iter != L'\0'; ++iter) {
    if (*iter == L'\\' || *iter == L'/') {
      *iter = L'\0';

      if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
        CreateDirectoryW (wszSubDir, nullptr);

      *iter = L'\\';
    }

    // The final subdirectory (FULL PATH)
    if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
      CreateDirectoryW (wszSubDir, nullptr);
  }

  free (wszSubDir);

  return true;
}

void
SKIM_StripTrailingSlashes (wchar_t* wszInOut)
{
  struct test_slashes
  {
    bool operator () (wchar_t a, wchar_t b) const
    {
      auto IsSlash = [](wchar_t a) -> bool {
        return (a == L'\\' || a == L'/');
      };

      return IsSlash (a) && IsSlash (b);
    }
  };
  
  std::wstring wstr (wszInOut);
  
  wstr.erase ( std::unique ( wstr.begin (),
                             wstr.end   (), test_slashes () ),
                 wstr.end () );

  wcscpy (wszInOut, wstr.c_str ());
}

const wchar_t*
SKIM_FindInstallPath (uint32_t appid)
{
  // DarkSouls 3's install path has additional baggage ;)
  bool ds3 = (appid == 374320);

  wchar_t wszAppend [MAX_PATH] = { };

  if (ds3)
    lstrcatW (wszAppend, L"\\Game");


  static wchar_t wszGamePath [MAX_PATH] = { };

  // Special Case: AppID 0 = Special K
  if (appid == 0)
  {
    uint32_t dwLen = MAX_PATH;

    SKIM_Util_GetDocumentsDir (wszGamePath, &dwLen);

    PathAppend (wszGamePath, L"My Mods\\SpecialK\\");

    return wszGamePath;
  }

  typedef char* steam_library_t [MAX_PATH];

  static bool scanned_libs = false;

#define MAX_STEAM_LIBRARIES 16
  static int             steam_libs = 0;
  static steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { };

  static const wchar_t* wszSteamPath;

  if (! scanned_libs) {
    wszSteamPath =
      SKIM_SteamUtil_GetInstallDir ();

      if (wszSteamPath != nullptr) {
        wchar_t wszLibraryFolders [MAX_PATH] = { };

        lstrcpyW   (wszLibraryFolders, wszSteamPath);
        PathAppend (wszLibraryFolders, L"steamapps\\libraryfolders.vdf");

      if (GetFileAttributesW (wszLibraryFolders) != INVALID_FILE_ATTRIBUTES) {
        HANDLE hLibFolders =
          CreateFileW ( wszLibraryFolders,
                          GENERIC_READ,
                            FILE_SHARE_READ,
                              nullptr,
                                OPEN_EXISTING,
                                  GetFileAttributesW (wszLibraryFolders),
                                    nullptr );

        if (hLibFolders != INVALID_HANDLE_VALUE) {
          DWORD  dwSize,
                 dwSizeHigh,
                 dwRead;

          dwSize = GetFileSize (hLibFolders, &dwSizeHigh);

          void* data =
            new uint8_t [dwSize];

          if (data == nullptr) {
            CloseHandle (hLibFolders);
            return nullptr;
          }

          dwRead = dwSize;

          if (ReadFile (hLibFolders, data, dwSize, &dwRead, nullptr)) {
            for (DWORD i = 0; i < dwSize; i++) {

              if (((const char *)data) [i] == '"' && i < dwSize - 3) {

                if (((const char *)data) [i + 2] == '"')
                  i += 2;
                else if (((const char *)data) [i + 3] == '"')
                  i += 3;
                else
                  continue;

                char* lib_start = nullptr;

                for (DWORD j = i; j < dwSize; j++,i++) {
                  if (((char *)data) [j] == '"' && lib_start == nullptr && j < dwSize - 1) {
                    lib_start = &((char *)data) [j+1];
                  }
                  else if (((char *)data) [j] == '"') {
                    ((char *)data) [j] = '\0';
                    lstrcpyA ((char *)steam_lib_paths [steam_libs++], lib_start);
                    lib_start = nullptr;
                  }
                }
              }
            }
          }

          delete [] data;

          CloseHandle (hLibFolders);
        }
      }
    }

    scanned_libs = true;
  }

  // Search custom library paths first
  if (steam_libs != 0) {
    for (int i = 0; i < steam_libs; i++) {
      char szManifest [MAX_PATH] = { };

      sprintf ( szManifest,
                  "%s\\steamapps\\appmanifest_%d.acf",
                    (char *)steam_lib_paths [i],
                      appid );

      if (GetFileAttributesA (szManifest) != INVALID_FILE_ATTRIBUTES)
      {
        HANDLE hManifest =
          CreateFileA ( szManifest,
                        GENERIC_READ,
                          FILE_SHARE_READ,
                            nullptr,
                              OPEN_EXISTING,
                                GetFileAttributesA (szManifest),
                                  nullptr );

        if (hManifest != INVALID_HANDLE_VALUE) {
          DWORD  dwSize,
                 dwSizeHigh,
                 dwRead;

          dwSize = GetFileSize (hManifest, &dwSizeHigh);

          char* szManifestData =
            new char [dwSize + 1];

          szManifestData [dwSize] = '\0';

          ReadFile ( hManifest,
                       szManifestData,
                         dwSize,
                           &dwRead,
                             nullptr );

          CloseHandle (hManifest);

          if (! dwRead) {
            delete [] szManifestData;
            continue;
          }

          char* szInstallDir =
            StrStrIA (szManifestData, "\"installdir\"");

          char szGamePath [MAX_PATH] = { };

          if (szInstallDir != nullptr) {
            // Make sure everything is lowercase
            strncpy (szInstallDir, "\"installdir\"", strlen ("\"installdir\""));

            sscanf ( szInstallDir,
                       "\"installdir\" \"%259[^\"]\"",
                         szGamePath );
          }

          swprintf ( wszGamePath,
                       L"%hs\\steamapps\\common\\%hs%s",
                         (char *)steam_lib_paths [i],
                           szGamePath, wszAppend );

          SKIM_StripTrailingSlashes (wszGamePath);

          if (GetFileAttributesW (wszGamePath) & FILE_ATTRIBUTE_DIRECTORY)
          {
            delete [] szManifestData;
            return wszGamePath;
          }

          delete [] szManifestData;
        }
      }
    }
  }

  char szManifest [MAX_PATH] = { };

  sprintf ( szManifest,
              "%ws\\steamapps\\appmanifest_%d.acf",
                wszSteamPath,
                  appid );

  if (GetFileAttributesA (szManifest) != INVALID_FILE_ATTRIBUTES) {
    HANDLE hManifest =
      CreateFileA ( szManifest,
                    GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr,
                          OPEN_EXISTING,
                            GetFileAttributesA (szManifest),
                              nullptr );

    if (hManifest != INVALID_HANDLE_VALUE)
    {
      DWORD  dwSize,
             dwSizeHigh,
             dwRead;

      dwSize = GetFileSize (hManifest, &dwSizeHigh);

      char* szManifestData =
        new char [dwSize + 1];

      szManifestData [dwSize] = '\0';

      if (szManifestData == nullptr) {
        CloseHandle (hManifest);
        return nullptr;
      }

      ReadFile ( hManifest,
                   szManifestData,
                     dwSize,
                       &dwRead,
                         nullptr );

      CloseHandle (hManifest);

      if (! dwRead) {
        delete [] szManifestData;
        return nullptr;
      }

      char* szInstallDir =
        StrStrIA (szManifestData, "\"installdir\"");

      char szGamePath [MAX_PATH] = { };

      if (szInstallDir != nullptr) {
        // Make sure everything is lowercase
        strncpy (szInstallDir, "\"installdir\"", strlen ("\"installdir\""));

        sscanf ( szInstallDir,
                   "\"installdir\" \"%259[^\"]\"",
                     szGamePath );
      }

      swprintf ( wszGamePath,
                   L"%ws\\steamapps\\common\\%hs%s",
                     wszSteamPath,
                       szGamePath, wszAppend );

      SKIM_StripTrailingSlashes (wszGamePath);

      if (GetFileAttributesW (wszGamePath) & FILE_ATTRIBUTE_DIRECTORY)
      {
        delete [] szManifestData;
        return wszGamePath;
      }

      delete [] szManifestData;
    }
  }

  return L"";
}

#include <Winver.h>
//#pragma comment (lib, "Mincore_Downlevel.lib") // Windows 8     (Delay-Load)
#pragma comment (lib, "version.lib")             // Windows 2000+ (Normal Import)

bool
__stdcall
SKIM_IsDLLFromProduct (const wchar_t* wszName, const wchar_t* wszProductName)
{
  UINT     cbTranslatedBytes = 0,
           cbProductBytes    = 0;

  uint8_t  cbData      [4096] = { };
  wchar_t  wszPropName [64]   = { };

  wchar_t* wszProduct = nullptr; // Will point somewhere in cbData

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate = nullptr;

  wchar_t wszFullyQualifiedName [MAX_PATH * 2] = { };

  lstrcatW (wszFullyQualifiedName, wszName);

  if (GetFileAttributes (wszFullyQualifiedName) == INVALID_FILE_ATTRIBUTES)
    return false;

  GetFileVersionInfoEx ( FILE_VER_GET_NEUTRAL | FILE_VER_GET_PREFETCHED,
                           wszFullyQualifiedName,
                             0x00,
                               4096,
                                 cbData );

  if (VerQueryValue ( cbData,
                      TEXT ("\\VarFileInfo\\Translation"),
                        (LPVOID *)&lpTranslate,
                                  &cbTranslatedBytes ) && cbTranslatedBytes)
  {
    wsprintfW ( wszPropName,
                  L"\\StringFileInfo\\%04x%04x\\ProductName",
                    lpTranslate [0].wLanguage,
                      lpTranslate [0].wCodePage );

    VerQueryValue ( cbData,
                      wszPropName,
                        (LPVOID *)&wszProduct,
                                  &cbProductBytes );

    return (cbProductBytes && (! wcsicmp (wszProduct, wszProductName)));
  }

  return false;
}

class iImportLibrary
{
public:
               iImportLibrary (const wchar_t* wszName) {
    UNREFERENCED_PARAMETER (wszName);
  }

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

  virtual ~DLLImport (void) {
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

bool
SKIM_Util_IsAdmin (void)
{
  bool   bRet   = false;
  HANDLE hToken = 0;

  if (OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken)) {
    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof (TOKEN_ELEVATION);

    if (GetTokenInformation (hToken, TokenElevation, &Elevation, sizeof (Elevation), &cbSize)) {
      bRet = Elevation.TokenIsElevated != 0;
    }
  }

  if (hToken)
    CloseHandle (hToken);

  return bRet;
}

bool
SKIM_Util_IsProcessRunning (const wchar_t* wszProcName)
{
  HANDLE         hProcSnap;
  PROCESSENTRY32 pe32;

  hProcSnap =
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize = sizeof PROCESSENTRY32;

  if (! Process32First (hProcSnap, &pe32)) {
    CloseHandle (hProcSnap);
    return false;
  }

  do {
    if (! lstrcmpiW (wszProcName, pe32.szExeFile)) {
      CloseHandle (hProcSnap);
      return true;
    }
  } while (Process32Next (hProcSnap, &pe32));

  CloseHandle (hProcSnap);

  return false;
}

bool
SKIM_Util_GetDocumentsDir (wchar_t* buf, uint32_t* pdwLen)
{
  HANDLE hToken;

  if (! OpenProcessToken (GetCurrentProcess (), TOKEN_READ, &hToken))
    return false;

  wchar_t* str;

  if ( SUCCEEDED (
         SHGetKnownFolderPath (
           FOLDERID_Documents, 0, hToken, &str
         )
       )
     )
  {
    if (buf != nullptr && pdwLen != nullptr && *pdwLen > 0) {
      wcsncpy (buf, str, *pdwLen);
    }

    CoTaskMemFree (str);

    return true;
  }

  return false;
}



#include <MMSystem.h>

void
SKIM_Util_MoveFileNoFail ( const wchar_t* wszOld, const wchar_t* wszNew )
{
  if (! MoveFileExW ( wszOld,
                        wszNew,
                          MOVEFILE_REPLACE_EXISTING ) )
  {
    wchar_t wszTemp [MAX_PATH] = { };
    GetTempFileNameW (nullptr, L"SKI", timeGetTime (), wszTemp);

    MoveFileExW ( wszNew, wszTemp, 0x00 );
  }
}






bool
SKIM_Depends_TestKB2533623 (SK_ARCHITECTURE arch)
{
  return GetProcAddress (GetModuleHandle (L"kernel32.dll"), "AddDllDirectory") != nullptr;
}

#include <Msi.h>
#pragma comment (lib, "msi.lib")

bool
SKIM_Depends_TestVisualCRuntime (SK_ARCHITECTURE arch)
{
  if (arch == SK_64_BIT)
  {
    const wchar_t* vcredist2015_upgradecode_x64 =
      L"{36F68A90-239C-34DF-B58C-64B30153CE35}";

    wchar_t        vcredist2015_any_productcode_x64 [39] =
        { L"\0" };

    if ( MsiEnumRelatedProductsW (
           vcredist2015_upgradecode_x64,
             0x00,
               0,
                 vcredist2015_any_productcode_x64 ) == ERROR_SUCCESS ) {
      return lstrlenW (vcredist2015_any_productcode_x64) &&
              MsiQueryProductState (vcredist2015_any_productcode_x64) ==
                INSTALLSTATE_DEFAULT;
    }

    return false;
  }

  else if (arch == SK_32_BIT)
  {
    const wchar_t* vcredist2015_upgradecode =
      L"{65E5BD06-6392-3027-8C26-853107D3CF1A}";

    wchar_t vcredist2015_any_productcode [39] =
        { L"\0" };

    if ( MsiEnumRelatedProductsW (
           vcredist2015_upgradecode,
             0x00,
               0,
                 vcredist2015_any_productcode ) == ERROR_SUCCESS ) {
      return lstrlenW (vcredist2015_any_productcode) &&
              MsiQueryProductState (vcredist2015_any_productcode) ==
                INSTALLSTATE_DEFAULT;
    }

    return false;
  }

  else {/* if (arch == SK_BOTH_BIT) {*/
    return SKIM_Depends_TestVisualCRuntime (SK_32_BIT) &&
           SKIM_Depends_TestVisualCRuntime (SK_64_BIT);
  }
}

void
SKIM_DeleteConfigFiles (sk_product_t* product)
{
  if (product->uiSteamAppID == 0)
    return;

  wchar_t wszWrapper [MAX_PATH];
  wchar_t wszPlugIn  [MAX_PATH];

  wchar_t wszInstallPath [MAX_PATH];
  wcscpy (wszInstallPath,
    SKIM_FindInstallPath (product->uiSteamAppID)
  );
  wcscat (wszInstallPath, L"\\");

  wcscpy (wszWrapper, product->wszWrapper);
  wcscpy (wszPlugIn,  product->wszPlugIn);

  bool has_plugin = wcslen (wszPlugIn) > 0;

  *wcsrchr (wszWrapper, L'.') = L'\0';

  wchar_t wszWrapperINI [MAX_PATH] = { };
  wcscat (wszWrapperINI, wszInstallPath);
  wcscat (wszWrapperINI, wszWrapper);
  wcscat (wszWrapperINI, L".ini");

  DeleteFileW (wszWrapperINI);

  wchar_t wszPlugInINI [MAX_PATH] = { };

  if (has_plugin) {
    *wcsrchr (wszPlugIn,  L'.') = L'\0';

    wcscat (wszPlugInINI, wszInstallPath);
    wcscat (wszPlugInINI, wszPlugIn);
    wcscat (wszPlugInINI, L".ini");

    DeleteFileW (wszPlugInINI);
    *wszPlugInINI  = L'\0';

    wcscat (wszPlugInINI, wszInstallPath);
    wcscat (wszPlugInINI, L"default_");
    wcscat (wszPlugInINI, wszPlugIn);
    wcscat (wszPlugInINI, L".ini");

    DeleteFileW (wszPlugInINI);

    *wszPlugInINI  = L'\0';

    wcscat (wszPlugInINI, wszInstallPath);
    wcscat (wszPlugInINI, wszPlugIn);
    wcscat (wszPlugInINI, L".pdb");

    DeleteFileW (wszPlugInINI);
  }

  *wszWrapperINI = L'\0';

  wcscat (wszWrapperINI, wszInstallPath);
  wcscat (wszWrapperINI, L"default_");
  wcscat (wszWrapperINI, wszWrapper);
  wcscat (wszWrapperINI, L".ini");

  DeleteFileW (wszWrapperINI);

  *wszWrapperINI = L'\0';

  wcscat (wszWrapperINI, wszInstallPath);
  wcscat (wszWrapperINI, wszWrapper);
  wcscat (wszWrapperINI, L".pdb");

  DeleteFileW (wszWrapperINI);

  *wszWrapperINI = L'\0';
  *wszPlugInINI  = L'\0';

  wcscat (wszPlugInINI, wszInstallPath);
  wcscat (wszPlugInINI, L"SpecialK32.pdb");

  wcscat (wszWrapperINI, wszInstallPath);
  wcscat (wszWrapperINI, L"SpecialK64.pdb");

  DeleteFileW (wszWrapperINI);
  DeleteFileW (wszPlugInINI);
}

unsigned int
WINAPI
SKIM_UninstallProduct (LPVOID user)
{
  sk_product_t* product = (sk_product_t *)user;

  extern HWND hWndMainDlg;
  HWND parent_dlg = hWndMainDlg;

  ShowWindow (parent_dlg, SW_HIDE);

  if (product->uiSteamAppID == 0) {
    SKIM_DisableGlobalInjector ();
  }

  SetCurrentDirectory (SKIM_FindInstallPath (product->uiSteamAppID));

  SKIM_DeleteConfigFiles (product);

  DeleteFileW (L"Version\\installed.ini");
  DeleteFileW (L"Version\\repository.ini");

  DeleteFileW (product->wszWrapper);
  DeleteFileW (product->wszPlugIn);

  wchar_t wszUninstall [256];
  wsprintf ( wszUninstall, L"%s has been successfully uninstalled.",
               product->wszProjectName );


  MessageBox ( 0,
                 wszUninstall,
                   L"Uninstall Success",
                     MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );

  ShowWindow          (parent_dlg, SW_SHOW);
  SetForegroundWindow (parent_dlg);
  SendMessage         (parent_dlg, WM_INITDIALOG, 0x00, 0x00);

  if (child)
  {
    TerminateProcess (GetCurrentProcess (), 0x00);
    ExitProcess      (                      0x00);
  }

  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
  SKIM_OnProductSelect ();
  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
  SKIM_OnBranchSelect ();

  CloseHandle (GetCurrentThread ());

  return 0;
}

#include <time.h>

DWORD
SKIM_CountProductBranches (sk_product_t *pProduct)
{
  SKIM_BranchManager::singleton ()->setProduct (pProduct->uiSteamAppID);

  return SKIM_BranchManager::singleton ()->getNumberOfBranches ();
}

SKIM_BranchManager::Branch*
SKIM_GetProductBranchByIdx (sk_product_t *pProduct, int idx)
{
  SKIM_BranchManager::singleton ()->setProduct (pProduct->uiSteamAppID);

  return SKIM_BranchManager::singleton ()->getBranchByIndex (idx);
}



unsigned int
__stdcall
SKIM_MigrateProduct (LPVOID user)//sk_product_t* pProduct)
{
  sk_product_t& product = *(sk_product_t *)user;

  // Disable installation of FO4W
  if ( product.uiSteamAppID == 377160 ) {
    CloseHandle (GetCurrentThread ());
    return 0;
  }

  if (! SKIM_Depends_TestKB2533623 (SK_BOTH_BIT) )
  {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager (Fatal Error)";
    config.dwCommonButtons    = 0;
    config.pszMainInstruction = L"Missing KB2533623 Update for Windows 7";
    config.pButtons           = nullptr;
    config.cButtons           = 0;

    config.pfCallback         = 
      []( _In_ HWND     hWnd,
          _In_ UINT     uNotification,
          _In_ WPARAM   wParam,
          _In_ LPARAM   lParam,
          _In_ LONG_PTR dwRefData ) ->
      HRESULT
      {
        UNREFERENCED_PARAMETER (dwRefData);
        UNREFERENCED_PARAMETER (wParam);

        if (uNotification == TDN_HYPERLINK_CLICKED)
        {
          ShellExecuteW ( hWnd,
                            L"OPEN",
                              (wchar_t *)lParam,
                                nullptr, nullptr,
                                  SW_SHOWMAXIMIZED );

          TerminateProcess (GetCurrentProcess (), 0x00);
          ExitProcess      (                      0x00);
        }

        return S_FALSE;
      };

    config.dwFlags           |= TDF_ENABLE_HYPERLINKS;

    config.pszMainIcon        = TD_ERROR_ICON;

#ifndef _WIN64
    config.pszContent       = L"Please grab the x86"
                              L" version from <a href=\""
                              L"https://www.microsoft.com/en-us/"
                              L"download/details.aspx?id=26767\">here</a>"
                              L" to continue.";
#else
    config.pszContent       = L"Please grab the x64"
                              L" version from <a href=\""
                              L"https://www.microsoft.com/en-us/"
                              L"download/details.aspx?id=267647\">here</a>"
                              L" to continue.";
#endif

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
    return 0;
  }

  if (! SKIM_Depends_TestVisualCRuntime (SK_BOTH_BIT) )
  {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager (Fatal Error)";
    config.dwCommonButtons    = 0;
    config.pszMainInstruction = L"Missing Visual C++ 2015 Runtime";
    config.pButtons           = nullptr;
    config.cButtons           = 0;

    config.pfCallback         = 
      []( _In_ HWND     hWnd,
          _In_ UINT     uNotification,
          _In_ WPARAM   wParam,
          _In_ LPARAM   lParam,
          _In_ LONG_PTR dwRefData ) ->
      HRESULT
      {
        UNREFERENCED_PARAMETER (dwRefData);
        UNREFERENCED_PARAMETER (wParam);

        if (uNotification == TDN_HYPERLINK_CLICKED)
        {
          ShellExecuteW ( hWnd,
                            L"OPEN",
                              (wchar_t *)lParam,
                                nullptr, nullptr,
                                  SW_SHOWMAXIMIZED );

          TerminateProcess (GetCurrentProcess (), 0x00);
          ExitProcess      (                      0x00);
        }

        return S_FALSE;
      };

    config.dwFlags           |= TDF_ENABLE_HYPERLINKS;

    config.pszMainIcon        = TD_ERROR_ICON;

#if 0
    if (product.architecture == SK_32_BIT) {
      config.pszContent       = L"Please grab the x86"
                                L" version from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746571"
                                L"\">here</a>"
                                L" to continue.";
    } else if (product.architecture == SK_64_BIT) {
      config.pszContent       = L"Please grab the x64"
                                L" version from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746572"
                                L"\">here</a>"
                                L" to continue.";
    } else {
#endif
      config.pszContent       = L"Please grab _BOTH_, the x86 and x64 "
                                L"versions from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746571"
                                L"\">here (x86)</a> and <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746572"
                                L"\">here (x64)</a> to continue.";
#if 0
    }
#endif

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
    return 0;
  }

  wchar_t wszInstallPath [MAX_PATH] = { };

  wcscpy ( wszInstallPath,
            SKIM_FindInstallPath (product.uiSteamAppID) );

  wchar_t wszAppID      [MAX_PATH * 2] = { };
  wchar_t wszExecutable [MAX_PATH    ] = { };

  GetModuleFileName (GetModuleHandle (nullptr), wszExecutable, MAX_PATH);

  swprintf (wszAppID, L"\"%ws\" %lu", wszExecutable, product.uiSteamAppID);

  if ( product.uiSteamAppID != 0 )
  {
    if ( GetFileAttributes (L"steam_api.dll")   == INVALID_FILE_ATTRIBUTES &&
         GetFileAttributes (L"steam_api64.dll") == INVALID_FILE_ATTRIBUTES )
    {
      wchar_t wszDeployedSteamAPI   [MAX_PATH] = { };
      wchar_t wszDeployedSteamAPI64 [MAX_PATH] = { };

      lstrcatW   (wszDeployedSteamAPI, wszInstallPath);
      PathAppend (wszDeployedSteamAPI, L"steam_api.dll");

      lstrcatW   (wszDeployedSteamAPI64, wszInstallPath);
      PathAppend (wszDeployedSteamAPI64, L"steam_api64.dll");

      extern HWND hWndMainDlg;

      if ( GetFileAttributes (wszDeployedSteamAPI)   != INVALID_FILE_ATTRIBUTES ||
           GetFileAttributes (wszDeployedSteamAPI64) != INVALID_FILE_ATTRIBUTES )
      {
        STARTUPINFO         sinfo = { 0 };
        PROCESS_INFORMATION pinfo = { 0 };

        sinfo.cb          = sizeof STARTUPINFO;
        sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        sinfo.wShowWindow = SW_HIDE;

        wchar_t wszRepoINI    [MAX_PATH] = { };

        wsprintf ( wszRepoINI,
                     L"%s\\Version\\repository.ini",
                       wszInstallPath );

        SKIM_DeleteConfigFiles (&product);

        DeleteFileW (wszRepoINI);

        CreateProcess ( wszExecutable,
                          wszAppID,
                            nullptr, nullptr,
                              FALSE, 0x00,
                                nullptr, wszInstallPath,
                                  &sinfo, &pinfo );

        if (hWndMainDlg != 0)
        {
          ShowWindow (hWndMainDlg, SW_HIDE);
        }

        if (WaitForSingleObject ( pinfo.hProcess, INFINITE ) == WAIT_OBJECT_0)
        {
          CloseHandle (pinfo.hThread);
          CloseHandle (pinfo.hProcess);
        }

        if (! child)
        {
          ShowWindow          (hWndMainDlg, SW_SHOW);
          SetForegroundWindow (hWndMainDlg);
          SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
        }

        else
        {
          SendMessage         (hWndMainDlg, WM_CLOSE, 0x00, 0x00);
          SendMessage         (hWndMainDlg, WM_QUIT,  0x00, 0x00);
        }

        return 1;
      }

      else
      {
        wchar_t wszErrorMsg [512] = { };

        _swprintf ( wszErrorMsg,
                      L"Unable to locate a valid install path (steam_api{64}.dll is missing)!\r\n\r\n"
                      L"Game is supposed to be installed to '%s', but is not.",
                        SKIM_FindInstallPath (product.uiSteamAppID) );

        MessageBox ( 0,
                      wszErrorMsg,
                        L"Special K Installer Fatal Error",
                          MB_ICONHAND | MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
      }
    }
  }

  SetCurrentDirectory (
    SKIM_FindInstallPath (product.uiSteamAppID)
  );

  extern HWND hWndMainDlg;
  ShowWindow (hWndMainDlg, SW_HIDE);

  if (SKIM_FetchInstallerDLL (product))
  {
    wchar_t wszInstallerDLL [MAX_PATH] = { };
    swprintf ( wszInstallerDLL,
                L"%s\\%s",
                  wszInstallPath, product.wszWrapper );

    HMODULE hModInstaller =
      LoadLibrary (wszInstallerDLL);

    if (hModInstaller != nullptr)
    {
      wchar_t wszRepoINI      [MAX_PATH] = { };

      wsprintf ( wszRepoINI,
                   L"%s\\Version\\repository.ini",
                     wszInstallPath );

      DeleteFileW (wszRepoINI);

      typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)(const wchar_t* wszProduct);
      typedef bool    (__stdcall *SK_FetchVersionInfo_pfn)(const wchar_t* wszProduct);

      SK_UpdateSoftware_pfn SK_UpdateSoftware =
        (SK_UpdateSoftware_pfn)
          GetProcAddress ( hModInstaller,
                            "SK_UpdateSoftware" );

      SK_FetchVersionInfo_pfn SK_FetchVersionInfo =
        (SK_FetchVersionInfo_pfn)
          GetProcAddress ( hModInstaller,
                          "SK_FetchVersionInfo" );

      if ( SK_FetchVersionInfo != nullptr &&
           SK_UpdateSoftware   != nullptr )
      {
        SK_FetchVersionInfo (product.wszRepoName);
        SK_UpdateSoftware   (product.wszRepoName);
      }
    }

    if (! child)
    {
      ShowWindow          (hWndMainDlg, SW_SHOW);
      SetForegroundWindow (hWndMainDlg);
      SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
    }

    else
    {
      SendMessage         (hWndMainDlg, WM_CLOSE, 0x00, 0x00);
      SendMessage         (hWndMainDlg, WM_QUIT,  0x00, 0x00);
    }

    SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
    SKIM_OnProductSelect          ();
    SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
  }

  else
  {
    MessageBoxW (NULL, L"Something went wrong attempting to download Installer DLL.", L"Install Failed", MB_OK | MB_ICONERROR);
  }

  return 0;
}


unsigned int
__stdcall
SKIM_InstallProduct (LPVOID user)//sk_product_t* pProduct)
{
  static int tries = 0;

  sk_product_t& product = *(sk_product_t *)user;

  // Disable installation of FO4W
  if ( product.uiSteamAppID == 377160 ) {
    CloseHandle (GetCurrentThread ());
    return 0;
  }

  if (! SKIM_Depends_TestKB2533623 (SK_BOTH_BIT) )
  {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager (Fatal Error)";
    config.dwCommonButtons    = 0;
    config.pszMainInstruction = L"Missing KB2533623 Update for Windows 7";
    config.pButtons           = nullptr;
    config.cButtons           = 0;

    config.pfCallback         = 
      []( _In_ HWND     hWnd,
          _In_ UINT     uNotification,
          _In_ WPARAM   wParam,
          _In_ LPARAM   lParam,
          _In_ LONG_PTR dwRefData ) ->
      HRESULT
      {
        UNREFERENCED_PARAMETER (dwRefData);
        UNREFERENCED_PARAMETER (wParam);

        if (uNotification == TDN_HYPERLINK_CLICKED) {
          ShellExecuteW ( hWnd,
                            L"OPEN",
                              (wchar_t *)lParam,
                                nullptr, nullptr,
                                  SW_SHOWMAXIMIZED );

          TerminateProcess (GetCurrentProcess (), 0x00);
          ExitProcess      (                      0x00);
        }

        return S_FALSE;
      };

    config.dwFlags           |= TDF_ENABLE_HYPERLINKS;

    config.pszMainIcon        = TD_ERROR_ICON;

#ifndef _WIN64
    config.pszContent       = L"Please grab the x86"
                              L" version from <a href=\""
                              L"https://www.microsoft.com/en-us/"
                              L"download/details.aspx?id=26767\">here</a>"
                              L" to continue.";
#else
    config.pszContent       = L"Please grab the x64"
                              L" version from <a href=\""
                              L"https://www.microsoft.com/en-us/"
                              L"download/details.aspx?id=267647\">here</a>"
                              L" to continue.";
#endif

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
    return 0;
  }

  SK_ARCHITECTURE arch = SK_64_BIT;

  if (product.architecture == SK_32_BIT)
    arch = SK_BOTH_BIT;

  if (! SKIM_Depends_TestVisualCRuntime (arch) )
  {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager (Fatal Error)";
    config.dwCommonButtons    = 0;
    config.pszMainInstruction = L"Missing Visual C++ 2015 Runtime";
    config.pButtons           = nullptr;
    config.cButtons           = 0;

    config.pfCallback         = 
      []( _In_ HWND     hWnd,
          _In_ UINT     uNotification,
          _In_ WPARAM   wParam,
          _In_ LPARAM   lParam,
          _In_ LONG_PTR dwRefData ) ->
      HRESULT
      {
        UNREFERENCED_PARAMETER (dwRefData);
        UNREFERENCED_PARAMETER (wParam);

        if (uNotification == TDN_HYPERLINK_CLICKED)
        {
          ShellExecuteW ( hWnd,
                            L"OPEN",
                              (wchar_t *)lParam,
                                nullptr, nullptr,
                                  SW_SHOWMAXIMIZED );

          TerminateProcess (GetCurrentProcess (), 0x00);
          ExitProcess      (                      0x00);
        }

        return S_FALSE;
      };

    config.dwFlags           |= TDF_ENABLE_HYPERLINKS;

    config.pszMainIcon        = TD_ERROR_ICON;

    if (arch == SK_BOTH_BIT)
    {
      config.pszContent       = L"Please grab _BOTH_, the x86 and x64 "
                                L"versions from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746571"
                                L"\">here (x86)</a> and <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746572"
                                L"\">here (x64)</a> to continue.";
    }

    else if (arch == SK_32_BIT)
    {
      config.pszContent       = L"Please grab the x86"
                                L" version from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746571"
                                L"\">here</a>"
                                L" to continue.";
    }

    else if (arch == SK_64_BIT)
    {
      config.pszContent       = L"Please grab the x64"
                                L" version from <a href=\""
                                L"https://go.microsoft.com/fwlink/?LinkId=746572"
                                L"\">here</a>"
                                L" to continue.";
    }

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
    return 0;
  }

  wchar_t wszInstallPath [MAX_PATH] = { };

  wcscpy ( wszInstallPath,
            SKIM_FindInstallPath (product.uiSteamAppID) );

  wchar_t wszAppID      [MAX_PATH * 2] = { };
  wchar_t wszExecutable [MAX_PATH    ] = { };

  GetModuleFileName (GetModuleHandle (nullptr), wszExecutable, MAX_PATH);

  swprintf (wszAppID, L"\"%ws\" %lu", wszExecutable, product.uiSteamAppID);

  if ( product.uiSteamAppID != 0 )
  {
    if ( GetFileAttributes (L"steam_api.dll")   == INVALID_FILE_ATTRIBUTES &&
         GetFileAttributes (L"steam_api64.dll") == INVALID_FILE_ATTRIBUTES )
    {
      wchar_t wszDeployedSteamAPI   [MAX_PATH] = { };
      wchar_t wszDeployedSteamAPI64 [MAX_PATH] = { };

      lstrcatW   (wszDeployedSteamAPI,   wszInstallPath);
      PathAppend (wszDeployedSteamAPI, L"steam_api.dll");

      lstrcatW   (wszDeployedSteamAPI64,     wszInstallPath);
      PathAppend (wszDeployedSteamAPI64, L"steam_api64.dll");

      extern HWND hWndMainDlg;

      if ( GetFileAttributes (wszDeployedSteamAPI)   != INVALID_FILE_ATTRIBUTES ||
           GetFileAttributes (wszDeployedSteamAPI64) != INVALID_FILE_ATTRIBUTES )
      {
        STARTUPINFO         sinfo = { 0 };
        PROCESS_INFORMATION pinfo = { 0 };

        sinfo.cb          = sizeof STARTUPINFO;
        sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        sinfo.wShowWindow = SW_HIDE;

        wchar_t wszInstallerINI [MAX_PATH] = { };

        wsprintf ( wszInstallerINI,
                     L"%s\\Version\\installed.ini",
                       wszInstallPath );

        DeleteFileW (wszInstallerINI);
  
        wchar_t wszRepoINI    [MAX_PATH] = { };

        wsprintf ( wszRepoINI,
                     L"%s\\Version\\repository.ini",
                       wszInstallPath );

        DeleteFileW (wszRepoINI);

        SKIM_DeleteConfigFiles (&product);

        CreateProcess ( wszExecutable,
                          wszAppID,
                            nullptr, nullptr,
                              FALSE, 0x00,
                                nullptr, wszInstallPath,
                                  &sinfo, &pinfo );

        if (hWndMainDlg != 0) {
          ShowWindow (hWndMainDlg, SW_HIDE);
        }

        if (WaitForSingleObject ( pinfo.hProcess, INFINITE ) == WAIT_OBJECT_0) {
          CloseHandle (pinfo.hThread);
          CloseHandle (pinfo.hProcess);
        }

        if (! child) {
          ShowWindow          (hWndMainDlg, SW_SHOW);
          SetForegroundWindow (hWndMainDlg);
          SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
        } else {
          SendMessage         (hWndMainDlg, WM_CLOSE, 0x00, 0x00);
          SendMessage         (hWndMainDlg, WM_QUIT,  0x00, 0x00);
        }

        SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
        SKIM_OnProductSelect          ();
        SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
        SKIM_OnBranchSelect           ();

        return 1;
      }

      else
      {
        wchar_t wszErrorMsg [512] = { };

        _swprintf ( wszErrorMsg,
                      L"Unable to locate a valid install path (steam_api{64}.dll is missing)!\r\n\r\n"
                      L"Game is supposed to be installed to '%s', but is not.",
                        SKIM_FindInstallPath (product.uiSteamAppID) );

        MessageBox ( 0,
                      wszErrorMsg,
                        L"Special K Installer Fatal Error",
                          MB_ICONHAND | MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
      }
    }
  }

  SetCurrentDirectory (
    SKIM_FindInstallPath (product.uiSteamAppID)
  );

  extern HWND hWndMainDlg;
  ShowWindow (hWndMainDlg, SW_HIDE);

  bool bValidInstaller = 
    SKIM_FetchInstallerDLL (product);

  wchar_t wszInstallerDLL [MAX_PATH] = { };
  swprintf ( wszInstallerDLL,
              L"%s\\%s",
                wszInstallPath, product.wszWrapper );

  HMODULE hModInstaller =
    bValidInstaller ? LoadLibrary (wszInstallerDLL) : 0;


  wchar_t wszRepoINI      [MAX_PATH] = { };

  wsprintf ( wszRepoINI,
               L"%s\\Version\\repository.ini",
                 wszInstallPath );

  DeleteFileW (wszRepoINI);

  SKIM_DeleteConfigFiles (&product);


  if (hModInstaller != nullptr)
  {
    typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)(const wchar_t* wszProduct);
    typedef bool    (__stdcall *SK_FetchVersionInfo_pfn)(const wchar_t* wszProduct);

    SK_UpdateSoftware_pfn SK_UpdateSoftware =
      (SK_UpdateSoftware_pfn)
        GetProcAddress ( hModInstaller,
                          "SK_UpdateSoftware" );

    SK_FetchVersionInfo_pfn SK_FetchVersionInfo =
      (SK_FetchVersionInfo_pfn)
        GetProcAddress ( hModInstaller,
                        "SK_FetchVersionInfo" );

    if ( SK_FetchVersionInfo != nullptr &&
         SK_UpdateSoftware   != nullptr ) {
      SK_FetchVersionInfo (product.wszRepoName);
      SK_UpdateSoftware   (product.wszRepoName);
    }
  }

  // Give a second try, in case internet cache went kaput
  else if (tries == 0)
  {
    ++tries;
    DeleteFileW (wszInstallerDLL);
    SKIM_InstallProduct (user);
  }
  

  // Too many tries, give up.
  else
  {
    tries = 0;
    DeleteFileW (wszInstallerDLL);
    MessageBoxW (NULL, L"Something went wrong attempting to download Installer DLL.", L"Install Failed", MB_OK | MB_ICONERROR);
  }


  if (! child)
  {
    ShowWindow          (hWndMainDlg, SW_SHOW);
    SetForegroundWindow (hWndMainDlg);
    SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
  }

  else
  {
    SendMessage         (hWndMainDlg, WM_CLOSE, 0x00, 0x00);
    SendMessage         (hWndMainDlg, WM_QUIT,  0x00, 0x00);
  }

  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);
  SKIM_OnProductSelect          ();
  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);

  return 0;
}



#include <Windows.h>
#include <WindowsX.h>
HWND hWndMainDlg;
HWND hWndStatusBar;

bool
SKIM_DetermineInstallState (sk_product_t& product)
{
  // Special Case: SpecialK
#ifndef _WIN64
  if (! _wcsicmp (product.wszWrapper, L"SpecialK32.dll"))
#else
  if (! _wcsicmp (product.wszWrapper, L"SpecialK64.dll"))
#endif
  {
             uint32_t dwLen = MAX_PATH;
    wchar_t wszSpecialK_ROOT [MAX_PATH] = { };

    SKIM_Util_GetDocumentsDir (wszSpecialK_ROOT, &dwLen);

    PathAppend (wszSpecialK_ROOT, L"My Mods\\SpecialK");
    PathAppend (wszSpecialK_ROOT, product.wszWrapper);

    if (GetFileAttributes (wszSpecialK_ROOT) != INVALID_FILE_ATTRIBUTES)
    {
      product.install_state =
        SKIM_IsDLLFromProduct (product.wszWrapper, product.wszDLLProductName) ?
          1 : 0;
      return true;
    }

    else {
      product.install_state = 0;
      return false;
    }
  }

  wchar_t wszFileToTest [MAX_PATH] = { };

  if (lstrlenW (product.wszPlugIn))
  {
    swprintf ( wszFileToTest,
                 L"%s\\%s",
                   SKIM_FindInstallPath (product.uiSteamAppID),
                     product.wszPlugIn );
  }

  else
  {
    swprintf ( wszFileToTest,
             L"%s\\%s",
               SKIM_FindInstallPath (product.uiSteamAppID),
                 product.wszWrapper );
  }

  const wchar_t* wszInstallPath =
    SKIM_FindInstallPath (product.uiSteamAppID);

  if (wszInstallPath == nullptr || *wszInstallPath == *L"")
  {
    product.install_state = -1;
    return false;
  }

  if ( ! ( GetFileAttributes (
             SKIM_FindInstallPath (product.uiSteamAppID)
           ) & FILE_ATTRIBUTE_DIRECTORY
         )
     )
  {
    product.install_state = -1;
    return false;
  }

  else
  {
    if (GetFileAttributes (wszFileToTest) != INVALID_FILE_ATTRIBUTES)
    {
      product.install_state =
        SKIM_IsDLLFromProduct (wszFileToTest, product.wszDLLProductName) ?
          1 : 0;

      return true;
    }

    product.install_state = 0;
    return false;
  }
}

std::wstring
SKIM_SummarizeRenderAPI (sk_product_t& product)
{
  std::wstring ret = L"";

  if (! _wcsicmp (product.wszWrapper, L"d3d9.dll"))
    ret += L"Direct3D 9";
  else if (! _wcsicmp (product.wszWrapper, L"dxgi.dll")) {
    if (! _wcsicmp (product.wszRepoName, L"UnX"))
      ret += L"Direct3D 11";
    else if ( (! _wcsicmp (product.wszRepoName, L"FO4W")) ||
              (! _wcsicmp (product.wszRepoName, L"SoulsUnsqueezed")) ||
              (! _wcsicmp (product.wszRepoName, L"FAR")) )
      ret += L"Direct3D 11";
    else
      ret += L"Direct3D 10/11/12?";
  }
  else if (! _wcsicmp (product.wszWrapper, L"OpenGL32.dll"))
    ret += L"OpenGL";
#ifndef _WIN64
  else if (! _wcsicmp (product.wszWrapper, L"SpecialK32.dll"))
#else
  else if (! _wcsicmp (product.wszWrapper, L"SpecialK64.dll"))
#endif
    ret += L"GL/D3D9/11/12/Vulkan";

  if (product.architecture == SK_32_BIT)
    ret += L" (32-bit)";
  else if (product.architecture == SK_64_BIT)
    ret += L" (64-bit)";
  else if (product.architecture == SK_BOTH_BIT)
    ret += L" (32-/64-bit)";

  return ret;
}

static int last_sel = 0;

RECT
SKIM_GetHWNDRect (HWND hWnd)
{
  RECT   out;

  GetWindowRect (hWnd, &out);

  return out;
}

RECT
SKIM_GetClientRect (HWND hWnd)
{
  RECT   out;

  GetClientRect (hWnd, &out);

  return out;
}

RECT
SKIM_GetDlgItemRect (HWND hDlg, UINT nIDDlgItem)
{
  return
    SKIM_GetHWNDRect (
      GetDlgItem ( hDlg, nIDDlgItem )
    );
}

void
SKIM_OnBranchSelect (void)
{
  HWND hWndProducts =
    GetDlgItem (hWndMainDlg, IDC_PRODUCT_SELECT);

  int sel = ComboBox_GetCurSel (hWndProducts);

  static HWND
       hWndBranchSelect =
            GetDlgItem (hWndMainDlg, IDC_BRANCH_SELECT),
        hWndBranchFrame =
            GetDlgItem (hWndMainDlg, IDC_BRANCH_FRAME),
         hWndBranchDesc =
            GetDlgItem (hWndMainDlg, IDC_BRANCH_DESC),
       hWndBrachMigrate =
            GetDlgItem (hWndMainDlg, IDC_MIGRATE_CMD);

  int  count         = SKIM_CountProductBranches (&products [sel]);

  int  user_idx      = ComboBox_GetCurSel (hWndBranchSelect);
  int  installed_idx = user_idx;
  bool empty         = ComboBox_GetCount  (hWndBranchSelect) == 0;

  if (empty)
  {
    user_idx      = 0;
    installed_idx = 0;
  }

  if (count > 1)
  {
    SKIM_BranchManager::Branch* pBranch = nullptr;

    for (int i = 0; i < count; i++)
    {
      pBranch = SKIM_GetProductBranchByIdx ( &products [sel], i );

      if (! pBranch)
        continue;

      if (pBranch->name == SKIM_BranchManager::singleton ()->getCurrentBranch ()->name)
        installed_idx = i;

      if (empty)
      {
        ComboBox_InsertString ( hWndBranchSelect,
                                  i,
                                    pBranch->name.c_str ()
        );
      }
    }

    if (empty)
      user_idx = installed_idx;

    ComboBox_SetCurSel (hWndBranchSelect, user_idx);

    EnableWindow (hWndBranchSelect, TRUE);
    EnableWindow ( hWndBranchFrame, TRUE);
    EnableWindow (  hWndBranchDesc, TRUE);

    EnableWindow (hWndBrachMigrate, installed_idx != user_idx);

    std::wstring branch_desc =
      SKIM_GetProductBranchByIdx ( &products [sel], user_idx )->description;

    SetWindowText (hWndBranchDesc, branch_desc.c_str () );
  }

  else if (count <= 1) {
    ComboBox_ResetContent (hWndBranchSelect);
    ComboBox_InsertString (hWndBranchSelect, 0, L"Main");
    ComboBox_SetCurSel    (hWndBranchSelect, 0);

    SetWindowText (hWndBranchDesc, L"Default branch; fewer updates but stable.");

    EnableWindow (hWndBranchSelect, FALSE);
    EnableWindow ( hWndBranchFrame, FALSE);
    EnableWindow (  hWndBranchDesc, FALSE);
    EnableWindow (hWndBrachMigrate, FALSE);
  }

  if ((! SKIM_DetermineInstallState (products [sel])) || count < 1)
  {
    SendMessage  ( hWndStatusBar,
                    SB_SETTEXT,
                      LOBYTE (0) | HIBYTE (0),
                        (LPARAM)L"\tNot Installed" );

    EnableWindow (hWndBrachMigrate, FALSE);
  }

  else
  {
    wchar_t wszStatus [256];
    swprintf ( wszStatus,
                 L"\t%s        (%s)",
                  SKIM_BranchManager::singleton ()->getInstallPackage ().c_str (),
                    SKIM_GetProductBranchByIdx (
                      &products [sel],
                        installed_idx
                    )->name.c_str ()
             );

    SendMessage ( hWndStatusBar,
                    SB_SETTEXT,
                      LOBYTE (0) | HIBYTE (0),
                        (LPARAM)wszStatus );
  }
}

HWND hWndRichProductDescrip;

void
SKIM_OnProductSelect (void)
{
  HWND hWndProducts =
    GetDlgItem (hWndMainDlg, IDC_PRODUCT_SELECT);

  int sel = ComboBox_GetCurSel (hWndProducts);

  last_sel = sel;

  Static_SetText (
    GetDlgItem (hWndMainDlg, IDC_NAME_OF_GAME),
      products [sel].wszGameName
  );

  Static_SetText (
    GetDlgItem (hWndMainDlg, IDC_API_SUMMARY),
      SKIM_SummarizeRenderAPI (products [sel]).c_str ()
  );

  CHARFORMAT2W cf2;
  ZeroMemory (&cf2, sizeof CHARFORMAT2W);

  cf2.cbSize = sizeof CHARFORMAT2W;

  cf2.crTextColor = RGB (0, 0, 0);
  cf2.dwMask      = CFM_COLOR;

  SendMessage (hWndRichProductDescrip, EM_SETSEL,        0,             -1);
  SendMessage (hWndRichProductDescrip, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  SendMessage (hWndRichProductDescrip, EM_REPLACESEL,    FALSE,         (LPARAM)products [sel].wszDescription);

  //HICON btn_icon = 0;

  HWND hWndManage =
    GetDlgItem (hWndMainDlg, IDC_MANAGE_CMD);

  // Easy way to detect Special K
  if (products [sel].uiSteamAppID == 0)
  {
    SetWindowTextA ( hWndManage, SKIM_GetInjectorState () ? "Stop Injecting" : "Start Injecting" );
  }

  else
    SetWindowTextA ( hWndManage, "Manage/Repair" );


  HWND hWndInstall =
    GetDlgItem (hWndMainDlg, IDC_INSTALL_CMD);

  HWND hWndUninstall =
    GetDlgItem (hWndMainDlg, IDC_UNINSTALL_CMD);


  if (products [sel].install_state < 0)
  {
    Button_Enable (hWndInstall,   0);
    Button_Enable (hWndManage,    0);
    Button_Enable (hWndUninstall, 0);
  }


  else if (products [sel].install_state == 0)
  {
    Button_Enable (hWndInstall,   1);
    Button_Enable (hWndManage,    0);
    Button_Enable (hWndUninstall, 0);
  }


  else if (products [sel].install_state == 1)
  {
    Button_Enable (hWndInstall,   0);
    Button_Enable (hWndUninstall, 1);

    // Tales of Symphonia (has DLC)
    if (products [sel].uiSteamAppID == 372360)
      Button_Enable (hWndManage, 1);
    else if (products [sel].uiSteamAppID == 374320)
      Button_Enable (hWndManage, 1);
    else if (products [sel].uiSteamAppID == 351970)
      Button_Enable (hWndManage, 1);
    else
      Button_Enable (hWndManage, 0);
  }

  if (products [sel].uiSteamAppID == 0 && products [sel].install_state == 1)
  {
    Button_Enable (hWndManage, 1);
  }

  static HWND
       hWndBranchSelect =
            GetDlgItem (hWndMainDlg, IDC_BRANCH_SELECT);

  ComboBox_ResetContent (hWndBranchSelect);
  SKIM_OnBranchSelect ();
}

bool
SKIM_ConfirmClose (void)
{
  if (SKIM_GetInjectorState ())
  {
    int               nButtonPressed =  0;
    TASKDIALOGCONFIG  config         = { };
  
    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager";
    config.dwCommonButtons    = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
    config.pszMainInstruction = L"Closing SKIM Will Stop Injection";
    config.pButtons           = nullptr;
    config.cButtons           = 0;
    config.nDefaultButton     = IDCANCEL;
  
    config.dwFlags            = /*TDF_SIZE_TO_CONTENT*/0x00;
    config.pszMainIcon        = TD_WARNING_ICON;
  
    config.pszContent         = L"Any software using Special K will continue, "
                                L"however, injection into new applications will cease.";

    config.pszFooter          = L"Minimizing SKIM will remove it from the task bar.";
    //config.pszFooterIcon      = L"SKIM64.exe:1";
  
    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
  
    if ( nButtonPressed == IDCANCEL )
    {
      return false;
    }
  }

  return true;
}

INT_PTR
CALLBACK
Main_DlgProc (
  _In_ HWND   hWndDlg,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  UNREFERENCED_PARAMETER (lParam);

  HWND hWndProducts =
    GetDlgItem (hWndDlg, IDC_PRODUCT_SELECT);

  sk_product_t& product =
    products [ComboBox_GetCurSel (hWndProducts)];

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      SendMessage (hWndDlg, WM_SETICON, ICON_BIG,   (LPARAM)hIconSKIM_LG);
      SendMessage (hWndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSKIM_SM);

      static bool init = false;

      if (! init) {
        INITCOMMONCONTROLSEX icex;
        ZeroMemory (&icex, sizeof INITCOMMONCONTROLSEX);

        icex.dwSize = sizeof INITCOMMONCONTROLSEX;
        icex.dwICC  = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;

        InitCommonControlsEx (&icex);

        SKIM_Tray_Init (hWndDlg);

        LoadLibrary (L"msftedit.dll");

        CoInitializeEx (nullptr, COINIT_MULTITHREADED);

        SKIM_Tray_UpdateStartup ();

        init = true;
      }

      hWndRichProductDescrip =
        CreateWindowEx ( 0, MSFTEDIT_CLASS, TEXT ("RichTextHere"),
                           WS_VISIBLE | WS_CHILD | ES_READONLY | ES_MULTILINE|
                           ES_LEFT    | WS_DISABLED,
                           210,85,230,140,
                           hWndDlg, NULL, g_hInstance, NULL );

      hWndStatusBar =
        CreateWindowEx ( 0, STATUSCLASSNAME,
                           nullptr,
                             WS_CHILD | WS_VISIBLE |
                             SBARS_TOOLTIPS,
                                0, 0, 0, 0,
                                  hWndDlg,
                                    (HMENU)IDC_STATUS,
                                        g_hInstance, nullptr);

      SetCurrentDirectory (startup_dir);

      for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++)
      {
        SKIM_DetermineInstallState (products [i]);
      }

      hWndMainDlg = hWndDlg;

      ComboBox_SetCurSel    (hWndProducts, last_sel);
      ComboBox_ResetContent (hWndProducts);

      for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++)
      {
        ComboBox_InsertString (hWndProducts, i, products [i].wszProjectName);
      }

      ComboBox_SetCurSel   (hWndProducts, last_sel);
      SKIM_OnProductSelect ();

      return TRUE;
    }

    case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
        case IDC_PRODUCT_SELECT:
        {
          switch (HIWORD (wParam))
          {
            case CBN_SELCHANGE:
            {
              SKIM_OnProductSelect ();
            } break;
          }
        } break;

        case IDC_BRANCH_SELECT:
        {
          switch (HIWORD (wParam))
          {
            case CBN_SELCHANGE:
            {
              SKIM_OnBranchSelect ();
            } break;
          }
        } break;

        case IDC_DONATE:
        {
          int sel = ComboBox_GetCurSel (hWndProducts);

          wchar_t wszURL [1024];
          wsprintf ( wszURL,
                       L"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=%s",
                         products [sel].wszDonateID );

          ShellExecuteW (NULL, L"OPEN", wszURL, nullptr, nullptr, SW_SHOWMAXIMIZED);
        } break;

        case IDC_MIGRATE_CMD:
        {
          static HWND
            hWndBranchSelect =
              GetDlgItem (hWndMainDlg, IDC_BRANCH_SELECT);

          wchar_t wszSel [256] = { };

          ComboBox_GetText (hWndBranchSelect, wszSel, 255);

          SKIM_BranchManager::singleton ()->migrateToBranch (wszSel);
          SKIM_BranchManager::singleton ()->setProduct      ((uint32_t)-1);

          if (product.uiSteamAppID != 0)
          {
            _beginthreadex (
              nullptr,
                0,
                 SKIM_MigrateProduct,
                   &product,
                     0x00,
                       nullptr );
          }

          else if (product.uiSteamAppID == 0)
          {
            _beginthreadex (
              nullptr,
                0,
                  SKIM_MigrateGlobalInjector,
                    (LPVOID)hWndDlg,
                      0x00,
                        nullptr );
          }
        } break;

        case IDC_INSTALL_CMD:
        {
          if (product.uiSteamAppID != 0)
          {
            _beginthreadex (
              nullptr,
                0,
                 SKIM_InstallProduct,
                   &product,
                     0x00,
                       nullptr );
          }

          else if (product.uiSteamAppID == 0)
          {
            _beginthreadex (
              nullptr,
                0,
                  SKIM_InstallGlobalInjector,
                    (LPVOID)hWndDlg,
                      0x00,
                        nullptr );
          }
        } break;


        case IDC_MANAGE_CMD:
        {
          // Tales of Symphonia  (has DLC)
          //
          if (product.uiSteamAppID == 372360)
          {
            extern unsigned int
            __stdcall
            DLCDlg_Thread (LPVOID user);

            SetCurrentDirectory (
              SKIM_FindInstallPath (product.uiSteamAppID)
            );

            extern HWND hWndMainDlg;
            ShowWindow (hWndMainDlg, SW_HIDE);

            SKIM_FetchDLCManagerDLL (product);

            _beginthreadex ( nullptr, 0,
                               DLCDlg_Thread, (LPVOID)hWndMainDlg,
                                 0x00, nullptr );
          }


          // Tales of Zestiria  (has external config tool)
          //
          else if (product.uiSteamAppID == 351970)
          {
            STARTUPINFO         sinfo = { 0 };
            PROCESS_INFORMATION pinfo = { 0 };

            sinfo.cb          = sizeof STARTUPINFO;
            sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            sinfo.wShowWindow = SW_SHOWNORMAL;

            SetCurrentDirectory (
              SKIM_FindInstallPath (product.uiSteamAppID)
            );

            CreateProcess ( L"tzt.exe",
                              nullptr,
                                nullptr, nullptr,
                                  FALSE, 0x00,
                                    nullptr, SKIM_FindInstallPath (product.uiSteamAppID),
                                      &sinfo, &pinfo );

            if (hWndMainDlg != 0)
            {
              ShowWindow (hWndMainDlg, SW_HIDE);
            }

            if (WaitForSingleObject ( pinfo.hProcess, INFINITE ) == WAIT_OBJECT_0)
            {
              CloseHandle (pinfo.hThread);
              CloseHandle (pinfo.hProcess);
            }

            ShowWindow          (hWndMainDlg, SW_SHOW);
            SetForegroundWindow (hWndMainDlg);
            SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
          }


          // Dark Souls III  (has external config tool)
          //
          else if (product.uiSteamAppID == 374320)
          {
            STARTUPINFO         sinfo = { 0 };
            PROCESS_INFORMATION pinfo = { 0 };

            sinfo.cb          = sizeof STARTUPINFO;
            sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            sinfo.wShowWindow = SW_SHOWNORMAL;

            SetCurrentDirectory (
              SKIM_FindInstallPath (product.uiSteamAppID)
            );

            CreateProcess ( L"ds3t.exe",
                              nullptr,
                                nullptr, nullptr,
                                  FALSE, 0x00,
                                    nullptr, SKIM_FindInstallPath (product.uiSteamAppID),
                                      &sinfo, &pinfo );

            if (hWndMainDlg != 0)
            {
              ShowWindow (hWndMainDlg, SW_HIDE);
            }

            if (WaitForSingleObject ( pinfo.hProcess, INFINITE ) == WAIT_OBJECT_0)
            {
              CloseHandle (pinfo.hThread);
              CloseHandle (pinfo.hProcess);
            }

            ShowWindow          (hWndMainDlg, SW_SHOW);
            SetForegroundWindow (hWndMainDlg);
            SendMessage         (hWndMainDlg, WM_INITDIALOG, 0x00, 0x00);
          }


          // Global Injector  (SKIM can start/stop injection)
          //
          else if (product.uiSteamAppID == 0)
          {
            SKIM_GlobalInject_StartStop (hWndDlg);
          }
        }  break;


        case IDC_UNINSTALL_CMD:
        {
          _beginthreadex (
            nullptr,
              0,
                SKIM_UninstallProduct, &product, 0x00, nullptr);
        } break;
      }
      return (INT_PTR)true;
    }

    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
    {
      if (SKIM_ConfirmClose ())
      {
        if (SKIM_GlobalInject_Stop ())
        {
          SKIM_Tray_RemoveFrom      (       );
          SKIM_StopInjectingAndExit (hWndDlg);

          TerminateProcess (GetCurrentProcess (), 0x00);
          ExitProcess      (                      0x00);
        }
      }

      return 1;
    } break;

    case SKIM_STOP_INJECTION:
    {
      SKIM_GlobalInject_Stop (hWndDlg, false);

      return 0;
    }

    case SKIM_STOP_INJECTION_AND_EXIT:
    {
      SKIM_StopInjectingAndExit (hWndDlg, false);

      return 0;
    }

    case SKIM_START_INJECTION:
    {
      SKIM_GlobalInject_Start (hWndDlg);

      return 0;
    }

    case (WM_USER | 0x0420):
    {
      SKIM_Tray_RefreshMenu (hWndDlg);

      switch (LOWORD (lParam))
      {
        case NIN_BALLOONSHOW:
          break;
        case NIN_POPUPOPEN:
        {
        } break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
          SKIM_Tray_RestoreFrom (hWndDlg);
          break;

        case WM_CONTEXTMENU:
        {
          SKIM_Tray_HandleContextMenu (hWndDlg);
        }
        break;
      }

      return (INT_PTR)false;
    } break;

    case WM_MENUCOMMAND:
    {
      SKIM_Tray_ProcessCommand (hWndDlg, lParam, wParam);

      return (INT_PTR)false;
    } break;

    case WM_SIZE:
    {
      if      (wParam == SIZE_MINIMIZED)
      {
        SKIM_Tray_SendTo (hWndDlg);
      }

      else if (wParam == SIZE_RESTORED)
      {
        SKIM_Tray_RestoreFrom (hWndDlg);
      }

      return (INT_PTR)false;
    } break;

    case WM_PAINT:
    case WM_CREATE:
      return (INT_PTR)false;
  }

  return (INT_PTR)false;
}

sk_product_t*
SKIM_FindProductByAppID (uint32_t appid)
{
  for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++) {
    if (products [i].uiSteamAppID == appid)
      return &products [i];
  }

  return nullptr;
}

void
SKIM_DisableGlobalInjector (void)
{
  SKIM_GlobalInject_Stop (false);
  SKIM_GlobalInject_Free ();

  // Wait for uninjection to finish (the stupid way)
  int iters = 0;
  while (GetFileAttributes (L"SpecialK32.pid") != INVALID_FILE_ATTRIBUTES && iters < 4) {
    Sleep (150UL); ++iters;
  }

  time_t _time;
  time (&_time);

  wchar_t wszTemp      [MAX_PATH] = { };
  wchar_t wsz32BitDLL  [MAX_PATH] = { };
  wchar_t wsz64BitDLL  [MAX_PATH] = { };

  wcscpy     (wsz32BitDLL, SKIM_FindInstallPath (0));
  PathAppend (wsz32BitDLL, L"SpecialK32.dll");

  wcscpy     (wsz64BitDLL, SKIM_FindInstallPath (0));
  PathAppend (wsz64BitDLL, L"SpecialK64.dll");

  GetTempFileNameW (SKIM_FindInstallPath (0), L"SKI", _time, wszTemp);
  MoveFileW        (wsz32BitDLL, wszTemp);
  DeleteFileW      (wszTemp);

  *wszTemp = L'\0';

  GetTempFileNameW (SKIM_FindInstallPath (0), L"SKI", _time + 1, wszTemp);
  MoveFileW        (wsz64BitDLL, wszTemp);
  DeleteFileW      (wszTemp);

  wchar_t wszRepo      [MAX_PATH];
  wchar_t wszInstalled [MAX_PATH];

  wcscpy (wszRepo,      SKIM_FindInstallPath (0));
  wcscpy (wszInstalled, SKIM_FindInstallPath (0));

  PathAppend (wszInstalled, L"Version\\installed.ini");
  PathAppend (wszRepo,      L"Version\\repository.ini");

  DeleteFileW (wszInstalled);
  DeleteFileW (wszRepo);
}


//
// XXX: This needs re-writing for 0.8.x
//
unsigned int
__stdcall
SKIM_MigrateGlobalInjector (LPVOID user)
{
  HWND hWndParent = (HWND)user;

  SKIM_GlobalInject_Stop (false);
  SKIM_GlobalInject_Free ();

  wchar_t wszDestDLL32 [MAX_PATH] = { };
  wchar_t wszDestDLL64 [MAX_PATH] = { };

  uint32_t dwStrLen = MAX_PATH;
  SKIM_Util_GetDocumentsDir (wszDestDLL32, &dwStrLen);

  dwStrLen = MAX_PATH;
  SKIM_Util_GetDocumentsDir (wszDestDLL64, &dwStrLen);

  PathAppend (wszDestDLL32, L"My Mods\\SpecialK\\");
  PathAppend (wszDestDLL64, L"My Mods\\SpecialK\\");

  // Create the destination directory
  SKIM_Util_CreateDirectories (wszDestDLL32);

  SetCurrentDirectoryW (wszDestDLL32);

  GetShortPathNameW (wszDestDLL32, wszDestDLL32, MAX_PATH);
  GetShortPathNameW (wszDestDLL64, wszDestDLL64, MAX_PATH);

  if (StrStrIW (wszDestDLL32, L" ")) {
    MessageBox ( hWndParent,
                   L"You may have 8.3 Filenames Disabled; they are necessary to "
                   L"make this software work!\n\n"
                   L"Delete the directory \"Documents\\My Mods\" after you fix "
                   L"Windows and run the software again.",
                     L"MS-DOS 8.3 Filename Not Possible",
                       MB_ICONERROR | MB_OK );
  }

  else {
    PathAppend (wszDestDLL32, L"SpecialK32.dll");
    PathAppend (wszDestDLL64, L"SpecialK64.dll");

    sk_product_t sk32 =
    {
      L"SpecialK32.dll",
      L"",
      L"Special K",
      L"Special K",
      L"Special K (Global Injector)",
      L"SpecialK",
      L"8A7FSUFJ6KB2U",
      0,
      SK_BOTH_BIT,
      nullptr,
      0
    };

    sk_product_t sk64 =
    {
      L"SpecialK64.dll",
      L"",
      L"Special K",
      L"Special K",
      L"Special K (Global Injector)",
      L"SpecialK",
      L"8A7FSUFJ6KB2U",
      0,
      SK_BOTH_BIT,
      nullptr,
      0
    };

    wcsncpy ( sk32.wszWrapper, 
                wszDestDLL32,
                  MAX_PATH );

    wcsncpy ( sk64.wszWrapper, 
                wszDestDLL64,
                  MAX_PATH );

    SKIM_FetchInjector32 (sk32);

#ifdef _WIN64
    SKIM_FetchInjector64 (sk64);
#endif


    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = g_hInstance;
    config.hwndParent         = hWndParent;
    config.pszWindowTitle     = L"Special K Install Manager";
    config.dwCommonButtons    = 0;//TDCBF_OK_BUTTON;
    config.pszMainInstruction = L"Installation of Global Injector Pending";
    config.pButtons           = nullptr;
    config.cButtons           = 0;
    config.pszContent         = L"Global Injector Downloaded but not Activated\r\n"
                                L"For compatibility options, PRESS AND HOLD "
                                L"Ctrl + Shift before launching a game.";
    config.pszMainIcon        = TD_INFORMATION_ICON;

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
  }


  SKIM_GlobalInject_Start ();

  ShowWindow  ((HWND)hWndParent, SW_SHOW);
  SendMessage ((HWND)hWndParent, WM_INITDIALOG, 0x00, 0x00);


  SKIM_OnProductSelect          ();
  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);

  return 0;
}

unsigned int
__stdcall
SKIM_InstallGlobalInjector (LPVOID user)
{
  HWND hWndParent = (HWND)user;

  SKIM_GlobalInject_Stop (false);

  wchar_t wszDestDLL32 [MAX_PATH] = {  };
  wchar_t wszDestDLL64 [MAX_PATH] = {  };

  uint32_t dwStrLen = MAX_PATH;
  SKIM_Util_GetDocumentsDir (wszDestDLL32, &dwStrLen);

  dwStrLen = MAX_PATH;
  SKIM_Util_GetDocumentsDir (wszDestDLL64, &dwStrLen);

  PathAppend (wszDestDLL32, L"My Mods\\SpecialK");
  PathAppend (wszDestDLL64, L"My Mods\\SpecialK");

  // Create the destination directory
  SKIM_Util_CreateDirectories (wszDestDLL32);

  wchar_t wszDestInstaller [MAX_PATH] = { };
  lstrcatW (wszDestInstaller, wszDestDLL32);
#ifdef _WIN64
  PathAppend (wszDestInstaller, L"SKIM64.exe");
#else
  PathAppend (wszDestInstaller, L"SKIM.exe");
#endif

  wchar_t wszExec [MAX_PATH] = { };

  GetModuleFileName (GetModuleHandle (nullptr), wszExec, MAX_PATH);
  DeleteFileW       (wszDestInstaller);
  MoveFileW         (wszExec, wszDestInstaller);

  if (wcsicmp (startup_dir, wszDestDLL32))
  {
    MessageBox ( NULL,
                   L"SKIM64.exe has been moved to Documents\\My Mods\\SpecialK\r\n\r\n"
                   L"\tPlease run it from there from now on",
                     L"Installer Moved",
                       MB_OK | MB_ICONINFORMATION );
    TerminateProcess (GetCurrentProcess (), 0x0);
  }

  SetCurrentDirectoryW (wszDestDLL32);
  wcscpy               (startup_dir, wszDestDLL32);

  DeleteFileW (L"Version\\installed.ini");

  GetShortPathNameW (wszDestDLL32, wszDestDLL32, MAX_PATH);
  GetShortPathNameW (wszDestDLL64, wszDestDLL64, MAX_PATH);

#if 0
  if (StrStrIW (wszDestDLL32, L" ")) {
    MessageBox ( hWndParent,
                   L"You may have 8.3 Filenames Disabled; they are necessary to "
                   L"make this software work!\n\n"
                   L"Delete the directory \"Documents\\My Mods\" after you fix "
                   L"Windows and run the software again.",
                     L"MS-DOS 8.3 Filename Not Possible",
                       MB_ICONERROR | MB_OK );
  }

  else 
#endif
  {
    PathAppend (wszDestDLL32, L"SpecialK32.dll");
    PathAppend (wszDestDLL64, L"SpecialK64.dll");

    sk_product_t sk32 =
    {
      L"SpecialK32.dll",
      L"",
      L"Special K",
      L"Special K",
      L"Special K (Global Injector)",
      L"SpecialK/0.8.x",
      L"8A7FSUFJ6KB2U",
      0,
      SK_BOTH_BIT,
      nullptr,
      0
    };

    sk_product_t sk64 =
    {
      L"SpecialK64.dll",
      L"",
      L"Special K",
      L"Special K",
      L"Special K (Global Injector)",
      L"SpecialK/0.8.x",
      L"8A7FSUFJ6KB2U",
      0,
      SK_BOTH_BIT,
      nullptr,
      0
    };

    wcsncpy ( sk32.wszWrapper, 
                wszDestDLL32,
                  MAX_PATH );

    wcsncpy ( sk64.wszWrapper, 
                wszDestDLL64,
                  MAX_PATH );

    SKIM_FetchInjector32  (sk32);

#ifdef _WIN64
    SKIM_FetchInjector64  (sk64);
#endif

    HMODULE hModInstaller =
      LoadLibrary (wszDestDLL64);

    if (hModInstaller != nullptr)
    {
      typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)(const wchar_t* wszProduct);
      typedef bool    (__stdcall *SK_FetchVersionInfo_pfn)(const wchar_t* wszProduct);

      SK_UpdateSoftware_pfn SK_UpdateSoftware =
        (SK_UpdateSoftware_pfn)
          GetProcAddress ( hModInstaller,
                            "SK_UpdateSoftware" );

      SK_FetchVersionInfo_pfn SK_FetchVersionInfo =
        (SK_FetchVersionInfo_pfn)
          GetProcAddress ( hModInstaller,
                          "SK_FetchVersionInfo" );

      if ( SK_FetchVersionInfo != nullptr &&
           SK_UpdateSoftware   != nullptr )
      {
        ShowWindow  ((HWND)hWndParent, SW_HIDE);

        SK_FetchVersionInfo (L"SpecialK");
        //SK_UpdateSoftware   (L"SpecialK");
        {
          int               nButtonPressed = 0;
          TASKDIALOGCONFIG  config         = {0};

          config.cbSize              = sizeof (config);
          config.hInstance           = g_hInstance;
          config.hwndParent          = hWndParent;
          config.pszWindowTitle      = L"Special K Install Manager";
          config.dwCommonButtons     = 0;//TDCBF_OK_BUTTON;
          config.pszMainInstruction  = L"Setup of Global Injector Successful";
          config.pButtons            = nullptr;
          config.cButtons            = 0;
          config.pszContent          = L"Installation will complete when you "
                                       L"run a Steam game.";
          config.pszFooterIcon       = TD_SHIELD_ICON;
          config.pszFooter           = L"Compatibility Menu:   HOLD "
                                       L"Ctrl + Shift at game startup.";
          config.pszMainIcon         = TD_INFORMATION_ICON;
          config.pszVerificationText = L"Start With Windows";
          config.dwFlags             = TDF_VERIFICATION_FLAG_CHECKED;

          BOOL bStartWithWindows = TRUE;

          TaskDialogIndirect (&config, &nButtonPressed, nullptr, &bStartWithWindows);

          SKIM_SetStartupInjection (bStartWithWindows, wszDestInstaller);
          SKIM_GlobalInject_Start  ();
        }

        ShowWindow  ((HWND)hWndParent, SW_SHOW);
        SendMessage ((HWND)hWndParent, WM_INITDIALOG, 0x00, 0x00);
      }

      //FreeLibrary (hModInstaller);
    }
  }

  SKIM_OnProductSelect          ();
  SKIM_BranchManager::singleton ()->setProduct ((uint32_t)-1);

  return 0;
}

#include <memory>

int
WINAPI
wWinMain ( _In_     HINSTANCE hInstance,
           _In_opt_ HINSTANCE hPrevInstance,
           _In_     LPWSTR    lpCmdLine,
           _In_     int       nCmdShow )
{
  g_hInstance    =                   hInstance;
  hIconSKIM_LG   = (HICON)LoadImage (hInstance, MAKEINTRESOURCE (IDI_ICON1), IMAGE_ICON, 128, 128, 0x00);
  hIconSKIM_SM   =        LoadIcon  (hInstance, MAKEINTRESOURCE (IDI_ICON1));

  GetCurrentDirectoryW (MAX_PATH, startup_dir);


  std::unique_ptr <wchar_t> wszCmdLine (
    GetCommandLineW ()
  );

  wchar_t* wszArgs =
    PathGetArgsW (wszCmdLine.get ());


                                 WNDCLASS wc = { };
  GetClassInfo  (g_hInstance, L"#32770", &wc);
                                          wc.lpszClassName = L"SKIM_Frontend";
  RegisterClass (                        &wc);

  HWND hWndExisting =
    FindWindow (L"SKIM_Frontend", nullptr);



  bool injector_action = false;
  bool __SKIM_Inject   = false,
       __SKIM_Uninject = false;

  if (wcslen (wszArgs))
  {
         if (StrStrIW (wszArgs, L"+Inject")) __SKIM_Inject   = true;
    else if (StrStrIW (wszArgs, L"-Inject")) __SKIM_Uninject = true;

    if (__SKIM_Inject || __SKIM_Uninject)
      injector_action = true;
  }

  // If we're not starting / stopping injection, then reuse an existing instance.
  else if ((! wcslen (wszArgs)) && IsWindow (hWndExisting))
  {
    SetForegroundWindow (hWndExisting);
    BringWindowToTop    (hWndExisting);
    SetActiveWindow     (hWndExisting);

    PostMessage (hWndExisting, WM_SIZE, SIZE_RESTORED, 0);

    return 0;
  }


  //for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++)
  //{
  //  SKIM_DetermineInstallState (products [i]);
  //}


  if (injector_action)
  {
    if (__SKIM_Uninject)
    {
      // Exit the existing instance
      if (IsWindow (hWndExisting))
        SendMessage (hWndExisting, SKIM_STOP_INJECTION_AND_EXIT, 0, 0);

      TerminateProcess (GetCurrentProcess (), 0x00);
      ExitProcess      (                      0x00);
    }

    if (__SKIM_Inject)
    {
      __SKIM_Inject = false;

      if ((! IsWindow (hWndExisting)) && SKIM_GlobalInject_Start ())
      {
        hWndMainDlg =
          CreateDialog ( hInstance,
                           MAKEINTRESOURCE (IDD_FRONTEND),
                             0,
                               Main_DlgProc );

        // Minimize to system tray immediately
        PostMessage (hWndMainDlg, WM_SIZE, SIZE_MINIMIZED, 0);
      }

      else
      {
        if (IsWindow (hWndExisting))
          SendMessage (hWndExisting, SKIM_START_INJECTION, 0, 0);

        TerminateProcess (GetCurrentProcess (), 0x00);
        ExitProcess      (                      0x00);
      }
    }
  }

  else
  {
    hWndMainDlg =
      CreateDialog ( hInstance,
                       MAKEINTRESOURCE (IDD_FRONTEND),
                         0,
                           Main_DlgProc );

    // Show normally (or whatever nCmdShow tells us to do)
    ShowWindow (hWndMainDlg, nCmdShow);
  }


  wchar_t wszTitle [128] = { };
  lstrcatW ( wszTitle,
                L"Special K Install Manager - (v " );
  lstrcatW ( wszTitle,
                SKIM_VERSION_STR_W );
  lstrcatW ( wszTitle,
                L")" );

  SetWindowText (hWndMainDlg, wszTitle);


  // We can install PlugIns by passing their AppID through the cmdline
  if ((! injector_action) && wcslen (wszArgs))
  {
    int32_t appid = _wtoi (wszArgs);

    if (appid > 0)
    {
      sk_product_t* prod =
        SKIM_FindProductByAppID (appid);

      if (prod != nullptr)
      {
        child = true;

        // Set the correct selection after re-launching
        uintptr_t idx = ( (uintptr_t)prod - (uintptr_t)products ) /
                         sizeof sk_product_t;

        HWND hWndProductCtrl =
          GetDlgItem (hWndMainDlg, IDC_PRODUCT_SELECT);

        ComboBox_SetCurSel   (hWndProductCtrl, idx);
        SKIM_OnProductSelect ();

        SetCurrentDirectory (SKIM_FindInstallPath (appid));
        _beginthreadex ( nullptr, 0,
                           SKIM_InstallProduct, prod,
                             0x00, nullptr );
      }
    }

    else
    {
      bool zero = StrStrIW (wszArgs, L"0")  != nullptr ? wcslen (StrStrIW (wszArgs, L"0"))  == 1 : false;
      bool neg1 = StrStrIW (wszArgs, L"-1") != nullptr ? wcslen (StrStrIW (wszArgs, L"-1")) == 2 : false;

      // Install Global Injector
      if (zero && appid == 0)
      {
        _beginthreadex (
          nullptr,
            0,
              SKIM_InstallGlobalInjector,
                (LPVOID)hWndMainDlg,
                  0x00,
                    nullptr );
      }

      // Uninstall Global Injector
      else if (neg1)
      {
        _beginthreadex (
          nullptr,
            0,
              SKIM_UninstallProduct, SKIM_FindProductByAppID (0), 0x00, nullptr);
      }
    }
  }


  MSG  msg;
  BOOL bRet;

  while (hWndMainDlg != 0 && (bRet = GetMessage (&msg, 0, 0, 0)) != 0)
  {
    if (bRet == -1)
      break;

    if (hWndMainDlg != 0)
    {
      TranslateMessage (&msg);
      DispatchMessage  (&msg);
    }

    else
      break;
  }

  // Prevent DLL shutdown, we didn't load the DLLs for their regular intended purpose
  TerminateProcess (GetCurrentProcess (), 0x00);
  ExitProcess      (                      0x00);

UNREFERENCED_PARAMETER (nCmdShow);
UNREFERENCED_PARAMETER (lpCmdLine);
UNREFERENCED_PARAMETER (hPrevInstance);

  //return 0;
}