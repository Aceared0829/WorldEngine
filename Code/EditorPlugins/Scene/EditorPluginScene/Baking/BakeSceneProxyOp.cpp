#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Baking/BakeSceneProxyOp.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpProxy_BakeScene, 1, WRTTIDefaultAllocator<WLongOpProxy_BakeScene>);
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WLongOpProxy_BakeScene::InitializeRegistered(const WUuid& documentGuid, const WUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void WLongOpProxy_BakeScene::GetReplicationInfo(WStringBuilder& out_sReplicationOpType, WStreamWriter& ref_description)
{
  out_sReplicationOpType = "WLongOpWorker_BakeScene";

  WStringBuilder sOutputPath;
  sOutputPath.SetFormat(":project/AssetCache/Generated/{0}", m_ComponentGuid);
  ref_description << sOutputPath;
}

void WLongOpProxy_BakeScene::Finalize(WResult result, const WDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    WQtEditorApp::GetSingleton()->ReloadEngineResources();
  }
}
