#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct WRmlUiResourceDescriptor;

class WRmlUiAssetDocument : public WSimpleAssetDocument<WRmlUiAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiAssetDocument, WSimpleAssetDocument<WRmlUiAssetProperties>);

public:
  WRmlUiAssetDocument(WStringView sDocumentPath);

  void OpenExternalEditor();

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  WStatus FindDependencies(WDependencyFile& ref_Dependencies, WStringView sFilePath) const;
  void FindPackageDependencies(WSet<WString>& ref_packageDeps, WStringView sFilePath, WSet<WString>& ref_visited) const;

private:
  WStatus FindDependencies(WDependencyFile& ref_Dependencies, WStringView sFilePath, WSet<WString>& ref_visited) const;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};
