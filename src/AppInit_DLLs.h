#pragma once

#include <string>

BOOL
SKIM_ContainsDLL (const wchar_t* wszOriginal, const wchar_t* wszDLL);

std::wstring
SKIM_AddDLL (const wchar_t* wszOriginal, const wchar_t* wszNew);

std::wstring
SKIM_RemoveDLL (const wchar_t* wszOriginal, const wchar_t* wszRemove);

const wchar_t*
SKIM_GetAppInitDLLs32 (void);

void
SKIM_SetAppInitDLLs32 (const wchar_t* wszAppInit_DLLs);

int
SKIM_GetLoadAppInitDLLs32 (void);

BOOL
SKIM_SetLoadAppInitDLLs32 (BOOL load);

BOOL
SKIM_IsEnabled32 (void);

BOOL
SKIM_Enable32 (const wchar_t* wszPathToDLL);

const wchar_t*
SKIM_GetAppInitDLLs64 (void);

void
SKIM_SetAppInitDLLs64 (const wchar_t* wszAppInit_DLLs);

int
SKIM_GetLoadAppInitDLLs64 (void);

BOOL
SKIM_SetLoadAppInitDLLs64 (BOOL load);

BOOL
SKIM_IsEnabled64 (void);

BOOL
SKIM_Enable64 (const wchar_t* wszPathToDLL);