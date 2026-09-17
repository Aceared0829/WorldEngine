#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/Process.h>

using WPluginModule = void*;

bool WPlugin::PlatformNeedsPluginCopy()
{
  return false;
}

void WPlugin::GetPluginPaths(WStringView sPluginName, WStringBuilder& sOriginalFile, WStringBuilder& sCopiedFile, WUInt8 uiFileCopyNumber)
{
  sOriginalFile = WOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = WOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

WResult UnloadPluginModule(WPluginModule& Module, WStringView sPluginFile)
{
  if (dlclose(Module) != 0)
  {
    WStringBuilder tmp;
    WLog::Error("Could not unload plugin '{0}'. Error {1}", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult LoadPluginModule(WStringView sFileToLoad, WPluginModule& Module, WStringView sPluginFile)
{
  WStringBuilder tmp;
  Module = dlopen(sFileToLoad.GetData(tmp), RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    WLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return W_FAILURE;
  }
  return W_SUCCESS;
}
