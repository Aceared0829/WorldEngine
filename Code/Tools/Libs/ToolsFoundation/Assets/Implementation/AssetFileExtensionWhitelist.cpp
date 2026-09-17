#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

WMap<WString, WSet<WString>> WAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void WAssetFileExtensionWhitelist::AddAssetFileExtension(WStringView sAssetType, WStringView sAllowedFileExtension)
{
  WStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  WStringBuilder sLowerExt = sAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool WAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(WStringView sAssetType, WStringView sFile)
{
  WStringBuilder sLowerExt = WPathUtils::GetFileExtension(sFile);
  sLowerExt.ToLower();

  WStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  WTempHybridArray<WString, 16> Types;
  sLowerType.Split(false, Types, ";");

  for (const auto& filter : Types)
  {
    if (s_ExtensionWhitelist[filter].Contains(sLowerExt))
      return true;
  }

  return false;
}

const WSet<WString>& WAssetFileExtensionWhitelist::GetAssetFileExtensions(WStringView sAssetType)
{
  return s_ExtensionWhitelist[sAssetType];
}
