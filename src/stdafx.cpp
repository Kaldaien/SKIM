// stdafx.cpp : source file that includes just the standard includes
// tsfix_injector.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

timeGetTime_pfn             timeGetTime             = nullptr;
VerQueryValueW_pfn          VerQueryValueW          = nullptr;
GetFileVersionInfoExW_pfn   GetFileVersionInfoExW   = nullptr;
MsiEnumRelatedProductsW_pfn MsiEnumRelatedProductsW = nullptr;
MsiQueryProductStateW_pfn   MsiQueryProductStateW   = nullptr;