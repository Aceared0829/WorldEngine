#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineAssetDocument, 4, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineAssetDocument::WStateMachineAssetDocument(WStringView sDocumentPath)
  : WAssetDocument(sDocumentPath, W_DEFAULT_NEW(WStateMachineNodeManager), WAssetDocEngineConnection::FullObjectMirroring)
{
}

WTransformStatus WStateMachineAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  return WAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

WTransformStatus WStateMachineAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_REPORT_FAILURE("Should not be called");
  return WTransformStatus();
}

void WStateMachineAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const WStateMachineNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void WStateMachineAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const WStateMachineNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void WStateMachineAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<WStateMachineNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void WStateMachineAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.StateMachineGraph");
}

bool WStateMachineAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const
{
  out_MimeType = "application/WEditor.StateMachineGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  if (!pManager->CopySelectedObjects(out_objectGraph))
    return false;

  // prevent that we get a second node with "IsInitialState" set to true
  for (auto itNode : out_objectGraph.GetAllNodes())
  {
    if (auto pInit = itNode.Value()->FindProperty("IsInitialState"))
    {
      pInit->m_Value = false;
    }
  }

  return true;
}

bool WStateMachineAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
