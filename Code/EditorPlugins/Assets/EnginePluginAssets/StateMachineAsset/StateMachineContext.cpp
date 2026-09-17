#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/StateMachineAsset/StateMachineContext.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachineBuiltins.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineContext, 1, WRTTIDefaultAllocator<WStateMachineContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "StateMachine"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WStateMachineContext::WStateMachineContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

WEngineProcessViewContext* WStateMachineContext::CreateViewContext()
{
  W_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void WStateMachineContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_ASSERT_DEV(false, "Should not be called");
}

WStatus WStateMachineContext::ExportDocument(const WExportDocumentMsgToEngine* pMsg)
{
  WDynamicArray<WUuid> nodeUuids;
  WDynamicArray<WStateMachineNodeBase*> nodes;
  WDynamicArray<WStateMachineConnection*> connections;

  m_Context.GetObjectsByType(nodes, &nodeUuids);
  m_Context.GetObjectsByType(connections);

  WStateMachineDescription desc;
  WHashTable<WUuid, WUInt32> nodeUuidToStateIndex;
  WSet<WString> stateNames;

  auto AddState = [&](const WStateMachineNode* pNode, const WUuid& uuid)
  {
    const WString& name = pNode->m_sName;
    if (stateNames.Contains(name))
    {
      return WStatus(WFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    WUniquePtr<WStateMachineState> pState = WUniquePtr<WStateMachineState>(pNode->m_pType, nullptr);
    if (pState == nullptr)
    {
      pState = W_DEFAULT_NEW(WStateMachineState_Empty);
    }

    if (pState->GetName().IsEmpty())
    {
      pState->SetName(name);
    }

    const WUInt32 uiStateIndex = desc.AddState(std::move(pState));
    nodeUuidToStateIndex.Insert(uuid, uiStateIndex);

    return WStatus(W_SUCCESS);
  };

  for (WUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = WDynamicCast<WStateMachineNode*>(nodes[i]);
    if (pNode != nullptr && pNode->m_bIsInitialState)
    {
      const WUuid& nodeUuid = nodeUuids[i];
      W_SUCCEED_OR_RETURN(AddState(pNode, nodeUuid));
      W_ASSERT_DEV(nodeUuidToStateIndex[nodeUuid] == 0, "Initial state has to have index 0");
      break;
    }
  }

  if (nodeUuidToStateIndex.IsEmpty())
  {
    return WStatus("Initial state is not set");
  }

  for (WUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = WDynamicCast<WStateMachineNode*>(nodes[i]);
    if (pNode == nullptr || pNode->m_bIsInitialState)
      continue;

    W_SUCCEED_OR_RETURN(AddState(pNode, nodeUuids[i]));
  }

  for (auto pConnection : connections)
  {
    WUniquePtr<WStateMachineTransition> pTransition = WUniquePtr<WStateMachineTransition>(pConnection->m_pType, nullptr);
    if (pTransition == nullptr)
    {
      pTransition = W_DEFAULT_NEW(WStateMachineTransition_Timeout);
    }

    WUInt32 uiFromStateIndex = WInvalidIndex;
    WUInt32 uiToStateIndex = WInvalidIndex;
    nodeUuidToStateIndex.TryGetValue(pConnection->m_Source, uiFromStateIndex); // Can fail for any states
    W_VERIFY(nodeUuidToStateIndex.TryGetValue(pConnection->m_Target, uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  WDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // Asset Header
  {
    WAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();
  }

  W_SUCCEED_OR_RETURN(desc.Serialize(file));

  // do the actual file writing
  if (file.Close().Failed())
    return WStatus(WFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return WStatus(W_SUCCESS);
}
