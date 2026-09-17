#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneContext, 1, WRTTIDefaultAllocator<WSceneContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab;PropertyAnim"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WWorld* WSceneContext::s_pWorldLinkedWithGameState = nullptr;

void WSceneContext::ComputeHierarchyBounds(WGameObject* pObj, WBoundingBoxSphere& bounds)
{
  pObj->UpdateGlobalTransformAndBounds();
  const auto& b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
  {
    ComputeHierarchyBounds(it, bounds);
  }
}

void WSceneContext::DrawSelectionBounds(const WViewHandle& hView)
{
  if (!m_bRenderSelectionBoxes)
    return;

  W_LOCK(m_pWorld->GetWriteMarker());

  for (const auto& obj : m_Selection)
  {
    WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

    WGameObject* pObj;
    if (!m_pWorld->TryGetObject(obj, pObj))
      continue;

    ComputeHierarchyBounds(pObj, bounds);

    if (bounds.IsValid())
    {
      WDebugRenderer::DrawLineBoxCorners(hView, bounds.GetBox(), 0.25f, WColorScheme::LightUI(WColorScheme::Yellow));
    }
  }
}

void WSceneContext::UpdateInvisibleLayerTags()
{
  if (m_bInvisibleLayersDirty)
  {
    m_bInvisibleLayersDirty = false;

    WMap<WUuid, WUInt32> layerGuidToIndex;
    for (WUInt32 i = 0; i < m_Layers.GetCount(); i++)
    {
      if (m_Layers[i] != nullptr)
      {
        layerGuidToIndex.Insert(m_Layers[i]->GetDocumentGuid(), i);
      }
    }

    WTempHybridArray<WTag, 1> newInvisibleLayerTags;
    newInvisibleLayerTags.Reserve(m_InvisibleLayers.GetCount());
    for (const WUuid& guid : m_InvisibleLayers)
    {
      WUInt32 uiLayerID = 0;
      if (layerGuidToIndex.TryGetValue(guid, uiLayerID))
      {
        newInvisibleLayerTags.PushBack(m_Layers[uiLayerID]->GetLayerTag());
      }
      else if (guid == GetDocumentGuid())
      {
        newInvisibleLayerTags.PushBack(m_LayerTag);
      }
    }

    for (WEngineProcessViewContext* pView : m_ViewContexts)
    {
      if (pView)
      {
        static_cast<WSceneViewContext*>(pView)->SetInvisibleLayerTags(m_InvisibleLayerTags.GetArrayPtr(), newInvisibleLayerTags.GetArrayPtr());
      }
    }
    m_InvisibleLayerTags.Swap(newInvisibleLayerTags);
  }
}

WSceneContext::WSceneContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
  m_bRenderSelectionOverlay = true;
  m_bRenderSelectionBoxes = true;
  m_bRenderShapeIcons = true;
  m_fGridDensity = 0;
  m_GridTransform.SetIdentity();
  m_pWorld = nullptr;

  WResourceManager::GetManagerEvents().AddEventHandler(WMakeDelegate(&WSceneContext::OnResourceManagerEvent, this));
  WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(WMakeDelegate(&WSceneContext::GameApplicationEventHandler, this));
}

WSceneContext::~WSceneContext()
{
  WResourceManager::GetManagerEvents().RemoveEventHandler(WMakeDelegate(&WSceneContext::OnResourceManagerEvent, this));
  WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(WMakeDelegate(&WSceneContext::GameApplicationEventHandler, this));
}

void WSceneContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WWorldSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages
    HandleWorldSettingsMsg(static_cast<const WWorldSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WSimulationSettingsMsgToEngine>())
  {
    HandleSimulationSettingsMsg(static_cast<const WSimulationSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WGridSettingsMsgToEngine>())
  {
    HandleGridSettingsMsg(static_cast<const WGridSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WObjectsForDebugVisMsgToEngine>())
  {
    HandleObjectsForDebugVisMsg(static_cast<const WObjectsForDebugVisMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WGameModeMsgToEngine>())
  {
    HandleGameModeMsg(static_cast<const WGameModeMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const WObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WExposedDocumentObjectPropertiesMsgToEngine>())
  {
    HandleExposedPropertiesMsg(static_cast<const WExposedDocumentObjectPropertiesMsgToEngine*>(pMsg));
    return;
  }

  if (const WExportSceneGeometryMsgToEngine* msg = WDynamicCast<const WExportSceneGeometryMsgToEngine*>(pMsg))
  {
    HandleSceneGeometryMsg(msg);
    return;
  }

  if (const WPullObjectStateMsgToEngine* msg = WDynamicCast<const WPullObjectStateMsgToEngine*>(pMsg))
  {
    HandlePullObjectStateMsg(msg);
    return;
  }

  if (const WSyncChildOrderMsgToEngine* msg = WDynamicCast<const WSyncChildOrderMsgToEngine*>(pMsg))
  {
    HandleSyncChildOrderMsg(msg);
    return;
  }

  if (pMsg->IsInstanceOf<WViewRedrawMsgToEngine>())
  {
    HandleViewRedrawMsg(static_cast<const WViewRedrawMsgToEngine*>(pMsg));
    // fall through
  }

  if (pMsg->IsInstanceOf<WActiveLayerChangedMsgToEngine>())
  {
    HandleActiveLayerChangedMsg(static_cast<const WActiveLayerChangedMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<WObjectTagMsgToEngine>())
  {
    HandleTagMsgToEngineMsg(static_cast<const WObjectTagMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<WLayerVisibilityChangedMsgToEngine>())
  {
    HandleLayerVisibilityChangedMsgToEngineMsg(static_cast<const WLayerVisibilityChangedMsgToEngine*>(pMsg));
    return;
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);

  if (pMsg->IsInstanceOf<WEntityMsgToEngine>())
  {
    W_LOCK(m_pWorld->GetWriteMarker());
    AddLayerIndexTag(*static_cast<const WEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void WSceneContext::HandleViewRedrawMsg(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_bUpdateAllLocalBounds)
  {
    m_bUpdateAllLocalBounds = false;

    W_LOCK(m_pWorld->GetWriteMarker());

    for (auto it = m_pWorld->GetObjects(); it.IsValid(); ++it)
    {
      it->UpdateLocalBounds();
    }
  }

  auto pDocView = GetViewContext(pMsg->m_uiViewID);
  if (pDocView)
    DrawSelectionBounds(pDocView->GetViewHandle());

  AnswerObjectStatePullRequest(pMsg);
  UpdateInvisibleLayerTags();
}

void WSceneContext::AnswerObjectStatePullRequest(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pWorld->GetWorldSimulationEnabled() || m_PushObjectStateMsg.m_ObjectStates.IsEmpty())
    return;

  W_LOCK(m_pWorld->GetReadMarker());

  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    WWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      return;

    // if the handle map is currently empty, the scene has not yet been sent over
    // return and try again later
    if (pContext->m_GameObjectMap.GetHandleToGuidMap().IsEmpty())
      return;
  }

  // now we need to adjust the transforms for all objects that were not directly pulled
  // ie. nodes inside instantiated prefabs
  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    // ignore the ones that we accessed directly, their transform is correct already
    if (!state.m_bAdjustFromPrefabRootChild)
      continue;

    WWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      continue;

    const auto& objectMapper = pContext->m_GameObjectMap;

    WGameObjectHandle hObject = objectMapper.GetHandle(state.m_ObjectGuid);

    // if this object does not exist anymore, this is not considered a problem (user may have deleted it)
    WGameObject* pObject;
    if (!m_pWorld->TryGetObject(hObject, pObject))
      continue;

    // we expect the object to have a child, if none is there yet, we assume the prefab
    // instantiation has not happened yet
    // stop the whole process and try again later
    if (pObject->GetChildCount() == 0)
      return;

    const WGameObject* pChild = pObject->GetChildren();

    const WVec3 localPos = pChild->GetLocalPosition();
    const WQuat localRot = pChild->GetLocalRotation();

    // the child's local position is expressed in the parent's (scaled) space,
    // so it has to be scaled by the parent's global scale before it can be subtracted
    // from the child's global position, otherwise scaled prefabs end up misplaced
    const WVec3 vParentScale = pObject->GetGlobalScaling();

    // now adjust the position
    state.m_qRotation = state.m_qRotation * localRot.GetInverse();
    state.m_vPosition -= state.m_qRotation * vParentScale.CompMul(localPos);
  }

  // send a return message with the result
  m_PushObjectStateMsg.m_DocumentGuid = pMsg->m_DocumentGuid;
  SendProcessMessage(&m_PushObjectStateMsg);

  m_PushObjectStateMsg.m_ObjectStates.Clear();
}

void WSceneContext::HandleActiveLayerChangedMsg(const WActiveLayerChangedMsgToEngine* pMsg)
{
  m_ActiveLayer = pMsg->m_ActiveLayer;
}

void WSceneContext::HandleTagMsgToEngineMsg(const WObjectTagMsgToEngine* pMsg)
{
  W_LOCK(m_pWorld->GetWriteMarker());

  WGameObjectHandle hObject = GetActiveContext().m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag(pMsg->m_sTag);

  WGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (pMsg->m_bApplyOnAllChildren)
    {
      if (pMsg->m_bSetTag)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (pMsg->m_bSetTag)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void WSceneContext::HandleLayerVisibilityChangedMsgToEngineMsg(const WLayerVisibilityChangedMsgToEngine* pMsg)
{
  m_InvisibleLayers = pMsg->m_HiddenLayers;
  m_bInvisibleLayersDirty = true;
}

void WSceneContext::HandleGridSettingsMsg(const WGridSettingsMsgToEngine* pMsg)
{
  m_fGridDensity = pMsg->m_fGridDensity;
  if (m_fGridDensity != 0.0f)
  {
    m_GridTransform.m_vPosition = pMsg->m_vGridCenter;

    if (pMsg->m_vGridTangent1.IsZero())
    {
      m_GridTransform.m_vScale.SetZero();
    }
    else
    {
      m_GridTransform.m_vScale.Set(1.0f);

      WMat3 mRot;
      mRot.SetColumn(0, pMsg->m_vGridTangent1);
      mRot.SetColumn(1, pMsg->m_vGridTangent2);
      mRot.SetColumn(2, pMsg->m_vGridTangent1.CrossRH(pMsg->m_vGridTangent2));
      m_GridTransform.m_qRotation = WQuat::MakeFromMat3(mRot);
    }
  }
}

void WSceneContext::HandleSimulationSettingsMsg(const WSimulationSettingsMsgToEngine* pMsg)
{
  const bool bSimulate = pMsg->m_bSimulateWorld;
  WGameStateBase* pState = GetGameState();
  m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);
  m_pWorld->GetClock().SetPaused(pMsg->m_fSimulationSpeed == 0.0f);

  if (pState == nullptr && bSimulate != m_pWorld->GetWorldSimulationEnabled())
  {
    m_pWorld->SetWorldSimulationEnabled(bSimulate);

    if (bSimulate)
      OnSimulationEnabled();
    else
      OnSimulationDisabled();
  }
}

void WSceneContext::HandleWorldSettingsMsg(const WWorldSettingsMsgToEngine* pMsg)
{
  m_bRenderSelectionOverlay = pMsg->m_bRenderOverlay;
  m_bRenderShapeIcons = pMsg->m_bRenderShapeIcons;
  m_bRenderSelectionBoxes = pMsg->m_bRenderSelectionBoxes;

  if (pMsg->m_bAddAmbientLight)
    AddAmbientLight(true, false);
  else
    RemoveAmbientLight();
}

void WSceneContext::QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  WBoundingBoxSphere bounds = WBoundingBoxSphere::MakeInvalid();

  {
    W_LOCK(m_pWorld->GetWriteMarker());

    for (const auto& obj : m_Selection)
    {
      WGameObject* pObj;
      if (!m_pWorld->TryGetObject(obj, pObj))
        continue;

      ComputeHierarchyBounds(pObj, bounds);
    }

    // if there are no valid bounds, at all, use dummy bounds for each object
    if (!bounds.IsValid())
    {
      for (const auto& obj : m_Selection)
      {
        WGameObject* pObj;
        if (!m_pWorld->TryGetObject(obj, pObj))
          continue;

        bounds.ExpandToInclude(WBoundingBoxSphere::MakeFromCenterExtents(pObj->GetGlobalPosition(), WVec3(0.0f), 0.0f));
      }
    }
  }

  // W_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

  if (!bounds.IsValid() || bounds.IsNaN())
  {
    WLog::Error("Selection has no valid bounding box");
    return;
  }

  const WQuerySelectionBBoxMsgToEngine* msg = static_cast<const WQuerySelectionBBoxMsgToEngine*>(pMsg);

  WQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void WSceneContext::OnSimulationEnabled()
{
  WLog::Info("World Simulation enabled");

  WSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  WResourceManager::ReloadAllResources(false);

  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(true);
  }
}

void WSceneContext::OnSimulationDisabled()
{
  WLog::Info("World Simulation disabled");

  WResourceManager::ResetAllResources();

  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

WGameStateBase* WSceneContext::GetGameState() const
{
  if (s_pWorldLinkedWithGameState == m_pWorld)
  {
    return WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState();
  }

  return nullptr;
}

WUInt32 WSceneContext::RegisterLayer(WLayerContext* pLayer)
{
  m_bInvisibleLayersDirty = true;
  m_Contexts.PushBack(&pLayer->m_Context);
  for (WUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == nullptr)
    {
      m_Layers[i] = pLayer;
      return i;
    }
  }

  m_Layers.PushBack(pLayer);
  return m_Layers.GetCount() - 1;
}

void WSceneContext::UnregisterLayer(WLayerContext* pLayer)
{
  m_Contexts.RemoveAndSwap(&pLayer->m_Context);
  for (WUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == pLayer)
    {
      m_Layers[i] = nullptr;
    }
  }

  while (!m_Layers.IsEmpty() && m_Layers.PeekBack() == nullptr)
    m_Layers.PopBack();
}

void WSceneContext::AddLayerIndexTag(const WEntityMsgToEngine& msg, WWorldRttiConverterContext& ref_context, const WTag& layerTag)
{
  if (msg.m_change.m_Change.m_Operation == WObjectChangeType::NodeAdded)
  {
    if ((msg.m_change.m_Change.m_sProperty == "Children" || msg.m_change.m_Change.m_sProperty.IsEmpty()) && msg.m_change.m_Change.m_Value.IsA<WUuid>())
    {
      const WUuid& object = msg.m_change.m_Change.m_Value.Get<WUuid>();
      WRttiConverterObject target = ref_context.GetObjectByGUID(object);
      if (target.m_pType == WGetStaticRTTI<WGameObject>() && target.m_pObject != nullptr)
      {
        // We do postpone tagging until after the first frame so that prefab references are instantiated and affected as well.
        WGameObject* pObject = static_cast<WGameObject*>(target.m_pObject);
        m_ObjectsToTag.PushBack({pObject->GetHandle(), layerTag});
      }
    }
  }
}

const WArrayPtr<const WTag> WSceneContext::GetInvisibleLayerTags() const
{
  return m_InvisibleLayerTags.GetArrayPtr();
}

void WSceneContext::OnInitialize()
{
  W_LOCK(m_pWorld->GetWriteMarker());
  if (!m_ActiveLayer.IsValid())
    m_ActiveLayer = m_DocumentGuid;
  m_Contexts.PushBack(&m_Context);

  m_LayerTag = WTagRegistry::GetGlobalRegistry().RegisterTag("Layer_Scene");

  WShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void WSceneContext::OnDeinitialize()
{
  m_Selection.Clear();
  m_SelectionWithChildren.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_hSkyLight.Invalidate();
  m_hDirectionalLight.Invalidate();
  m_LayerTag = WTag();
  m_ObjectsToTag.Clear();
  for (WLayerContext* pLayer : m_Layers)
  {
    if (pLayer != nullptr)
      pLayer->SceneDeinitialized();
  }
}

WEngineProcessViewContext* WSceneContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WSceneViewContext, this);
}

void WSceneContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

void WSceneContext::HandleSelectionMsg(const WObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  WStringBuilder sSel = pMsg->m_sSelection;
  WStringBuilder sGuid;

  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const WUuid guid = WConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = GetActiveContext().m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      WGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void WSceneContext::OnPlayTheGameModeStarted(WStringView sStartPosition, const WTransform& startPositionOffset)
{
  if (WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState() != nullptr)
  {
    WLog::Warning("A Play-the-Game instance is already running, cannot launch a second in parallel.");
    return;
  }

  WLog::Info("Starting Play-the-Game mode");

  WSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), false);

  WResourceManager::ReloadAllResources(false);

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  s_pWorldLinkedWithGameState = m_pWorld;
  WGameApplicationBase::GetGameApplicationBaseInstance()->ActivateGameState(m_pWorld, sStartPosition, startPositionOffset);

  WGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG = true;

  SendProcessMessage(&msgRet);

  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

void WSceneContext::OnResourceManagerEvent(const WResourceManagerEvent& e)
{
  if (e.m_Type == WResourceManagerEvent::Type::ReloadAllResources)
  {
    // when resources get reloaded, make sure to update all object bounds
    // this is to prevent culling errors after meshes got transformed etc.
    m_bUpdateAllLocalBounds = true;
  }
}

void WSceneContext::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  if (e.m_Type == WGameApplicationExecutionEvent::Type::AfterUpdatePlugins && !m_ObjectsToTag.IsEmpty())
  {
    // At this point the world was ticked once and prefab instances are instantiated and will be affected by SetTagRecursive.
    W_LOCK(m_pWorld->GetWriteMarker());
    for (const TagGameObject& tagObject : m_ObjectsToTag)
    {
      WGameObject* pObject = nullptr;
      if (m_pWorld->TryGetObject(tagObject.m_hObject, pObject))
      {
        SetTagRecursive(pObject, tagObject.m_Tag);
      }
    }
    m_ObjectsToTag.Clear();
  }
}

void WSceneContext::HandleObjectsForDebugVisMsg(const WObjectsForDebugVisMsgToEngine* pMsg)
{
  W_LOCK(GetWorld()->GetWriteMarker());

  const WArrayPtr<const WUuid> guids(reinterpret_cast<const WUuid*>(pMsg->m_Objects.GetData()), pMsg->m_Objects.GetCount() / sizeof(WUuid));

  for (auto guid : guids)
  {
    auto hComp = GetActiveContext().m_ComponentMap.GetHandle(guid);

    if (hComp.IsInvalidated())
      continue;

    WEventMessageHandlerComponent* pComp = nullptr;
    if (!m_pWorld->TryGetComponent(hComp, pComp))
      continue;

    pComp->SetDebugOutput(true);
  }
}

void WSceneContext::HandleGameModeMsg(const WGameModeMsgToEngine* pMsg)
{
  WGameStateBase* pState = GetGameState();

  if (pMsg->m_bEnablePTG)
  {
    if (pState != nullptr)
    {
      WLog::Error("Cannot start Play-the-Game, there is already a game state active for this world");
      return;
    }

    if (pMsg->m_bUseStartPosition)
    {
      WQuat qRot = WQuat::MakeShortestRotation(WVec3(1, 0, 0), pMsg->m_vStartDirection);

      WTransform tStart(pMsg->m_vStartPosition, qRot);

      OnPlayTheGameModeStarted("GlobalOverride", tStart);
    }
    else
    {
      OnPlayTheGameModeStarted({}, WTransform::MakeIdentity());
    }
  }
  else
  {
    if (pState == nullptr)
      return;

    WLog::Info("Attempting to stop Play-the-Game mode");
    pState->RequestQuit("editor-force");
  }
}

void WSceneContext::InsertSelectedChildren(const WGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}

WStatus WSceneContext::ExportDocument(const WExportDocumentMsgToEngine* pMsg)
{
  if (!m_Context.m_UnknownTypes.IsEmpty())
  {
    WStringBuilder s;

    s.Append("Scene / prefab export failed: ");

    for (const WString& sType : m_Context.m_UnknownTypes)
    {
      s.AppendFormat("'{}' is unknown. ", sType);
    }

    return WStatus(s.GetView());
  }

  // make sure the world has been updated at least once, otherwise components aren't initialized
  // and messages for geometry extraction won't be delivered
  // this is necessary for the scene export modifiers to work
  {
    W_LOCK(m_pWorld->GetWriteMarker());
    m_pWorld->SetWorldSimulationEnabled(false);
    m_pWorld->Update();
  }

  // #TODO layers
  WSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentType(), GetDocumentGuid(), true);

  WDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // export
  {
    // File Header
    {
      WAssetFileHeader header;
      header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
      header.Write(file).IgnoreResult();

      const char* szSceneTag = "[WEBinaryScene]";
      file.WriteBytes(szSceneTag, sizeof(char) * 16).IgnoreResult();
    }

    const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    const WTag& tagNoExport = WTagRegistry::GetGlobalRegistry().RegisterTag("Exclude From Export");

    WTagSet tags;
    tags.Set(tagEditor);
    tags.Set(tagNoExport);

    WWorldWriter ww;
    ww.WriteWorld(file, *m_pWorld, &tags);

    ExportExposedParameters(ww, file);
  }

  // do the actual file writing
  if (file.Close().Failed())
    return WStatus(WFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return WStatus(W_SUCCESS);
}

void WSceneContext::ExportExposedParameters(const WWorldWriter& ww, WDeferredFileWriter& file) const
{
  WTempHybridArray<WExposedPrefabParameterDesc, 16> exposedParams;

  for (const auto& esp : m_ExposedSceneProperties)
  {
    WGameObject* pTargetObject = nullptr;
    const WRTTI* pComponenType = nullptr;

    WRttiConverterObject obj = m_Context.GetObjectByGUID(esp.m_Object);

    if (obj.m_pType == nullptr)
      continue;

    if (obj.m_pType->IsDerivedFrom<WGameObject>())
    {
      pTargetObject = reinterpret_cast<WGameObject*>(obj.m_pObject);
    }
    else if (obj.m_pType->IsDerivedFrom<WComponent>())
    {
      WComponent* pComponent = reinterpret_cast<WComponent*>(obj.m_pObject);

      pTargetObject = pComponent->GetOwner();
      pComponenType = obj.m_pType;
    }

    if (pTargetObject == nullptr)
      continue;

    WInt32 iFoundObjRoot = -1;
    WInt32 iFoundObjChild = -1;

    // search for the target object in the exported objects
    {
      const auto& objects = ww.GetAllWrittenRootObjects();
      for (WUInt32 i = 0; i < objects.GetCount(); ++i)
      {
        if (objects[i] == pTargetObject)
        {
          iFoundObjRoot = i;
          break;
        }
      }

      if (iFoundObjRoot < 0)
      {
        const auto& objects = ww.GetAllWrittenChildObjects();
        for (WUInt32 i = 0; i < objects.GetCount(); ++i)
        {
          if (objects[i] == pTargetObject)
          {
            iFoundObjChild = i;
            break;
          }
        }
      }
    }

    // if exposed object not found, ignore parameter
    if (iFoundObjRoot < 0 && iFoundObjChild < 0)
      continue;

    // store the exposed parameter information
    WExposedPrefabParameterDesc& paramdesc = exposedParams.ExpandAndGetRef();
    paramdesc.m_sExposeName.Assign(esp.m_sName.GetData());
    paramdesc.m_uiWorldReaderChildObject = (iFoundObjChild >= 0) ? 1 : 0;
    paramdesc.m_uiWorldReaderObjectIndex = (iFoundObjChild >= 0) ? iFoundObjChild : iFoundObjRoot;
    paramdesc.m_sComponentType.Clear();

    if (pComponenType)
    {
      paramdesc.m_sComponentType.Assign(pComponenType->GetTypeName());
    }

    paramdesc.m_sProperty.Assign(esp.m_sPropertyPath.GetData());
  }

  exposedParams.Sort([](const WExposedPrefabParameterDesc& lhs, const WExposedPrefabParameterDesc& rhs) -> bool
    { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });

  file << exposedParams.GetCount();

  for (const auto& ep : exposedParams)
  {
    ep.Save(file);
  }
}

void WSceneContext::OnThumbnailViewContextCreated()
{
  // make sure there is ambient light in the thumbnails
  // TODO: should check whether this is a prefab (info currently not available in WSceneContext)
  RemoveAmbientLight();
  AddAmbientLight(false, true);
}

void WSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}

void WSceneContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();

  if (WGameStateBase* pState = GetGameState())
  {
    // If we have a running game state we always want to render it (e.g. play the game).
    pState->AddMainViewsToRender();

    if (pState->WasQuitRequested())
    {
      WGameApplicationBase::GetGameApplicationBaseInstance()->DeactivateGameState();
      s_pWorldLinkedWithGameState = nullptr;

      WGameModeMsgToEditor msgToEd;
      msgToEd.m_DocumentGuid = GetDocumentGuid();
      msgToEd.m_bRunningPTG = false;

      SendProcessMessage(&msgToEd);
    }
  }
}

WGameObjectHandle WSceneContext::ResolveStringToGameObjectHandle(const void* pString, WComponentHandle hThis, WStringView sProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pString);

  if (hThis.IsInvalidated() && sProperty.IsEmpty())
  {
    // This code path is used by WPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!WConversionUtils::IsStringUuid(szTargetGuid))
      return WGameObjectHandle();

    // convert string to GUID and check if references a known object
    return m_Context.m_GameObjectMap.GetHandle(WConversionUtils::ConvertStringToUuid(szTargetGuid));
  }

  // Test if the component is a direct part of this scene or one of its layers.
  if (m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
  {
    return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
  }
  for (const WLayerContext* pLayer : m_Layers)
  {
    if (pLayer)
    {
      if (pLayer->m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
      {
        return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
      }
    }
  }

  // Component not found - it is probably an engine prefab instance part.
  // Walk up the hierarchy and find a game object that belongs to the scene or layer.
  WComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent<WComponent>(hThis, pComponent))
    return {};

  const WGameObject* pParent = pComponent->GetOwner();
  while (pParent)
  {
    if (m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
    {
      return SUPER::ResolveStringToGameObjectHandle(pString, hThis, sProperty);
    }
    for (const WLayerContext* pLayer : m_Layers)
    {
      if (pLayer)
      {
        if (pLayer->m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
        {
          return pLayer->ResolveStringToGameObjectHandle(pString, hThis, sProperty);
        }
      }
    }
    pParent = pParent->GetParent();
  }

  WLog::Error("Game object reference could not be resolved. Component source was not found.");
  return WGameObjectHandle();
}

bool WSceneContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  const WBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  WSceneViewContext* pMaterialViewContext = static_cast<WSceneViewContext*>(pThumbnailViewContext);
  const bool result = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void WSceneContext::AddAmbientLight(bool bSetEditorTag, bool bForce)
{
  if (!m_hSkyLight.IsInvalidated() || !m_hDirectionalLight.IsInvalidated())
    return;

  W_LOCK(GetWorld()->GetWriteMarker());

  // delay adding ambient light until the scene isn't empty, to prevent adding two skylights
  if (!bForce && GetWorld()->GetObjectCount() == 0)
    return;

  WSkyLightComponentManager* pSkyMan = GetWorld()->GetComponentManager<WSkyLightComponentManager>();
  if (pSkyMan == nullptr || pSkyMan->GetSingletonComponent() == nullptr)
  {
    // only create a skylight, if there is none yet

    WGameObjectDesc obj;
    obj.m_sName.Assign("Sky Light");

    if (bSetEditorTag)
    {
      const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    WGameObject* pObj;
    m_hSkyLight = GetWorld()->CreateObject(obj, pObj);


    WSkyLightComponent* pSkyLight = nullptr;
    WSkyLightComponent::CreateComponent(pObj, pSkyLight);
    pSkyLight->SetCubeMapFile("{ 0b202e08-a64f-465d-b38e-15b81d161822 }");
    pSkyLight->SetReflectionProbeMode(WReflectionProbeMode::Static);
  }

  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("Ambient Light");

    obj.m_LocalRotation = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(-14.510815f), WAngle::MakeFromDegree(43.07951f), WAngle::MakeFromDegree(93.223808f));

    if (bSetEditorTag)
    {
      const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    WGameObject* pLight;
    m_hDirectionalLight = GetWorld()->CreateObject(obj, pLight);

    WDirectionalLightComponent* pDirLight = nullptr;
    WDirectionalLightComponent::CreateComponent(pLight, pDirLight);
    pDirLight->SetIntensity(10.0f);
  }
}

void WSceneContext::RemoveAmbientLight()
{
  W_LOCK(GetWorld()->GetWriteMarker());

  if (!m_hSkyLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hSkyLight);
    m_hSkyLight.Invalidate();
  }

  if (!m_hDirectionalLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hDirectionalLight);
    m_hDirectionalLight.Invalidate();
  }
}

const WEngineProcessDocumentContext* WSceneContext::GetActiveDocumentContext() const
{
  if (m_ActiveLayer == GetDocumentGuid())
  {
    return this;
  }

  for (const WLayerContext* pLayer : m_Layers)
  {
    if (pLayer && m_ActiveLayer == pLayer->GetDocumentGuid())
    {
      return pLayer;
    }
  }

  W_REPORT_FAILURE("Active layer does not exist.");
  return this;
}

WEngineProcessDocumentContext* WSceneContext::GetActiveDocumentContext()
{
  return const_cast<WEngineProcessDocumentContext*>(const_cast<const WSceneContext*>(this)->GetActiveDocumentContext());
}

const WWorldRttiConverterContext& WSceneContext::GetActiveContext() const
{
  return GetActiveDocumentContext()->m_Context;
}

WWorldRttiConverterContext& WSceneContext::GetActiveContext()
{
  return const_cast<WWorldRttiConverterContext&>(const_cast<const WSceneContext*>(this)->GetActiveContext());
}

WWorldRttiConverterContext* WSceneContext::GetContextForLayer(const WUuid& layerGuid)
{
  if (layerGuid == GetDocumentGuid())
    return &m_Context;

  for (WLayerContext* pLayer : m_Layers)
  {
    if (pLayer && layerGuid == pLayer->GetDocumentGuid())
    {
      return &pLayer->m_Context;
    }
  }
  return nullptr;
}

WArrayPtr<WWorldRttiConverterContext*> WSceneContext::GetAllContexts()
{
  return m_Contexts;
}

void WSceneContext::HandleExposedPropertiesMsg(const WExposedDocumentObjectPropertiesMsgToEngine* pMsg)
{
  m_ExposedSceneProperties = pMsg->m_Properties;
}

void WSceneContext::HandleSceneGeometryMsg(const WExportSceneGeometryMsgToEngine* pMsg)
{
  WWorldGeoExtractionUtil::MeshObjectList objects;

  WTagSet excludeTags;
  excludeTags.SetByName("Editor");

  if (pMsg->m_bSelectionOnly)
    WWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<WWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), m_SelectionWithChildren);
  else
    WWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<WWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), &excludeTags);

  WWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(pMsg->m_sOutputFile, objects, pMsg->m_Transform);
}

void WSceneContext::HandlePullObjectStateMsg(const WPullObjectStateMsgToEngine* pMsg)
{
  if (!m_pWorld->GetWorldSimulationEnabled())
    return;

  // Objects that don't move on their own can't be placed physically. Ask them to become simulated for the rest of
  // this simulation, so that pressing the shortcut again records where they came to rest.
  {
    WWorld* pMutableWorld = GetWorld();
    W_LOCK(pMutableWorld->GetWriteMarker());

    WMsgPhysicsMakeTemporarilyDynamic makeDynamicMsg;

    for (WGameObjectHandle hObject : m_SelectionWithChildren)
    {
      WGameObject* pObject = nullptr;
      if (!pMutableWorld->TryGetObject(hObject, pObject))
        continue;

      pObject->SendMessage(makeDynamicMsg);
    }
  }

  const WWorld* pWorld = GetWorld();
  W_LOCK(pWorld->GetReadMarker());

  const auto& objectMapper = GetActiveContext().m_GameObjectMap;

  m_PushObjectStateMsg.m_ObjectStates.Reserve(m_PushObjectStateMsg.m_ObjectStates.GetCount() + m_SelectionWithChildren.GetCount());

  for (WGameObjectHandle hObject : m_SelectionWithChildren)
  {
    const WGameObject* pObject = nullptr;
    if (!pWorld->TryGetObject(hObject, pObject))
      continue;

    WUuid objectGuid = objectMapper.GetGuid(hObject);
    bool bAdjust = false;

    if (!objectGuid.IsValid())
    {
      // this must be an object created on the runtime side, try to match it to some editor object
      // we only try the direct parent, more steps than that are not allowed

      const WGameObject* pParentObject = pObject->GetParent();
      if (pParentObject == nullptr)
        continue;

      // if the parent has more than one child, remapping the position from the child to the parent is not possible, so skip those
      if (pParentObject->GetChildCount() > 1)
        continue;

      auto parentGuid = objectMapper.GetGuid(pParentObject->GetHandle());

      if (!parentGuid.IsValid())
        continue;

      objectGuid = parentGuid;
      bAdjust = true;

      for (WUInt32 i = 0; i < m_PushObjectStateMsg.m_ObjectStates.GetCount(); ++i)
      {
        if (m_PushObjectStateMsg.m_ObjectStates[i].m_ObjectGuid == objectGuid)
        {
          m_PushObjectStateMsg.m_ObjectStates.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    {
      auto& state = m_PushObjectStateMsg.m_ObjectStates.ExpandAndGetRef();

      state.m_LayerGuid = m_ActiveLayer;
      state.m_ObjectGuid = objectGuid;
      state.m_bAdjustFromPrefabRootChild = bAdjust;
      state.m_vPosition = pObject->GetGlobalPosition();
      state.m_qRotation = pObject->GetGlobalRotation();

      WMsgRetrieveBoneState msg;
      pObject->SendMessage(msg);

      state.m_BoneTransforms = msg.m_BoneTransforms;
    }
  }

  // the return message is sent after the simulation has stopped
}

void WSceneContext::HandleSyncChildOrderMsg(const WSyncChildOrderMsgToEngine* pMsg)
{
  W_LOCK(m_pWorld->GetWriteMarker());

  WWorldRttiConverterContext* pContext = pMsg->m_LayerGuid.IsValid() ? GetContextForLayer(pMsg->m_LayerGuid) : &GetActiveContext();
  if (pContext == nullptr)
    return;

  WComponentHandle hComponent = pContext->m_ComponentMap.GetHandle(pMsg->m_ComponentGuid);
  WComponent* pComponent = nullptr;
  if (!m_pWorld->TryGetComponent(hComponent, pComponent))
    return;

  WVariantArray handles;
  handles.Reserve(pMsg->m_ChildOrder.GetCount());
  for (const WUuid& guid : pMsg->m_ChildOrder)
  {
    handles.PushBack(WVariant(pContext->m_GameObjectMap.GetHandle(guid)));
  }

  for (const WAbstractFunctionProperty* pFunc : pComponent->GetDynamicRTTI()->GetFunctions())
  {
    if (WStringUtils::IsEqual(pFunc->GetPropertyName(), "SetChildOrder"))
    {
      WTempHybridArray<WVariant, 1> params;
      params.PushBack(handles);
      WVariant ret;
      pFunc->Execute(pComponent, params, ret);
      break;
    }
  }
}
