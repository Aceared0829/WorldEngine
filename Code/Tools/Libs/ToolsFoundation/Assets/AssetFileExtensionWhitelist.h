#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A global whitelist for file extension that may be used as certain asset types
///
/// UI elements etc. may use this whitelist to detect whether a selected file is a valid candidate for an asset slot
class W_TOOLSFOUNDATION_DLL WAssetFileExtensionWhitelist
{
public:
  static void AddAssetFileExtension(WStringView sAssetType, WStringView sAllowedFileExtension);

  static bool IsFileOnAssetWhitelist(WStringView sAssetType, WStringView sFile);

  static const WSet<WString>& GetAssetFileExtensions(WStringView sAssetType);

private:
  static WMap<WString, WSet<WString>> s_ExtensionWhitelist;
};
