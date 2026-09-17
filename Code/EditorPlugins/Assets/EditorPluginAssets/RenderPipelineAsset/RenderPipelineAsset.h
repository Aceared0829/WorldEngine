#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

struct WAssetCuratorEvent;

/// Declares the type of pin to prevent connecting textures to buffers.
struct WRenderPipelineResourceType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Texture,
    Buffer,
    Default = Texture,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WRenderPipelineResourceType);


/// Stored inside WRenderPipelineAssetMetaData to declare an input / output pin of a sub-graph.
struct WRenderPipelineAssetPinInfo
{
  WEnum<WRenderPipelineResourceType> m_ResourceType;
  WString m_sName;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WRenderPipelineAssetPinInfo);


// Metadata attached to a WRenderPipelineAssetDocument to declare a sub-graph's input / output.
class WRenderPipelineAssetMetaData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineAssetMetaData, WReflectedClass);

public:
  WDynamicArray<WRenderPipelineAssetPinInfo> m_Inputs;
  WDynamicArray<WRenderPipelineAssetPinInfo> m_Outputs;
};


/// Custom pin class so that InternalCanConnect can prevent connecting textures to buffers.
class WRenderPipelineNodeGraphPin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineNodeGraphPin, WVisualGraphPin);

public:
  WRenderPipelineNodeGraphPin(WVisualGraphPin::Type type, const char* szName, const WColorGammaUB& color, const WDocumentObject* pObject, WRenderPipelineResourceType::Enum resourceType);
  ~WRenderPipelineNodeGraphPin();

  WRenderPipelineResourceType::Enum m_ResourceType = WRenderPipelineResourceType::Texture;
};


/// Object manager for render pipeline graphs.
///
/// Manages the node graph that defines a rendering pipeline, including render passes, resources, and their connections.
/// Validates connections to ensure render pipeline integrity.
class WRenderPipelineNodeManager : public WVisualGraphObjectManager
{
public:
  WRenderPipelineNodeManager();
  ~WRenderPipelineNodeManager();

  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override;

  virtual WStatus InternalCanAdd(const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const override;
  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const override;

  virtual bool InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const override;

private:
  struct SubGraphCache
  {
    const WDocumentObject* m_pObject = nullptr;
    WUuid m_SourceAssetGuid;
    WUInt64 m_uiMetaDataHash = 0;
  };

  void AssetCuratorEventHandler(const WAssetCuratorEvent& e);
  void NodeEventHandler(const WVisualGraphObjectManagerEvent& e);

  WMap<WUuid, SubGraphCache> m_SubGraphs;
};


class WRenderPipelineAssetDocument : public WAssetDocument
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineAssetDocument, WAssetDocument);

public:
  WRenderPipelineAssetDocument(WStringView sDocumentPath);
  ~WRenderPipelineAssetDocument();

protected:
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  // Copy & Paste support
  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const override;
  virtual bool Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;

  WStatus Validate() const;

  // meta data stores node positions and the pipeline's input/output interface
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;
};
