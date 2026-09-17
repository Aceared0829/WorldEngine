#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class WImageDataAssetDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WImageDataAssetDocumentManager, WAssetDocumentManager);

public:
  WImageDataAssetDocumentManager();
  ~WImageDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return WAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const WDocumentManager::Event& e);

  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  WAssetDocumentTypeDescriptor m_DocTypeDesc;
};
