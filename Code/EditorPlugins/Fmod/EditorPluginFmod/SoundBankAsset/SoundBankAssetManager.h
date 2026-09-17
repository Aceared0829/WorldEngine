#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WSimpleFmod;

class WSoundBankAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WSoundBankAssetDocumentManager, WAssetDocumentManager);

public:
  WSoundBankAssetDocumentManager();
  ~WSoundBankAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return WAssetDocumentManager::OutputReliability::Perfect; }

  virtual void FillOutSubAssetList(const WAssetDocumentInfo& assetInfo, WDynamicArray<WSubAssetData>& out_subAssets) const override;
  virtual WString GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const override;

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);
  WString GetSoundBankAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const;

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const override;

  WAssetDocumentTypeDescriptor m_DocTypeDesc;
  WUniquePtr<WSimpleFmod> m_pFmod;

  struct SoundBankCache
  {
    WTimestamp m_LastModification;
    WDynamicArray<WSubAssetData> m_CachedSubAssets;
  };

  mutable WMap<WUuid, SoundBankCache> m_Cache;
};
