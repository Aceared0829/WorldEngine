#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class WVisualScriptClassAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptClassAssetProperties, WReflectedClass);

public:
  WString m_sBaseClass;
  WDynamicArray<WVisualScriptVariable> m_Variables;
  bool m_bDumpAST;
};

class WVisualScriptClassAssetDocument : public WSimpleAssetDocument<WVisualScriptClassAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptClassAssetDocument, WSimpleAssetDocument<WVisualScriptClassAssetProperties>);

public:
  WVisualScriptClassAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;
};
