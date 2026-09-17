#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class WVisualGraphPin;

class WProcGenGraphAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenGraphAssetProperties, WReflectedClass);

public:
  WString m_sDebugPrefab;
  WString m_sDebugColorGradient;
  WString m_sDebugSurface;
  float m_fDebugFootprint = 1.0f;
  float m_fDebugAlignToNormal = 1.0f;
  WEnum<WProcPlacementPattern> m_DebugPlacementPattern = WProcPlacementPattern::RegularGrid;
};

class WProcGenGraphAssetDocument : public WAssetDocument
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenGraphAssetDocument, WAssetDocument);

public:
  WProcGenGraphAssetDocument(WStringView sDocumentPath);

  void SetDebugPin(const WVisualGraphPin* pDebugPin);
  void UpdateDebugNode();

  WStatus WriteAsset(WStreamWriter& inout_stream, const WPlatformProfile* pAssetProfile, bool bAllowDebug) const;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;

  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(WDynamicArray<const WDocumentObject*>& placementNodes, WDynamicArray<const WDocumentObject*>& vertexColorNodes) const;

private:
  friend class WProcGenAction;

  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const override;

  struct GenerateContext;

  WExpressionAST::Node* GenerateExpressionAST(const WDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, WExpressionAST& out_Ast) const;
  WExpressionAST::Node* GenerateDebugExpressionAST(GenerateContext& context, WExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  const WVisualGraphPin* m_pDebugPin = nullptr;
  WUniquePtr<WProcGen_PlacementOutput> m_pDebugNode;
};
