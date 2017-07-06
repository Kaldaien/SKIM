#pragma once

enum SK_ARCHITECTURE {
  SK_32_BIT   = 0x01,
  SK_64_BIT   = 0x02,

  SK_BOTH_BIT = 0x03
};

struct sk_product_t {
  wchar_t         wszWrapper        [MAX_PATH];
  wchar_t         wszPlugIn         [64];
  wchar_t         wszDLLProductName [128];
  wchar_t         wszGameName       [128];
  wchar_t         wszProjectName    [128];
  wchar_t         wszRepoName       [32];
  wchar_t         wszDonateID       [16];
  uint32_t        uiSteamAppID;
  SK_ARCHITECTURE architecture;
  wchar_t*        wszDescription;

  int32_t         install_state; // To be filled-in later
};