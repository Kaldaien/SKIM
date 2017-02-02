#pragma once

extern const wchar_t*
SKIM_FindInstallPath (uint32_t appid);

class SKIM_BranchManager {
public:
  class Branch {
  public:
    std::wstring description;
    std::wstring name;
  };

  SKIM_BranchManager (void) {
    product_.reset ();
  }

  ~SKIM_BranchManager (void) {
  }

  static SKIM_BranchManager* singleton (void) {
    static SKIM_BranchManager* pMgr = nullptr;

    if (pMgr == nullptr)
      pMgr = new SKIM_BranchManager ();

    return pMgr;
  }

  void setProduct (uint32_t uiAppID) {
    if (product_.app_id == uiAppID)
      return;

    product_.app_id = uiAppID;
    product_.reset ();
  }

  std::wstring getInstallPackage (void) {
    return product_.install_package;
  }

  Branch* getBranch (const wchar_t* wszName) {
    return product_.getBranch (wszName);
  }

  Branch* getCurrentBranch (void) const {
    return product_.getCurrentBranch ();
  }

  Branch* getBranchByIndex (uint32_t idx) {
    if (getNumberOfBranches () > idx)
      return &product_.branches [idx];

    return nullptr;
  }

  uint32_t getNumberOfBranches (void) {
    return product_.getCount ();
  }

  bool migrateToBranch (const wchar_t* wszName) {
    return product_.migrateToBranch (wszName);
  }

protected:
  struct branch_list_s {
    void reset (void) {
      active_branch = nullptr;
      branches.clear ();

      if (installed_.ini != nullptr) {
        delete installed_.ini;
               installed_.ini = nullptr;
      }

      if (repo_.ini != nullptr) {
        delete repo_.ini;
               repo_.ini = nullptr;
      }

      if (app_id != UINT32_MAX) {
        const wchar_t* wszInstallPath =
          SKIM_FindInstallPath (app_id);

        wchar_t wszInstalledINI [MAX_PATH] = { L'\0' };
        wchar_t wszRepoINI      [MAX_PATH] = { L'\0' };

        lstrcatW (wszInstalledINI, wszInstallPath);
        lstrcatW (wszRepoINI,      wszInstallPath);

        lstrcatW (wszInstalledINI, L"\\Version\\installed.ini");
        lstrcatW (wszRepoINI,      L"\\Version\\repository.ini");

        installed_.path = wszInstalledINI;
        repo_.path      = wszRepoINI;

        installed_.ini = new iSK_INI (installed_.path.c_str ());
        repo_.ini      = new iSK_INI (repo_.path.c_str ());

        installed_.ini->parse ();
        repo_.ini->parse      ();

        auto repo_it =
          repo_.ini->get_sections ().begin ();

        while (repo_it != repo_.ini->get_sections ().end ())
        {
          // Find any section named Version.*
          if ( wcsstr (repo_it->second.name.c_str (), L"Version.") ==
                 repo_it->second.name.c_str () ) {
            Branch branch;
            branch.name = repo_it->second.name.c_str () + 8;

            if (branch.name == L"Latest" || branch.name == L"Invalid")
              branch.name = L"Main";

            branch.description =
              ((iSK_INISection &)repo_it->second).get_value (L"BranchDescription");

            if (branch.description == L"Invalid")
              branch.description = L"No Description Available?!";

            branches.push_back (branch);
          }

          ++repo_it;
        }

        std::wstring installed_branch =
          installed_.ini->get_section (L"Version.Local").get_value (L"Branch");

        install_package =
          installed_.ini->get_section (L"Version.Local").get_value (L"InstallPackage");

        if (installed_branch == L"Latest" || installed_branch == L"Invalid")
          installed_branch = L"Main";

        active_branch = getBranch (installed_branch.c_str ());
      }
    }

    Branch* getBranch (const wchar_t* wszName) {
      int  idx = 0;
      auto it  = branches.begin ();

      while (it != branches.end ())
      {
        if (! _wcsicmp (wszName, it->name.c_str ()))
          return &(*it);

        ++idx, ++it;
      }

      return nullptr;
    }

    Branch* getCurrentBranch (void) const {
      return active_branch;
    }

    uint32_t getCount (void) {
      return (uint32_t)branches.size ();
    }

    bool migrateToBranch (const wchar_t* wszName)
    {
      Branch* pBranch = getBranch (wszName);

      if (! pBranch)
        return false;

      if (active_branch == pBranch)
        return false;

      try {
        iSK_INI* installed =
          installed_.ini;

        iSK_INISection& sec =
          installed->get_section (L"Version.Local");

        std::wstring& ini_branch =
          sec.get_value (L"Branch");


        // A little bit of trickery since I stupidly reserved the
        //   Latest branch before I realized what that implied
        if (! wcscmp (wszName, L"Main"))
          ini_branch = L"Latest";
        else
          ini_branch = wszName;

        sec.remove_key    (L"InstallPackage");
        sec.add_key_value (L"InstallPackage", L"PendingBranchMigration,0");

        iSK_INISection& update_sec =
          installed->get_section (L"Update.User");

        if (update_sec.contains_key (L"Reminder"))
          update_sec.get_value (L"Reminder") = L"0";
        else
          update_sec.add_key_value (L"Reminder", L"0");

        installed->write (installed_.path.c_str ());
      } catch (...) {
        return false;
      }

      return true;
    }

    std::wstring         install_package = L"";
    Branch*              active_branch   = nullptr;
    std::vector <Branch> branches;
    uint32_t             app_id;

  private:
    struct {
      iSK_INI*     ini  = nullptr;
      std::wstring path = L"";
    }  installed_,
       repo_;
  } product_;
};