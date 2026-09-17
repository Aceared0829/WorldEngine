#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WJoltCollisionMeshAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshAssetDocumentManager, WAssetDocumentManager);

public:
  WJoltCollisionMeshAssetDocumentManager();
  ~WJoltCollisionMeshAssetDocumentManager();

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }
  virtual void AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix = "\n"_wsv) const override;

  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const override;

  WAssetDocumentTypeDescriptor m_DocTypeDesc;
  WAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
