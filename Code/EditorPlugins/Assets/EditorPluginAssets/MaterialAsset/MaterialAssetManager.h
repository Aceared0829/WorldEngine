#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WMaterialAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialAssetDocumentManager, WAssetDocumentManager);

public:
  WMaterialAssetDocumentManager();
  ~WMaterialAssetDocumentManager();

  virtual WString GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDescriptor, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const override;
  virtual bool IsOutputUpToDate(WStringView sDocumentPath, WStringView sOutputTag, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor) override;
  virtual WStringView GetOutputDocumentType(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sOutputTag, const WPlatformProfile* pAssetProfile = nullptr) const override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
