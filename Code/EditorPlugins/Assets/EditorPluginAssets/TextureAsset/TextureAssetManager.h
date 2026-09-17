#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

class W_EDITORPLUGINASSETS_DLL WTextureAssetProfileConfig : public WProfileConfigData
{
  W_ADD_DYNAMIC_REFLECTION(WTextureAssetProfileConfig, WProfileConfigData);

public:
  WUInt16 m_uiMaxResolution = 1024 * 16;
};

class WTextureAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WTextureAssetDocumentManager, WAssetDocumentManager);

public:
  WTextureAssetDocumentManager();
  ~WTextureAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return WAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const override;

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }
  virtual void AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix = "\n"_wsv) const override;

  virtual WString GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDescriptor, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const override;

private:
  WAssetDocumentTypeDescriptor m_DocTypeDesc;
  WAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
