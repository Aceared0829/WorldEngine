#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WCustomDataAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WCustomDataAssetDocumentManager, WAssetDocumentManager);

public:
  WCustomDataAssetDocumentManager();
  ~WCustomDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override
  {
    // CustomData structs are typically defined in plugins, which may have changed, so they are a candidate for clearing them from the asset cache
    return WAssetDocumentManager::OutputReliability::Unknown;
  }

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
