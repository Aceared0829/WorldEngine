#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WDecalAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WDecalAssetDocumentManager, WAssetDocumentManager);

public:
  WDecalAssetDocumentManager();
  ~WDecalAssetDocumentManager();

  virtual void AddEntriesToAssetTable(
    WStringView sDataDirectory, const WPlatformProfile* pAssetProfile, WDelegate<void(WStringView sGuid, WStringView sPath, WStringView sType)> addEntry) const override;
  virtual WString GetAssetTableEntry(
    const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const override;

  /// There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  WStatus GenerateDecalTexture(const WPlatformProfile* pAssetProfile);
  WString GetDecalTexturePath(const WPlatformProfile* pAssetProfile) const;

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);
  bool IsDecalTextureUpToDate(const char* szDecalFile, WUInt64 uiAssetHash, WUInt16 uiAssetVersion) const;
  WStatus RunTexConv(const char* szTargetFile, const char* szInputFile, const WAssetFileHeader& AssetHeader);

  virtual void InternalCreateDocument(
    WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const override;

  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
