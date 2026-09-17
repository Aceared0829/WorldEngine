#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WSubstancePackageAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WSubstancePackageAssetDocumentManager, WAssetDocumentManager);

public:
  WSubstancePackageAssetDocumentManager();
  ~WSubstancePackageAssetDocumentManager();

  const WAssetDocumentTypeDescriptor& GetTextureTypeDesc() const { return m_TextureTypeDesc; }

private:
  virtual void FillOutSubAssetList(const WAssetDocumentInfo& assetInfo, WDynamicArray<WSubAssetData>& out_subAssets) const override;
  virtual WString GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const override;
  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const override { return 1; }

  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  WAssetDocumentTypeDescriptor m_PackageTypeDesc;
  WAssetDocumentTypeDescriptor m_TextureTypeDesc;
};
