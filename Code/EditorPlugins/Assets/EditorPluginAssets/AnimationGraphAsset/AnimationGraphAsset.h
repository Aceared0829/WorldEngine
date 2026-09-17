#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

using WAnimationClipResourceHandle = WTypedResourceHandle<class WAnimationClipResource>;

class WAnimGraphInstance;
class WAnimGraphNode;

/// Visual graph pin for animation graph nodes.
///
/// Stores animation-specific pin metadata such as the animation data type and whether it supports multiple inputs.
class WAnimationGraphNodePin : public WVisualGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationGraphNodePin, WVisualGraphPin);

public:
  WAnimationGraphNodePin(Type type, const char* szName, const WColorGammaUB& color, const WDocumentObject* pObject);
  ~WAnimationGraphNodePin();

  bool m_bMultiInputPin = false;
  WAnimGraphPin::Type m_DataType = WAnimGraphPin::Invalid;
};

/// Object manager for animation graphs.
///
/// Manages animation graph nodes and their connections. Handles dynamic pin creation for nodes
/// that support variable numbers of inputs, and validates connections based on animation data types.
class WAnimationGraphNodeManager : public WVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const WDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const WDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override;

  virtual WStatus InternalCanConnect(const WVisualGraphPin& source, const WVisualGraphPin& target, CanConnectResult& out_result) const override;

private:
  virtual bool InternalIsDynamicPinProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp) const override;
};

class WAnimationGraphAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationGraphAssetProperties, WReflectedClass);

public:
  WDynamicArray<WString> m_IncludeGraphs;
  WDynamicArray<WAnimationClipMapping> m_AnimationClipMapping;
};

class WAnimationGraphAssetDocument : public WSimpleAssetDocument<WAnimationGraphAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationGraphAssetDocument, WSimpleAssetDocument<WAnimationGraphAssetProperties>);

public:
  WAnimationGraphAssetDocument(WStringView sDocumentPath);

protected:
  struct PinCount
  {
    WUInt16 m_uiInputCount = 0;
    WUInt16 m_uiInputIdx = 0;
    WUInt16 m_uiOutputCount = 0;
    WUInt16 m_uiOutputIdx = 0;
  };

  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const override;
  virtual bool Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;

  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;
};
