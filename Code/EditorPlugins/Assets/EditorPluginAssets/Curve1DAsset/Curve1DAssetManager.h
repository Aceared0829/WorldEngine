#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WCurve1DAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WCurve1DAssetDocumentManager, WAssetDocumentManager);

public:
  WCurve1DAssetDocumentManager();
  ~WCurve1DAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return WAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
