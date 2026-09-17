#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WKrautTreeAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeAssetDocumentManager, WAssetDocumentManager);

public:
  WKrautTreeAssetDocumentManager();
  ~WKrautTreeAssetDocumentManager();

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
