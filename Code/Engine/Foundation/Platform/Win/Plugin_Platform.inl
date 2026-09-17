#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringBuilder.h>

using WPluginModule = HMODULE;

bool WPlugin::PlatformNeedsPluginCopy()
{
  return true;
}

void WPlugin::GetPluginPaths(WStringView sPluginName, WStringBuilder& ref_sOriginalFile, WStringBuilder& ref_sCopiedFile, WUInt8 uiFileCopyNumber)
{
  ref_sOriginalFile = WOSFile::GetApplicationDirectory();
  ref_sOriginalFile.AppendPath(sPluginName);
  ref_sOriginalFile.Append(".dll");

  ref_sCopiedFile = WOSFile::GetApplicationDirectory();
  ref_sCopiedFile.AppendPath(sPluginName);

  if (!WOSFile::ExistsFile(ref_sOriginalFile))
  {
    ref_sOriginalFile = WOSFile::GetCurrentWorkingDirectory();
    ref_sOriginalFile.AppendPath(sPluginName);
    ref_sOriginalFile.Append(".dll");

    ref_sCopiedFile = WOSFile::GetCurrentWorkingDirectory();
    ref_sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
    ref_sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  ref_sCopiedFile.Append(".loaded");
}

WResult UnloadPluginModule(WPluginModule& ref_pModule, WStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  if (FreeLibrary(ref_pModule) == FALSE)
  {
    WLog::Error("Could not unload plugin '{0}'. Error-Code {1}", sPluginFile, WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }

  ref_pModule = nullptr;
  return W_SUCCESS;
}

WResult LoadPluginModule(WStringView sFileToLoad, WPluginModule& ref_pModule, WStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  ref_pModule = LoadLibraryW(WStringWChar(sFileToLoad).GetData());

  if (ref_pModule == nullptr)
  {
    const DWORD err = GetLastError();
    WLog::Error("Could not load plugin '{0}'. Error-Code {1}", sPluginFile, WArgErrorCode(err));

    if (err == 126)
    {
      WLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd "
                   "party DLLs next to the plugin.");
    }

    return W_FAILURE;
  }

  return W_SUCCESS;
}
