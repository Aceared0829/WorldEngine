#include <Foundation/Configuration/Plugin.h>

using WPluginModule = void*;

bool WPlugin::PlatformNeedsPluginCopy()
{
  W_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void WPlugin::GetPluginPaths(WStringView sPluginName, WStringBuilder& sOriginalFile, WStringBuilder& sCopiedFile, WUInt8 uiFileCopyNumber)
{
  W_ASSERT_NOT_IMPLEMENTED;
}

WResult UnloadPluginModule(WPluginModule& Module, WStringView sPluginFile)
{
  W_ASSERT_NOT_IMPLEMENTED;

  return W_FAILURE;
}

WResult LoadPluginModule(WStringView sFileToLoad, WPluginModule& Module, WStringView sPluginFile)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return W_FAILURE;
}
