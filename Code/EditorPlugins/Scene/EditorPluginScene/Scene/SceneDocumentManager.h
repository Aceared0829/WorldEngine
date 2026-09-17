#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class WSceneDocumentManager : public WAssetDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WSceneDocumentManager, WAssetDocumentManager);

public:
  WSceneDocumentManager();

  virtual WResult OpenPickedDocument(const WDocumentObject* pPickedComponent, WUInt32 uiPartIndex) override;

private:
  virtual void InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const override;
  virtual void InternalCloneDocument(WStringView sPath, WStringView sClonePath, const WUuid& documentId, const WUuid& seedGuid, const WUuid& cloneGuid, WAbstractObjectGraph* pHeader, WAbstractObjectGraph* pObjects, WAbstractObjectGraph* pTypes) override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  void SetupDefaultScene(WDocument* pDocument);


  WStaticArray<WAssetDocumentTypeDescriptor, 4> m_DocTypeDescs;
};
