#pragma once

#include <Windows.h>
#include "product.h"

bool __stdcall
SKIM_FetchDLCManagerDLL (  sk_product_t  product,
                           const wchar_t *wszRemoteFile =
                           #ifndef _WIN64
L"installer.dll"
                           #else
L"installer64.dll"
                           #endif
);

bool __stdcall
SKIM_FetchInstallerDLL (  sk_product_t  product,
                         const wchar_t *wszRemoteFile =
                         #ifndef _WIN64
L"installer.dll"
                         #else
L"installer64.dll"
                         #endif
);

bool __stdcall
SKIM_FetchInjectorDLL (  sk_product_t  product,
                         const wchar_t *wszRemoteFile =
                         #ifndef _WIN64
L"injector.dll"
                         #else
L"injector64.dll"
                         #endif
                                     );

DWORD __stdcall HeaderThread (LPVOID user);

bool __stdcall SKIM_FetchInstaller32 (sk_product_t product);
bool __stdcall SKIM_FetchInstaller64 (sk_product_t product);

bool __stdcall SKIM_FetchInjector32  (sk_product_t product);
bool __stdcall SKIM_FetchInjector64  (sk_product_t product);