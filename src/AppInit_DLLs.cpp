#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_NON_CONFORMING_SWPRINTFS
#pragma warning (disable: 4091)

#include "stdafx.h"

#include "AppInit_DLLs.h"

#include <vector>

BOOL
SKIM_ContainsDLL (const wchar_t* wszOriginal, const wchar_t* wszDLL)
{
  if (wszOriginal == nullptr)
    return FALSE;

  std::vector <std::wstring> dlls;

  wchar_t* wszMap  = _wcsdup (wszOriginal);
  wchar_t* wszTok  =  wcstok (wszMap, L", ");

  if (wszTok != nullptr) {
    while (wszTok != nullptr) {
      dlls.push_back (wszTok);

      wszTok = wcstok (nullptr, L", ");
    }
  } else {
    dlls.push_back (wszOriginal);
  }

  free (wszMap);

  std::wstring out;

  while (! dlls.empty ()) {
    if (wcsstr (dlls.back ().c_str (), wszDLL))
      return TRUE;

    dlls.pop_back ();
  }

  return FALSE;
}

std::wstring
SKIM_AddDLL (const wchar_t* wszOriginal, const wchar_t* wszNew)
{
  if (wszOriginal == nullptr)
    return wszNew;

  std::vector <std::wstring> dlls;

  wchar_t* wszMap  = _wcsdup (wszOriginal);
  wchar_t* wszTok  =  wcstok (wszMap, L", ");

  if (wszTok != nullptr) {
    while (wszTok != nullptr) {
      dlls.push_back (wszTok);

      wszTok = wcstok (nullptr, L", ");
    }
  } else {
    dlls.push_back (wszOriginal);
  }

  free (wszMap);

  std::wstring out;

  while (! dlls.empty ()) {
    out += dlls.back ();
    out += L" ";

    dlls.pop_back ();
  }

  out += wszNew;

  return out;
}

std::wstring
SKIM_RemoveDLL (const wchar_t* wszOriginal, const wchar_t* wszRemove)
{
  if (wszOriginal == nullptr)
    return L"";

  std::vector <std::wstring> dlls;

  wchar_t* wszMap  = _wcsdup (wszOriginal);
  wchar_t* wszTok  =  wcstok (wszMap, L", ");

  if (wszTok != nullptr) {
    while (wszTok != nullptr) {
      dlls.push_back (wszTok);

      wszTok = wcstok (nullptr, L", ");
    }
  } else {
    dlls.push_back (wszOriginal);
  }

  free (wszMap);

  std::wstring out;

  while (! dlls.empty ()) {
    if (! wcsstr (dlls.back ().c_str (), wszRemove)) {
      out += dlls.back ();
      out += L" ";
    }

    dlls.pop_back ();
  }

  return out;
}

wchar_t wszAppInit_DLLs32 [MAX_PATH] = { L"32" };

const wchar_t*
SKIM_GetAppInitDLLs32 (void)
{
  DWORD   len            = MAX_PATH * sizeof (wchar_t);

  LSTATUS status = ERROR;
  HKEY    key    = nullptr;

  status =
    RegOpenKeyExW ( HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                      L"Windows",
                          0x00,
                            KEY_WOW64_32KEY | KEY_READ | KEY_WRITE,
                              &key );

  status =
    RegGetValueW ( key,
                     nullptr,
                       L"AppInit_DLLs",
                         RRF_RT_REG_SZ,
                           0x0,
                             wszAppInit_DLLs32,
                               &len );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS) {
    return wszAppInit_DLLs32;
  }
  else
    return nullptr;
}

void
SKIM_SetAppInitDLLs32 (const wchar_t* wszAppInit_DLLs)
{
  DWORD len = wcslen (wszAppInit_DLLs);

  LSTATUS status        = ERROR;
  HKEY    key           = nullptr;
  DWORD   dwDisposition = 0x00;

  status =
    RegCreateKeyExW ( HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                        L"Windows",
                          0, NULL, 0x00L,
                            KEY_WOW64_32KEY | KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  status =
    RegSetValueExW ( key,
                       L"AppInit_DLLs",
                         0,
                           REG_SZ,
                             (BYTE *)wszAppInit_DLLs,
                               len * sizeof (wchar_t) );

  RegFlushKey (key);
  RegCloseKey (key);
}

int
SKIM_GetLoadAppInitDLLs32 (void)
{
  int   load = 0;
  DWORD len  = sizeof (int);

  LSTATUS status = ERROR;

  HKEY    key    = nullptr;

  status =
    RegOpenKeyExW ( HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                      L"Windows",
                          0x00,
                            KEY_WOW64_32KEY | KEY_READ | KEY_WRITE,
                              &key );
  status =
    RegGetValueW ( key,
                     nullptr,
                       L"LoadAppInit_DLLs",
                         RRF_RT_ANY,
                           NULL,
                             &load,
                               (LPDWORD)&len );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS)
    return load;
  else
    return 0;
}

BOOL
SKIM_SetLoadAppInitDLLs32 (BOOL load)
{
  LSTATUS status        = ERROR;
  HKEY    key           = nullptr;
  DWORD   dwDisposition = 0x00;

  status =
    RegCreateKeyExW ( HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                        L"Windows",
                          0, NULL, 0x00L,
                            KEY_WOW64_32KEY | KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  status =
    RegSetValueExW ( key,
                       L"LoadAppInit_DLLs",
                         0,
                           REG_DWORD,
                             (BYTE *)&load,
                               sizeof (BOOL) );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS)
    return load;
  else
    return 0;
}

BOOL
SKIM_IsEnabled32 (void)
{
  return ( SKIM_GetLoadAppInitDLLs32 () != 0x00 &&
           SKIM_ContainsDLL (
             SKIM_GetAppInitDLLs32 (),
               L"SpecialK32.dll" )
         );
}

BOOL
SKIM_Enable32 (const wchar_t* wszPathToDLL)
{
  if (wszPathToDLL != nullptr) {
    SKIM_SetLoadAppInitDLLs32 (0x1);
    SKIM_SetAppInitDLLs32     ( SKIM_AddDLL (
                                  SKIM_RemoveDLL (
                                    SKIM_GetAppInitDLLs32 (), L"SpecialK32.dll"
                                  ).c_str (),
                                  wszPathToDLL).c_str ()
                              );
  } else {
    SKIM_SetAppInitDLLs32     (SKIM_RemoveDLL (SKIM_GetAppInitDLLs32 (), L"SpecialK32.dll").c_str ());
  }

  return wszPathToDLL != nullptr;
}

wchar_t wszAppInit_DLLs64 [MAX_PATH] = { L"64" };

const wchar_t*
SKIM_GetAppInitDLLs64 (void)
{
  DWORD len = MAX_PATH * sizeof (wchar_t);

  LSTATUS status = ERROR;
  HKEY    key    = nullptr;

  status =
    RegOpenKeyExW ( HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                      L"Windows",
                          0x00,
                            KEY_WOW64_64KEY | KEY_READ | KEY_WRITE,
                              &key );

  status =
    RegGetValueW ( key,
                     nullptr,
                       L"AppInit_DLLs",
                         RRF_RT_REG_SZ,
                           0x0,
                             wszAppInit_DLLs64,
                               &len );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS) {
    return wszAppInit_DLLs64;
  }
  else
    return nullptr;
}

void
SKIM_SetAppInitDLLs64 (const wchar_t* wszAppInit_DLLs)
{
  DWORD len = wcslen (wszAppInit_DLLs);

  LSTATUS status        = ERROR;
  HKEY    key           = nullptr;
  DWORD   dwDisposition = 0x00;

  status =
    RegCreateKeyExW ( HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                        L"Windows",
                          0, NULL, 0x00L,
                            KEY_WOW64_64KEY | KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  status =
    RegSetValueExW ( key,
                       L"AppInit_DLLs",
                         0,
                           REG_SZ,
                             (BYTE *)wszAppInit_DLLs,
                               len * sizeof (wchar_t) );

  RegFlushKey (key);
  RegCloseKey (key);
}

int
SKIM_GetLoadAppInitDLLs64 (void)
{
  int   load = 0;
  DWORD len  = sizeof (int);

  LSTATUS status = ERROR;

  HKEY    key    = nullptr;

  status =
    RegOpenKeyExW ( HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                      L"Windows",
                          0x00,
                            KEY_WOW64_64KEY | KEY_READ | KEY_WRITE,
                              &key );
  status =
    RegGetValueW ( key,
                     nullptr,
                       L"LoadAppInit_DLLs",
                         RRF_RT_ANY,
                           NULL,
                             &load,
                               (LPDWORD)&len );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS)
    return load;
  else
    return 0;
}

BOOL
SKIM_SetLoadAppInitDLLs64 (BOOL load)
{
  LSTATUS status        = ERROR;
  HKEY    key           = nullptr;
  DWORD   dwDisposition = 0x00;

  status =
    RegCreateKeyExW ( HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
                        L"Windows",
                          0, NULL, 0x00L,
                            KEY_WOW64_64KEY | KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  status =
    RegSetValueExW ( key,
                       L"LoadAppInit_DLLs",
                         0,
                           REG_DWORD,
                             (BYTE *)&load,
                               sizeof (BOOL) );

  RegFlushKey (key);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS)
    return load;
  else
    return 0;
}

BOOL
SKIM_IsEnabled64 (void)
{
  return ( SKIM_GetLoadAppInitDLLs64 () != 0x00 &&
           SKIM_ContainsDLL (
             SKIM_GetAppInitDLLs64 (),
               L"SpecialK64.dll" )
         );
}

BOOL
SKIM_Enable64 (const wchar_t* wszPathToDLL)
{
  if (wszPathToDLL != nullptr) {
    SKIM_SetLoadAppInitDLLs64 (0x1);
    SKIM_SetAppInitDLLs64     ( SKIM_AddDLL (
                                  SKIM_RemoveDLL (
                                    SKIM_GetAppInitDLLs64 (), L"SpecialK64.dll"
                                  ).c_str (),
                                  wszPathToDLL
                                ).c_str ()
                              );
  } else {
    SKIM_SetAppInitDLLs64     (SKIM_RemoveDLL (SKIM_GetAppInitDLLs64 (), L"SpecialK64.dll").c_str ());
  }

  return wszPathToDLL != nullptr;
}