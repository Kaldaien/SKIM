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
#include <algorithm>

#include "Resource.h"

#include "AppInit_DLLs.h"

void
SKIM_DisableGlobalInjector (void);

unsigned int
__stdcall
SKIM_InstallGlobalInjector (LPVOID user);

bool
SKIM_GetDocumentsDir (wchar_t* buf, uint32_t* pdwLen);

unsigned int
__stdcall
SKIM_FeatureUnsupportedMessage (LPVOID user);

enum SK_ARCHITECTURE {
  SK_32_BIT   = 0x01,
  SK_64_BIT   = 0x02,

  SK_BOTH_BIT = 0x03
};

struct sk_product_t {
  wchar_t         wszWrapper     [MAX_PATH];
  wchar_t         wszPlugIn      [32];
  wchar_t         wszGameName    [128];
  wchar_t         wszProjectName [128];
  wchar_t         wszRepoName    [32];
  uint32_t        uiSteamAppID;
  SK_ARCHITECTURE architecture;
  wchar_t*        wszDescription;

   int32_t        install_state; // To be filled-in later
} products [] =

{
  {
#ifndef _WIN64
    L"SpecialK32.dll",
#else
    L"SpecialK64.dll",
#endif
    L"",
    L"Special K",
    L"Special K (Global Injector)",
    L"SpecialK",
    0,
    SK_BOTH_BIT,
    L"Applies Special K's non-game-specific features to all Steam games "
    L"launched on your system.\r\n\r\n"

    L"These include things like Steam achievement unlock sound, "
    L"mouse cursor fixes and various framerate enhancing options "
    L"for D3D9/11/12 games.\r\n\r\n",
    0
  },

  {
    L"d3d9.dll",
    L"tzfix.dll",
    L"Tales of Zestiria",
    L"Tales of Zestiria \"Fix\"",
    L"TZF",
    351970,
    //SK_32_BIT,
    SK_BOTH_BIT,
    L"Adds 60 FPS support, vastly improves graphics, adds aspect ratio "
    L"correction and fixes multi-channel / high sample-rate audio stability.",
    0
  },

  {
    L"d3d9.dll",
    L"tsfix.dll",
    L"Tales of Symphonia",
    L"Tales of Symphonia \"Fix\"",
    L"TSF",
    372360,
    //SK_32_BIT,
    SK_BOTH_BIT,
    L"Adds MSAA, fixes Namco's framerate limiter, supports 4K textures, "
    L"fixes input-related problems, helps your children with homework.",
    0
  },

  {
    L"dxgi.dll",
    L"",
    L"Fallout 4",
    L"Fallout 4 \"Works\"",
    L"FO4W",
    377160,
    SK_64_BIT,
    //SK_BOTH_BIT,
    L"Dramatically improves framepacing to eliminate seasickness while "
    L"moving from point A to point B.\r\n\r\n"
    L"  (Use the Global Injector; Plug-In is built-in)",
    0
  },

  {
    L"OpenGL32.dll",
    L"PrettyPrinny.dll",
    L"Disgaea PC",
    L"Pretty Prinny",
    L"PrettyPrinny",
    405900,
    //SK_32_BIT,
    SK_BOTH_BIT,
    L"Improves framepacing, optimizes post-processing, removes the 720p "
    L"resolution lock, adds borderless window and MSAA, supports custom "
    L"button icons; tends to be much more reliable than the game itself.",
    0
  },

  {
    L"dxgi.dll",
    L"",
    L"Dark Souls III",
    L"Souls \"Unsqueezed\"",
    L"SoulsUnsqueezed",
    374320,
    SK_64_BIT,
    L"Adds support for non-16:9 aspect ratios, texture memory optimizations "
    L"and multi-monitor rendering.\r\n\r\n"
    L"  (Use the Global Injector; Plug-In is built-in)",
    0
  },

  {
    L"dxgi.dll",
    L"UnX.dll",
    L"Final Fantasy X / X-2 HD Remaster",
    L"\"Untitled\" Project X",
    L"UnX",
    359870,
    //SK_32_BIT,
    SK_BOTH_BIT,
    L"Adds dual-audio support, texture modding cutscene skipping in FFX, "
    L"cursor management, Intel GPU bypass, fullscreen exclusive mode, "
    L"maps all PC-specific extra features to gamepad.\n",
    0
  }
};

const wchar_t*
SKIM_GetSteamDir (void)
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
SKIM_CreateDirectories ( const wchar_t* wszPath )
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

const wchar_t*
SKIM_FindInstallPath (uint32_t appid)
{
  static wchar_t wszGamePath [MAX_PATH] = { L'\0' };

  // Special Case: AppID 0 = Special K
  if (appid == 0) {
    uint32_t dwLen = MAX_PATH;

    SKIM_GetDocumentsDir (wszGamePath, &dwLen);

    lstrcatW (wszGamePath, L"\\My Mods\\SpecialK\\");

    return wszGamePath;
  }

  typedef char* steam_library_t [MAX_PATH];

#define MAX_STEAM_LIBRARIES 16
  int             steam_libs = 0;
  steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { 0 };

  const wchar_t* wszSteamPath =
    SKIM_GetSteamDir ();

  if (wszSteamPath != nullptr) {
    wchar_t wszLibraryFolders [MAX_PATH];

    lstrcpyW (wszLibraryFolders, wszSteamPath);
    lstrcatW (wszLibraryFolders, L"\\steamapps\\libraryfolders.vdf");

    if (GetFileAttributesW (wszLibraryFolders) != INVALID_FILE_ATTRIBUTES) {
      HANDLE hLibFolders =
        CreateFileW ( wszLibraryFolders,
                        GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                              OPEN_EXISTING,
                                GetFileAttributesW (wszLibraryFolders),
                                  nullptr );

      if (hLibFolders != INVALID_HANDLE_VALUE) {
        DWORD  dwSize,
               dwSizeHigh,
               dwRead;

        // This isn't a 4+ GiB file... so who the heck cares about the high-bits?
        dwSize = GetFileSize (hLibFolders, &dwSizeHigh);

        void* data =
          HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwSize);

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

        HeapFree (GetProcessHeap (), 0, data);

        CloseHandle (hLibFolders);
      }
    }
  }

  // Search custom library paths first
  if (steam_libs != 0) {
    for (int i = 0; i < steam_libs; i++) {
      char szManifest [MAX_PATH] = { '\0' };

      sprintf ( szManifest,
                  "%s\\steamapps\\appmanifest_%d.acf",
                    (char *)steam_lib_paths [i],
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

        if (hManifest != INVALID_HANDLE_VALUE) {
          DWORD  dwSize,
                 dwSizeHigh,
                 dwRead;

          dwSize = GetFileSize (hManifest, &dwSizeHigh);

          char* szManifestData =
            (char *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwSize);

          ReadFile ( hManifest,
                       szManifestData,
                         dwSize,
                           &dwRead,
                             nullptr );

          CloseHandle (hManifest);

          if (! dwRead) {
            HeapFree (GetProcessHeap (), 0, szManifestData);
            continue;
          }

          char* szInstallDir =
            StrStrIA (szManifestData, "\"installdir\"");

          char szGamePath [MAX_PATH] = { '\0' };

          if (szInstallDir != nullptr) {
            std::string install_dir (szInstallDir);
            std::transform ( install_dir.begin     (),
                               install_dir.end     (),
                                 install_dir.begin (),
                                   tolower );

            strcpy (szInstallDir, install_dir.c_str ());

            sscanf ( szInstallDir,
                       "\"installdir\" \"%259[^\"]\"",
                         szGamePath );
          }

          swprintf ( wszGamePath,
                       L"%hs\\steamapps\\common\\%hs",
                         (char *)steam_lib_paths [i],
                           szGamePath );

          if (GetFileAttributesW (wszGamePath) & FILE_ATTRIBUTE_DIRECTORY) {
            HeapFree (GetProcessHeap (), 0, szManifestData);
            return wszGamePath;
          }

          HeapFree (GetProcessHeap (), 0, szManifestData);
        }
      }
    }
  }

  char szManifest [MAX_PATH] = { '\0' };

  sprintf ( szManifest,
              "%ls\\steamapps\\appmanifest_%d.acf",
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

    if (hManifest != INVALID_HANDLE_VALUE) {
      DWORD  dwSize,
             dwSizeHigh,
             dwRead;

      dwSize = GetFileSize (hManifest, &dwSizeHigh);

      char* szManifestData =
        (char *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwSize);

      ReadFile ( hManifest,
                   szManifestData,
                     dwSize,
                       &dwRead,
                         nullptr );

      CloseHandle (hManifest);

      if (! dwRead) {
        HeapFree (GetProcessHeap (), 0, szManifestData);
        return nullptr;
      }

      char* szInstallDir =
        StrStrIA (szManifestData, "\"installdir\"");

      char szGamePath [MAX_PATH] = { '\0' };

      if (szInstallDir != nullptr) {
        std::string install_dir (szInstallDir);
        std::transform ( install_dir.begin     (),
                           install_dir.end     (),
                             install_dir.begin (),
                               tolower );

        strcpy (szInstallDir, install_dir.c_str ());

        sscanf ( szInstallDir,
                   "\"installdir\" \"%259[^\"]\"",
                     szGamePath );
      }

      swprintf ( wszGamePath,
                   L"%ls\\steamapps\\common\\%hs",
                     wszSteamPath,
                       szGamePath );

      if (GetFileAttributesW (wszGamePath) & FILE_ATTRIBUTE_DIRECTORY) {
        HeapFree (GetProcessHeap (), 0, szManifestData);
        return wszGamePath;
      }

      HeapFree (GetProcessHeap (), 0, szManifestData);
    }
  }

  return nullptr;
}


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

  if (hFile == INVALID_HANDLE_VALUE)
    return 0;

  uint32_t filecrc32 = 0;
  DWORD    dwSize,
           dwSizeHigh,
           dwRead;

  // This isn't a 4+ GiB file... so who the heck cares about the high-bits?
  dwSize = GetFileSize (hFile, &dwSizeHigh);

  void* data =
    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, dwSize);

  if (data == nullptr) {
    CloseHandle (hFile);
    return 0;
  }

  dwRead = dwSize;

  if (ReadFile (hFile, data, dwSize, &dwRead, nullptr)) {
    filecrc32 =
      crc32 (0x00, data, dwRead);
  }

  HeapFree (GetProcessHeap (), 0, data);

  CloseHandle (hFile);

  return filecrc32;
}

class iImportLibrary
{
public:
               iImportLibrary (const wchar_t* wszName) { UNREFERENCED_PARAMETER(wszName); }

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
SKIM_IsAdmin (void)
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
SKIM_IsProcessRunning (const wchar_t* wszProcName)
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
SKIM_GetDocumentsDir (wchar_t* buf, uint32_t* pdwLen)
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

void
SKIM_EnableAppInitDLLs (bool enable)
{
  BOOL bWoW64 = FALSE;

  IsWow64Process (GetCurrentProcess (), &bWoW64);

  if (enable)
    return;
}


#include <WinInet.h>
#pragma comment (lib, "wininet.lib")

bool
__stdcall
SKIM_FetchDLCManagerDLL (  sk_product_t  product,
                           const wchar_t *wszRemoteFile =
#ifndef _WIN64
L"installer.dll"
#else
L"installer64.dll"
#endif
)
{
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

  wchar_t wszRemoteRepoURL [MAX_PATH] = { L'\0' };

  wsprintf ( wszRemoteRepoURL,
               L"/Kaldaien/SpecialK/master/%s",
                /*product.wszRepoName,*/ wszRemoteFile );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_NO_UI          | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
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
                                  0 ) ) {
    DWORD dwSize;

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      DWORD dwAttribs = GetFileAttributes (wszRemoteFile);

      if (dwAttribs == INVALID_FILE_ATTRIBUTES)
        dwAttribs = FILE_ATTRIBUTE_NORMAL;

      HANDLE hVersionFile =
        CreateFileW ( wszRemoteFile,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                dwAttribs |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hVersionFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        DWORD    dwRead = 0;
        uint8_t *pData  = (uint8_t *)malloc (dwSize);

        if (! pData)
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

        free (pData);
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      if (hVersionFile != INVALID_HANDLE_VALUE)
        CloseHandle (hVersionFile);
    }

    HttpEndRequest ( hInetGitHubOpen, nullptr, 0x00, 0 );
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot);

  return true;
}

bool
__stdcall
SKIM_FetchInstallerDLL (  sk_product_t  product,
                         const wchar_t *wszRemoteFile =
#ifndef _WIN64
L"installer.dll"
#else
L"installer64.dll"
#endif
)
{
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

  wchar_t wszRemoteRepoURL [MAX_PATH] = { L'\0' };

  wsprintf ( wszRemoteRepoURL,
               L"/Kaldaien/SpecialK/master/%s",
                /*product.wszRepoName,*/ wszRemoteFile );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_NO_UI          | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
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
                                  0 ) ) {
    DWORD dwSize;

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      DWORD dwAttribs = GetFileAttributes (product.wszWrapper);

      if (dwAttribs == INVALID_FILE_ATTRIBUTES)
        dwAttribs = FILE_ATTRIBUTE_NORMAL;

      HANDLE hVersionFile =
        CreateFileW ( product.wszWrapper,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                dwAttribs |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hVersionFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        DWORD    dwRead = 0;
        uint8_t *pData  = (uint8_t *)malloc (dwSize);

        if (! pData)
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

        free (pData);
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      if (hVersionFile != INVALID_HANDLE_VALUE)
        CloseHandle (hVersionFile);
    }

    HttpEndRequest ( hInetGitHubOpen, nullptr, 0x00, 0 );
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot);

  return true;
}

bool
__stdcall
SKIM_FetchInstaller32 ( sk_product_t product )
{
  return SKIM_FetchInstallerDLL ( product, L"installer.dll" );
}

bool
__stdcall
SKIM_FetchInstaller64 ( sk_product_t product )
{
  return SKIM_FetchInstallerDLL ( product, L"installer64.dll" );
}



#include <Msi.h>
#pragma comment (lib, "msi.lib")

bool
SKIM_TestVisualCRuntime (SK_ARCHITECTURE arch)
{
  if (arch == SK_64_BIT) {
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

  else if (arch == SK_32_BIT) {
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
    return SKIM_TestVisualCRuntime (SK_32_BIT) &&
           SKIM_TestVisualCRuntime (SK_64_BIT);
  }
}

void
SKIM_UninstallProduct (sk_product_t product, HWND parent_dlg)
{
  ShowWindow (parent_dlg, SW_HIDE);

  SetCurrentDirectory (SKIM_FindInstallPath (product.uiSteamAppID));

  DeleteFileW (product.wszWrapper);
  DeleteFileW (product.wszPlugIn);

  DeleteFileW (L"Version\\installed.ini");
  DeleteFileW (L"Version\\repository.ini");

  if (product.uiSteamAppID == 0) {
    SKIM_DisableGlobalInjector ();
  }

  wchar_t wszUninstall [256];
  wsprintf ( wszUninstall, L"%s has been successfully uninstalled.",
               product.wszProjectName );

  MessageBox ( parent_dlg,
                 wszUninstall,
                   L"Uninstall Success",
                     MB_OK | MB_ICONINFORMATION );

  ExitProcess (0x00);
}

unsigned int
__stdcall
SKIM_InstallProduct (LPVOID user)//sk_product_t* pProduct)
{
  sk_product_t& product = *(sk_product_t *)user;

  // Disable installation of FO4W / SUS
  if ( product.uiSteamAppID == 377160 ||
       product.uiSteamAppID == 374320 ) {
    CloseHandle (GetCurrentThread ());
    return 0;
  }

  if (! SKIM_TestVisualCRuntime (product.architecture)) {
    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = GetModuleHandle (nullptr);
    config.hwndParent         = GetActiveWindow ();
    config.pszWindowTitle     = L"Special K Install Manager (Fatal Error)";
    config.dwCommonButtons    = 0;//TDCBF_OK_BUTTON;
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

        if (uNotification == TDN_HYPERLINK_CLICKED) {
          ShellExecuteW ( hWnd,
                            L"OPEN",
                              (wchar_t *)lParam,
                                nullptr, nullptr,
                                  SW_SHOWMAXIMIZED );

          ExitProcess (0x00);
        }

        return S_FALSE;
      };

    config.dwFlags           |= TDF_ENABLE_HYPERLINKS;

    config.pszMainIcon        = TD_ERROR_ICON;

    if (product.architecture == SK_32_BIT) {
      config.pszContent       = L"Please grab the x86"
                                L" version from <a href=\""
                                L"https://www.microsoft.com/en-us/"
                                L"download/details.aspx?id=53587\">here</a>"
                                L" to continue.";
    } else if (product.architecture == SK_64_BIT) {
      config.pszContent       = L"Please grab the x64"
                                L" version from <a href=\""
                                L"https://www.microsoft.com/en-us/"
                                L"download/details.aspx?id=53587\">here</a>"
                                L" to continue.";
    } else {
      config.pszContent       = L"Please grab _BOTH_, the x86 and x64 "
                                L"versions from <a href=\""
                                L"https://www.microsoft.com/en-us/"
                                L"download/details.aspx?id=53587\">here</a>"
                                L" to continue.";
    }

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
    return 0;
  }

  wchar_t wszInstallPath [MAX_PATH] = { L'\0' };

  wcscpy ( wszInstallPath,
            SKIM_FindInstallPath (product.uiSteamAppID) );

  wchar_t wszAppID      [16]       = { L'\0' };
  wchar_t wszExecutable [MAX_PATH] = { L'\0' };

  GetModuleFileName (GetModuleHandle (nullptr), wszExecutable, MAX_PATH);

  swprintf (wszAppID, L"%lu", product.uiSteamAppID);

  if ( product.uiSteamAppID != 0 ) {
    if ( GetFileAttributes (L"steam_api.dll")   == INVALID_FILE_ATTRIBUTES &&
         GetFileAttributes (L"steam_api64.dll") == INVALID_FILE_ATTRIBUTES ) {
      wchar_t wszDeployedSteamAPI   [MAX_PATH] = { L'\0' };
      wchar_t wszDeployedSteamAPI64 [MAX_PATH] = { L'\0' };

      lstrcatW (wszDeployedSteamAPI,   SKIM_FindInstallPath (product.uiSteamAppID));
      lstrcatW (wszDeployedSteamAPI, L"\\steam_api.dll");

      lstrcatW (wszDeployedSteamAPI64, SKIM_FindInstallPath (product.uiSteamAppID));
      lstrcatW (wszDeployedSteamAPI64, L"\\steam_api64.dll");

      if ( GetFileAttributes (wszDeployedSteamAPI)   != INVALID_FILE_ATTRIBUTES ||
           GetFileAttributes (wszDeployedSteamAPI64) != INVALID_FILE_ATTRIBUTES ) {
        ShellExecute ( 0,
                         L"OPEN",
                           wszExecutable,
                             wszAppID,
                               wszInstallPath,
                                 SW_SHOWNORMAL );
      } else {
        wchar_t wszErrorMsg [512] = { L'\0' };

        _swprintf ( wszErrorMsg,
                      L"Unable to locate a valid install path (steam_api{64}.dll is missing)!\r\n\r\n"
                      L"Game is supposed to be installed to '%s', but is not.",
                        SKIM_FindInstallPath (product.uiSteamAppID) );

        MessageBox ( NULL,
                      wszErrorMsg,
                        L"Special K Installer Fatal Error",
                          MB_ICONHAND | MB_OK );
      }
      ExitProcess (0x00);
    }
  }

  SetCurrentDirectory (
    SKIM_FindInstallPath (product.uiSteamAppID)
  );

  extern HWND hWndMainDlg;
  ShowWindow (hWndMainDlg, SW_HIDE);

  SKIM_FetchInstallerDLL (product);

  wchar_t wszInstallerDLL [MAX_PATH] = { L'\0' };
  swprintf ( wszInstallerDLL,
              L"%s\\%s",
                wszInstallPath, product.wszWrapper );

  HMODULE hModInstaller =
    LoadLibrary (wszInstallerDLL);

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

  ExitProcess (0x00);
}



#include <Windows.h>
#include <WindowsX.h>
HWND hWndMainDlg;

bool
SKIM_DetermineInstallState (sk_product_t& product)
{
  // Special Case: SpecialK
#ifndef _WIN64
  if (! _wcsicmp (product.wszWrapper, L"SpecialK32.dll")) {
#else
  if (! _wcsicmp (product.wszWrapper, L"SpecialK64.dll")) {
#endif
             uint32_t dwLen = MAX_PATH;
    wchar_t wszSpecialK_ROOT [MAX_PATH] = { L'\0' };

    SKIM_GetDocumentsDir (wszSpecialK_ROOT, &dwLen);

    lstrcatW (wszSpecialK_ROOT, L"\\My Mods\\SpecialK\\");

    lstrcatW (wszSpecialK_ROOT, product.wszWrapper);

    if (GetFileAttributes (wszSpecialK_ROOT) != INVALID_FILE_ATTRIBUTES) {
      if (SKIM_IsEnabled32 () && SKIM_IsEnabled64 ()) {
        product.install_state = 1;
        return true;
      } else {
        product.install_state = 0;
        return false;
      }
    }

    else {
      product.install_state = 0;
      return false;
    }
  }

  wchar_t wszFileToTest [MAX_PATH] = { L'\0' };

  if (lstrlenW (product.wszPlugIn)) {
    swprintf ( wszFileToTest,
                 L"%s\\%s",
                   SKIM_FindInstallPath (product.uiSteamAppID),
                     product.wszPlugIn );
  } else {
      swprintf ( wszFileToTest,
               L"%s\\%s",
                 SKIM_FindInstallPath (product.uiSteamAppID),
                   product.wszWrapper );
  }

  const wchar_t* wszInstallPath =
    SKIM_FindInstallPath (product.uiSteamAppID);

  if (wszInstallPath == nullptr) {
    product.install_state = -1;
    return false;
  }

  if ( ! ( GetFileAttributes (
             SKIM_FindInstallPath (product.uiSteamAppID)
           ) & FILE_ATTRIBUTE_DIRECTORY
         )
     ) {
    product.install_state = -1;
    return false;
  }

  else
  {
    if (GetFileAttributes (wszFileToTest) != INVALID_FILE_ATTRIBUTES) {
      product.install_state = 1;
      return true;
    }

    product.install_state = 0;
    return false;
  }
}

std::wstring
SKIM_SummarizeRenderAPI (sk_product_t product)
{
  std::wstring ret = L"";

  if (! _wcsicmp (product.wszWrapper, L"d3d9.dll"))
    ret += L"Direct3D 9";
  else if (! _wcsicmp (product.wszWrapper, L"dxgi.dll")) {
    if (! _wcsicmp (product.wszRepoName, L"UnX"))
      ret += L"Direct3D 11";
    else if ( (! _wcsicmp (product.wszRepoName, L"FO4W")) ||
              (! _wcsicmp (product.wszRepoName, L"SoulsUnsqueezed")) )
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

void
SKIM_OnProductSelect (void)
{
  HWND hWndProducts =
    GetDlgItem (hWndMainDlg, IDC_PRODUCT_SELECT);

  int sel = ComboBox_GetCurSel (hWndProducts);

  Static_SetText (
    GetDlgItem (hWndMainDlg, IDC_NAME_OF_GAME),
      products [sel].wszGameName
  );

  Static_SetText (
    GetDlgItem (hWndMainDlg, IDC_API_SUMMARY),
      SKIM_SummarizeRenderAPI (products [sel]).c_str ()
  );

  HWND hWndDescrip =
    GetDlgItem (hWndMainDlg, IDC_PRODUCT_DETAILS);

  Edit_SetText (hWndDescrip, products [sel].wszDescription);


  HICON btn_icon = 0;

  // Easy way to detect Special K
  if (products [sel].uiSteamAppID == 0) {
    static HICON hIconShield = 0;

    if (hIconShield == 0) {
      SHSTOCKICONINFO sii = { 0 };
               sii.cbSize = sizeof (sii);

      SHGetStockIconInfo ( SIID_SHIELD,
                             SHGSI_ICON | SHGSI_LARGEICON,
                               &sii );

      hIconShield  = sii.hIcon;
    }

    btn_icon = hIconShield;
  }

  HWND hWndInstall =
    GetDlgItem (hWndMainDlg, IDC_INSTALL_CMD);

  HWND hWndManage =
    GetDlgItem (hWndMainDlg, IDC_MANAGE_CMD);

  HWND hWndUninstall =
    GetDlgItem (hWndMainDlg, IDC_UNINSTALL_CMD);

  SendMessage ( hWndInstall,
                  BM_SETIMAGE,
                    IMAGE_ICON,
                      (LPARAM)btn_icon
              );

  SendMessage ( hWndUninstall,
                  BM_SETIMAGE,
                    IMAGE_ICON,
                      (LPARAM)btn_icon
              );

  if (products [sel].install_state < 0) {
    Button_Enable (hWndInstall,   0);
    Button_Enable (hWndManage,    0);
    Button_Enable (hWndUninstall, 0);
  }

  else if (products [sel].install_state == 0) {
    Button_Enable (hWndInstall,   1);
    Button_Enable (hWndManage,    0);
    Button_Enable (hWndUninstall, 0);
  }

  else if (products [sel].install_state == 1) {
    Button_Enable (hWndInstall,   0);
    Button_Enable (hWndUninstall, 1);

    // Tales of Symphonia (has DLC)
    if (products [sel].uiSteamAppID == 372360)
      Button_Enable (hWndManage, 1);
    else
      Button_Enable (hWndManage, 0);
  }
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

  switch (uMsg) {
    case WM_INITDIALOG:
    {
      hWndMainDlg = hWndDlg;

      HWND hWndProducts =
        GetDlgItem (hWndDlg, IDC_PRODUCT_SELECT);

      ComboBox_SetCurSel    (hWndProducts, 1);
      ComboBox_ResetContent (hWndProducts);

      for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++) {
        ComboBox_InsertString (hWndProducts, i, products [i].wszProjectName);
      }

      ComboBox_SetCurSel   (hWndProducts, 0);
      SKIM_OnProductSelect ();

      return TRUE;
    }

    case WM_COMMAND:
    {
      HWND hWndProducts =
        GetDlgItem (hWndDlg, IDC_PRODUCT_SELECT);

      sk_product_t& product =
        products [ComboBox_GetCurSel (hWndProducts)];

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

        case IDC_INSTALL_CMD:
        {
          if (product.uiSteamAppID != 0) {
            _beginthreadex (
              nullptr,
                0,
                 SKIM_InstallProduct,
                   &product,
                     0x00,
                       nullptr );
          }

          else if (product.uiSteamAppID == 0) {
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
          if (product.uiSteamAppID == 372360) {
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
          //PostMessage (hWndMainDlg, WM_CLOSE, 0, 0);
        }  break;

        case IDC_UNINSTALL_CMD:
        {
          HWND hWndProducts =
            GetDlgItem (hWndDlg, IDC_PRODUCT_SELECT);

          sk_product_t& product =
            products [ComboBox_GetCurSel (hWndProducts)];

          SKIM_UninstallProduct (product, hWndDlg);
        } break;
      }
      return (INT_PTR)true;
    }

    case WM_CLOSE:
    case WM_DESTROY:
    {
      ExitProcess (0x00);
    }

    case WM_CREATE:
    case WM_PAINT:
    case WM_SIZE:
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

#include "winbase.h"

void
SKIM_DisableGlobalInjector (void)
{
  if (! SKIM_IsAdmin ()) {
    wchar_t wszExec [MAX_PATH] = { L'\0' };

    GetModuleFileName (GetModuleHandle (nullptr), wszExec, MAX_PATH);

    ShellExecute ( nullptr,
                     L"runas",
                       wszExec,
                         L"-1", nullptr,
                           SW_SHOWNORMAL );
    ExitProcess (0);
  }

  if (SKIM_IsEnabled32 ()) {
    SKIM_Enable32 (nullptr);
  }

  if (SKIM_IsEnabled64 ()) {
    SKIM_Enable64 (nullptr);
  }

  wchar_t wszRepo      [MAX_PATH];
  wchar_t wszInstalled [MAX_PATH];

  wcscpy (wszRepo,      SKIM_FindInstallPath (0));
  wcscpy (wszInstalled, SKIM_FindInstallPath (0));

  lstrcatW (wszInstalled, L"\\Version\\installed.ini");
  lstrcatW (wszRepo,      L"\\Version\\repository.ini");

  DeleteFileW (wszInstalled);
  DeleteFileW (wszRepo);
}

unsigned int
__stdcall
SKIM_InstallGlobalInjector (LPVOID user)
{
  HWND hWndParent = (HWND)user;

  if (! SKIM_IsAdmin ()) {
    wchar_t wszExec [MAX_PATH] = { L'\0' };

    GetModuleFileName (GetModuleHandle (nullptr), wszExec, MAX_PATH);

    ShellExecute ( (HWND)hWndParent,
                     L"runas",
                       wszExec,
                         L"0", nullptr,
                           SW_SHOWNORMAL );
    PostMessage ( (HWND)hWndParent, WM_CLOSE, 0x00, 0x00 );
    ExitProcess (0);
  }

  wchar_t wszDestDLL32 [MAX_PATH] = { L'\0' };
  wchar_t wszDestDLL64 [MAX_PATH] = { L'\0' };

  uint32_t dwStrLen = MAX_PATH;
  SKIM_GetDocumentsDir (wszDestDLL32, &dwStrLen);

  dwStrLen = MAX_PATH;
  SKIM_GetDocumentsDir (wszDestDLL64, &dwStrLen);

  lstrcatW (wszDestDLL32, L"\\My Mods\\SpecialK\\");
  lstrcatW (wszDestDLL64, L"\\My Mods\\SpecialK\\");

  // Create the destination directory
  SKIM_CreateDirectories (wszDestDLL32);

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
    lstrcatW (wszDestDLL32, L"SpecialK32.dll");
    lstrcatW (wszDestDLL64, L"SpecialK64.dll");

    sk_product_t sk32 =
    {
      L"SpecialK32.dll",
      L"",
      L"Special K",
      L"Special K (Global Injector)",
      L"SpecialK",
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
      L"Special K (Global Injector)",
      L"SpecialK",
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

    SKIM_FetchInstaller32  (sk32);
    SKIM_FetchInstaller64  (sk64);

    //SKIM_FetchInstallerDLL (products [0]);

    SKIM_Enable32 (wszDestDLL32);
    SKIM_Enable64 (wszDestDLL64);

    int               nButtonPressed = 0;
    TASKDIALOGCONFIG  config         = {0};

    config.cbSize             = sizeof (config);
    config.hInstance          = GetModuleHandle (nullptr);
    config.hwndParent         = hWndParent;
    config.pszWindowTitle     = L"Special K Install Manager";
    config.dwCommonButtons    = 0;//TDCBF_OK_BUTTON;
    config.pszMainInstruction = L"Installation of Global Injector Pending";
    config.pButtons           = nullptr;
    config.cButtons           = 0;
    config.pszContent         = L"Special K (Global Injector) will finalize its"
                                L" installation the next time you launch a game"
                                L" from Steam.\n\n"
                                L"For compatibility options, PRESS AND HOLD "
                                L"Ctrl + Shift before launching a game.";
    config.pszMainIcon        = TD_INFORMATION_ICON;

    TaskDialogIndirect (&config, &nButtonPressed, nullptr, nullptr);
  }

  PostMessage ((HWND)hWndParent, WM_CLOSE, 0x00, 0x00);
  return 0;
}

int
WINAPI
WinMain ( _In_     HINSTANCE hInstance,
          _In_opt_ HINSTANCE hPrevInstance,
          _In_     LPSTR     lpCmdLine,
          _In_     int       nCmdShow )
{
  INITCOMMONCONTROLSEX icex;
  ZeroMemory (&icex, sizeof INITCOMMONCONTROLSEX);

  icex.dwSize = sizeof INITCOMMONCONTROLSEX;
  icex.dwICC  = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;

  InitCommonControlsEx (&icex);

  LoadLibrary (L"Msftedit.dll");

  for (int i = 0; i < sizeof (products) / sizeof (sk_product_t); i++) {
    SKIM_DetermineInstallState (products [i]);
  }

  hWndMainDlg =
    CreateDialog ( hInstance,
                     MAKEINTRESOURCE (IDD_FRONTEND),
                       0,
                         Main_DlgProc );


  // We can install PlugIns by passing their AppID throug the cmdline
  if (strlen (lpCmdLine) != 0)
  {
    int32_t appid = atoi (lpCmdLine);

    sk_product_t* prod =
      SKIM_FindProductByAppID (appid);

    if (appid > 0) {
      if (prod != nullptr)
      {
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

    else {
      // Install Global Injector
      if (appid == 0) {
        _beginthreadex (
          nullptr,
            0,
              SKIM_InstallGlobalInjector,
                (LPVOID)hWndMainDlg,
                  0x00,
                    nullptr );
      }

      // Uninstall Global Injector
      else if (appid == -1) {
        SKIM_UninstallProduct (*SKIM_FindProductByAppID (0), hWndMainDlg);
        PostMessage           (hWndMainDlg, WM_CLOSE, 0x00, 0x00);
      }
    }
  }

  MSG  msg;
  BOOL bRet;

  while ((bRet = GetMessage (&msg, 0, 0, 0)) != 0)
  {
    if (bRet == -1) {
      return 0;
    }

    TranslateMessage (&msg);
    DispatchMessage  (&msg);
  }

UNREFERENCED_PARAMETER (nCmdShow);
UNREFERENCED_PARAMETER (lpCmdLine);
UNREFERENCED_PARAMETER (hPrevInstance);

  return 0;
}