#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Baking/BakeSceneWorkerOp.h>

#ifdef BUILDSYSTEM_ENABLE_EMBREE_SUPPORT

#  include <BakingPlugin/BakingScene.h>
#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#  include <Foundation/Utilities/Progress.h>
#  include <ToolsFoundation/Document/DocumentManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpWorker_BakeScene, 1, WRTTIDefaultAllocator<WLongOpWorker_BakeScene>);
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WLongOpWorker_BakeScene::InitializeExecution(WStreamReader& config, const WUuid& DocumentGuid)
{
  WEngineProcessDocumentContext* pDocContext = WEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return W_FAILURE;

  config >> m_sOutputPath;

  {
    m_pScene = WBaking::GetSingleton()->GetOrCreateScene(*pDocContext->GetWorld());

    W_SUCCEED_OR_RETURN(m_pScene->Extract());
  }

  return W_SUCCESS;
}

WResult WLongOpWorker_BakeScene::Execute(WProgress& progress, WStreamWriter& proxydata)
{
  W_SUCCEED_OR_RETURN(m_pScene->Bake(m_sOutputPath, progress));

  return W_SUCCESS;
}

#endif
