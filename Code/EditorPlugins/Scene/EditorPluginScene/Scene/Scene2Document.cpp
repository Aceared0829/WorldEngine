#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneLayerBase, 1, WRTTINoAllocator)
{
  //W_BEGIN_PROPERTIES
  //{
  //}
  //W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneLayer, 1, WRTTIDefaultAllocator<WSceneLayer>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Layer", m_Layer)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneDocumentSettings, 2, WRTTIDefaultAllocator<WSceneDocumentSettings>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Layers", m_Layers)->AddFlags(WPropertyFlags::PointerOwner)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScene2Document, 2, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


WSceneLayerBase::WSceneLayerBase() = default;

WSceneLayerBase::~WSceneLayerBase() = default;

//////////////////////////////////////////////////////////////////////////

WSceneLayer::WSceneLayer() = default;

WSceneLayer::~WSceneLayer() = default;

//////////////////////////////////////////////////////////////////////////

WSceneDocumentSettings::WSceneDocumentSettings() = default;

WSceneDocumentSettings::~WSceneDocumentSettings()
{
  for (WSceneLayerBase* pLayer : m_Layers)
  {
    W_DEFAULT_DELETE(pLayer);
  }
}

WScene2Document::WScene2Document(WStringView sDocumentPath)
  : WSceneDocument(sDocumentPath, WSceneDocument::DocumentType::Scene)
{
  // Separate selection for the layer panel.
  m_pLayerSelection = W_DEFAULT_NEW(WSelectionManager, m_pObjectManager.Borrow());
}

WScene2Document::~WScene2Document()
{
  m_ChildOrderStructureEventUnsubscriber.Unsubscribe();
  m_ChildOrderCommandHistoryUnsubscriber.Unsubscribe();
  m_SelectionHandlerUnsubscriber.Unsubscribe();

  SetActiveLayer(GetGuid()).LogFailure();

  // We need to clear all things that are dependent in the current object manager, selection etc setup before we swap the managers as otherwise those will fail to de-register.
  WVisualizerManager::GetSingleton()->SetVisualizersActive(this, false);
  m_pSelectionManager->Clear();

  // Game object document subscribed to the true document originally but we rerouted that to the mock data.
  // In order to destroy the game object document we need to revert this and subscribe to the true document again.
  UnsubscribeGameObjectEventHandlers();

  // close the layers before we move (and thus potentially destroy) the managers below
  for (auto it : m_Layers)
  {
    auto pDoc = it.Value().m_pLayer;

    if (pDoc && pDoc != this)
    {
      WDocumentManager* pManager = pDoc->GetDocumentManager();
      pManager->CloseDocument(pDoc);
    }
  }

  // Move the preserved real scene document back.
  m_pSelectionManager = std::move(m_pSceneSelectionManager);
  m_pCommandHistory = std::move(m_pSceneCommandHistory);
  m_pObjectManager = std::move(m_pSceneObjectManager);
  m_pObjectAccessor = std::move(m_pSceneObjectAccessor);
  m_DocumentObjectMetaData = std::move(m_pSceneDocumentObjectMetaData);
  m_GameObjectMetaData = std::move(m_pSceneGameObjectMetaData);

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  m_DocumentManagerEventSubscriber.Unsubscribe();
  m_LayerSelectionEventSubscriber.Unsubscribe();
  m_StructureEventSubscriber.Unsubscribe();
  m_CommandHistoryEventSubscriber.Unsubscribe();

  m_pLayerSelection = nullptr;
}

void WScene2Document::SetSwitchLayerToSelection(bool bEnable)
{
  if (m_bSwitchLayerToSelection == bEnable)
    return;

  m_bSwitchLayerToSelection = bEnable;

  WScene2LayerEvent e;
  e.m_Type = WScene2LayerEvent::Type::SettingsChanged;
  m_LayerEvents.Broadcast(e);
}

void WScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

  m_ActiveLayerGuid = GetGuid();
  WObjectDirectAccessor accessor(GetObjectManager());
  WObjectAccessorBase* pAccessor = &accessor;
  auto pRoot = GetObjectManager()->GetObject(GetSettingsObject()->GetGuid());
  if (pRoot->GetChildren().IsEmpty())
  {
    WUuid objectGuid;
    pAccessor->AddObjectByName(pRoot, "Layers", 0, WGetStaticRTTI<WSceneLayer>(), objectGuid).AssertSuccess();
    const WDocumentObject* pObject = pAccessor->GetObject(objectGuid);
    pAccessor->SetValueByName(pObject, "Layer", GetGuid()).AssertSuccess();
  }

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void WScene2Document::InitializeAfterLoadingAndSaving()
{
  m_pLayerSelection->m_Events.AddEventHandler(WMakeDelegate(&WScene2Document::LayerSelectionEventHandler, this), m_LayerSelectionEventSubscriber);
  m_pObjectManager->m_StructureEvents.AddEventHandler(WMakeDelegate(&WScene2Document::StructureEventHandler, this), m_StructureEventSubscriber);
  m_pCommandHistory->m_Events.AddEventHandler(WMakeDelegate(&WScene2Document::CommandHistoryEventHandler, this), m_CommandHistoryEventSubscriber);
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WScene2Document::DocumentManagerEventHandler, this), m_DocumentManagerEventSubscriber);

  SUPER::InitializeAfterLoadingAndSaving();

  // Game object document subscribed to the true document originally but want to reroute it to the mock data so that is picks up the layer content on its own.
  // Therefore we need to unsubscribe the original subscriptions and replace them with ones to the mock data.
  UnsubscribeGameObjectEventHandlers();

  // These preserve the real scene document.
  m_pSceneObjectManager = std::move(m_pObjectManager);
  m_pSceneCommandHistory = std::move(m_pCommandHistory);
  m_pSceneSelectionManager = std::move(m_pSelectionManager);
  m_pSceneObjectAccessor = std::move(m_pObjectAccessor);
  m_pSceneDocumentObjectMetaData = std::move(m_DocumentObjectMetaData);
  m_pSceneGameObjectMetaData = std::move(m_GameObjectMetaData);

  // Replace real scene elements with copies.
  m_pObjectManager = W_DEFAULT_NEW(WSceneObjectManager);
  m_pObjectManager->SetDocument(this);
  m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
  m_pCommandHistory = W_DEFAULT_NEW(WCommandHistory, this);
  m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
  m_pSelectionManager = W_DEFAULT_NEW(WSelectionManager, m_pSceneObjectManager.Borrow());
  m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
  m_pObjectAccessor = W_DEFAULT_NEW(WObjectCommandAccessor, m_pCommandHistory.Borrow());
  using ObjectMetaData = WObjectMetaData<WUuid, WDocumentObjectMetaData>;
  m_DocumentObjectMetaData = W_DEFAULT_NEW(ObjectMetaData);
  m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
  using GameObjectMetaData = WObjectMetaData<WUuid, WGameObjectMetaData>;
  m_GameObjectMetaData = W_DEFAULT_NEW(GameObjectMetaData);
  m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  UpdateLayers();

  if (const WDocumentObject* pLayerObject = GetLayerObject(GetActiveLayer()))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }

  // change the selection handler to our custom selection manager
  m_SelectionHandlerUnsubscriber.Unsubscribe();
  GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WScene2Document::SelectionManagerEventHandler, this), m_SelectionHandlerUnsubscriber);

  m_ChildOrderStructureEventUnsubscriber.Unsubscribe();
  GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WScene2Document::ChildOrderStructureEventHandler, this), m_ChildOrderStructureEventUnsubscriber);

  m_ChildOrderCommandHistoryUnsubscriber.Unsubscribe();
  WCommandHistory* pHistory = IsMainDocument() ? GetCommandHistory() : static_cast<WSceneDocument*>(GetMainDocument())->GetCommandHistory();
  pHistory->m_Events.AddEventHandler(WMakeDelegate(&WScene2Document::ChildOrderCommandHistoryEventHandler, this), m_ChildOrderCommandHistoryUnsubscriber);
}

const WDocumentObject* WScene2Document::GetSettingsObject() const
{
  /// This function is overwritten so that after redirecting to the active document this still accesses the original content and is not redirected.
  if (m_pSceneObjectManager == nullptr)
    return SUPER::GetSettingsObject();

  auto pRoot = GetSceneObjectManager()->GetRootObject();
  WVariant value;
  W_VERIFY(GetSceneObjectAccessor()->GetValueByName(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  WUuid id = value.Get<WUuid>();
  return GetSceneObjectManager()->GetObject(id);
}

void WScene2Document::HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (const WPushObjectStateMsgToEditor* msg = WDynamicCast<const WPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg2(msg);
    return;
  }

  SUPER::HandleEngineMessage(pMsg);
}

WTaskGroupID WScene2Document::InternalSaveDocument(AfterSaveCallback callback)
{
  // We need to switch the active layer back to the original content as otherwise the scene will not save itself but instead the active layer's content into itself.
  SetActiveLayer(GetGuid()).LogFailure();
  return SUPER::InternalSaveDocument(callback);
}

void WScene2Document::SendGameWorldToEngine()
{
  SUPER::SendGameWorldToEngine();
  for (auto layer : m_Layers)
  {
    WSceneDocument* pLayer = layer.Value().m_pLayer;
    if (pLayer != this && pLayer != nullptr)
    {
      pLayer->SendDocumentOpenMessage(true);
    }
  }
}

WTransformStatus WScene2Document::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& assetHeader, WBitflags<WTransformFlags> transformFlags)
{
  // We need to wait for layers to be fully loaded before we can transform, i.e. export, a scene.
  W_SUCCEED_OR_RETURN(WaitForEngineStatusLoaded());

  WTempHybridArray<WSceneDocument*, 4> layers;
  GetLoadedLayers(layers);

  for (WSceneDocument* pLayer : layers)
  {
    W_SUCCEED_OR_RETURN(pLayer->WaitForEngineStatusLoaded());
  }
  return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, assetHeader, transformFlags);
}

void WScene2Document::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  W_ASSERT_DEBUG(GetActiveLayer() == m_pDocumentInfo->m_DocumentID, "Ensure that the active layer is the scene itself before calling this function");
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // Add layers as dependencies
  WStringBuilder sTemp;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Key() != this->GetDocumentInfo()->m_DocumentID)
    {
      WConversionUtils::ToString(it.Key(), sTemp);
      pInfo->m_TransformDependencies.Insert(sTemp);
      pInfo->m_ThumbnailDependencies.Insert(sTemp);
    }
  }
}

void WScene2Document::PreventDoubleSelectionChange(bool b)
{
  m_iAllowSelectionChanges = b ? 1 : -1;
}

void WScene2Document::UndoSelection()
{
  bool bAllowRetry = true;

  // if a previous selection can't be restored, try the next one
  while (bAllowRetry)
  {
    if (m_SelectionStack.IsEmpty())
      return;

    if (m_SelectionStack.GetCount() > 1)
      m_SelectionStack.PopBack();
    else
      bAllowRetry = false;

    auto& back = m_SelectionStack.PeekBack();

    m_bStoreSelectionChange = false;
    W_SCOPE_EXIT(m_bStoreSelectionChange = true);

    if (SetActiveLayer(back.m_documentGuid).Failed())
      continue;

    auto* pDoc = WDocumentManager::GetDocumentByGuid(back.m_documentGuid);
    if (pDoc == nullptr)
      continue;

    auto pObjMan = pDoc->GetObjectManager();

    if (back.m_Objects.IsEmpty())
    {
      GetSelectionManager()->Clear();
      return;
    }
    else
    {
      WDeque<const WDocumentObject*> newSel;
      for (const WUuid& guid : back.m_Objects)
      {
        if (auto pDoc = pObjMan->GetObject(guid))
        {
          newSel.PushBack(pDoc);
        }
      }

      if (newSel.IsEmpty())
        continue;

      GetSelectionManager()->SetSelection(newSel);
      return;
    }
  }
}

void WScene2Document::LayerSelectionEventHandler(const WSelectionManagerEvent& e)
{
  const WDocumentObject* pObject = m_pLayerSelection->GetCurrentObject();
  // We can't change the active layer while a transaction is in progress at it will swap out the data storage the transaction is currently modifying.
  if (pObject && !m_pCommandHistory->IsInTransaction() && !m_pSceneCommandHistory->IsInTransaction())
  {
    if (pObject->GetType()->IsDerivedFrom(WGetStaticRTTI<WSceneLayer>()))
    {
      WUuid layerGuid = GetSceneObjectAccessor()->GetByName<WUuid>(pObject, "Layer");
      if (IsLayerLoaded(layerGuid))
      {
        SetActiveLayer(layerGuid).LogFailure();
      }
    }
  }
}

void WScene2Document::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
}

void WScene2Document::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case WCommandHistoryEvent::Type::UndoEnded:
    case WCommandHistoryEvent::Type::RedoEnded:
    case WCommandHistoryEvent::Type::TransactionEnded:
      UpdateLayers();
      break;
    default:
      return;
  }
}

void WScene2Document::SyncAllChildOrders()
{
  SUPER::SyncAllChildOrders();

  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer != nullptr && it.Value().m_pLayer != this)
      it.Value().m_pLayer->SyncAllChildOrders();
  }
}

void WScene2Document::DocumentManagerEventHandler(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentOpened:
    {
      if (WLayerDocument* pLayer = WDynamicCast<WLayerDocument*>(e.m_pDocument))
      {
        if (pLayer->GetMainDocument() != this)
          return;

        WUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        // Either the layer is currently being creating, in which case m_Layers can't be filled yet,
        // or an existing layer's state is toggled either internally by the scene or externally by the editor in which case the layer is known and we must react to it.
        if (m_Layers.TryGetValue(layerGuid, pInfo) && pInfo->m_pLayer != pLayer)
        {
          pInfo->m_pLayer = pLayer;

          WScene2LayerEvent e;
          e.m_Type = WScene2LayerEvent::Type::LayerLoaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    case WDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<WLayerDocument>())
      {
        WUuid layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo = nullptr;
        if (m_Layers.TryGetValue(layerGuid, pInfo))
        {
          pInfo->m_pLayer = nullptr;

          WScene2LayerEvent e;
          e.m_Type = WScene2LayerEvent::Type::LayerUnloaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    default:
      break;
  }
}

void WScene2Document::HandleObjectStateFromEngineMsg2(const WPushObjectStateMsgToEditor* pMsg)
{
  WMap<WUuid, WHybridArray<const WPushObjectStateData*, 1>> layerToChanges;
  for (const WPushObjectStateData& change : pMsg->m_ObjectStates)
  {
    layerToChanges[change.m_LayerGuid].PushBack(&change);
  }

  const WUuid activeLayer = m_ActiveLayerGuid;
  for (auto it : layerToChanges)
  {
    if (SetActiveLayer(it.Key()).Failed())
      continue;

    auto pHistory = GetCommandHistory();

    pHistory->StartTransaction("Pull Object State");

    for (const WPushObjectStateData* pState : it.Value())
    {
      auto pObject = GetObjectManager()->GetObject(pState->m_ObjectGuid);

      if (!pObject)
        continue;

      // set the general transform of the object
      SetGlobalTransform(pObject, WTransform(pState->m_vPosition, pState->m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);

      // if we also have bone transforms, attempt to set them as well
      if (pState->m_BoneTransforms.IsEmpty())
        continue;

      auto pAccessor = GetObjectAccessor();

      // check all components
      for (auto pComponent : pObject->GetChildren())
      {
        auto pComponentType = pComponent->GetType();

        const auto* pBoneManipAttr = pComponentType->GetAttributeByType<WBoneManipulatorAttribute>();

        // we can only apply bone transforms on components that have the WBoneManipulatorAttribute attribute
        if (pBoneManipAttr == nullptr)
          continue;

        auto pBonesProperty = pComponentType->FindPropertyByName(pBoneManipAttr->GetTransformProperty());
        W_ASSERT_DEBUG(pBonesProperty, "Invalid transform property set on WBoneManipulatorAttribute");

        const WExposedParametersAttribute* pExposedParamsAttr = pBonesProperty->GetAttributeByType<WExposedParametersAttribute>();
        W_ASSERT_DEBUG(pExposedParamsAttr, "Expected exposed parameters on WBoneManipulatorAttribute property");

        const WAbstractProperty* pParameterSourceProp = pComponentType->FindPropertyByName(pExposedParamsAttr->GetParametersSource());
        W_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pExposedParamsAttr->GetParametersSource(), pComponentType->GetTypeName());

        // retrieve all the bone keys and values, these will contain the exposed default values, in case a bone has never been overridden before
        WVariantArray boneValues, boneKeys;
        WExposedParameterCommandAccessor proxy(pAccessor, pBonesProperty, pParameterSourceProp);
        proxy.GetValues(pComponent, pBonesProperty, boneValues).AssertSuccess();
        proxy.GetKeys(pComponent, pBonesProperty, boneKeys).AssertSuccess();

        // apply all the new bone transforms
        for (const auto& bone : pState->m_BoneTransforms)
        {
          // ignore bones that are unknown (not exposed somehow)
          WUInt32 idx = boneKeys.IndexOf(bone.Key());
          if (idx == WInvalidIndex)
            continue;

          W_ASSERT_DEBUG(boneValues[idx].GetReflectedType() == WGetStaticRTTI<WExposedBone>(), "Expected an WExposedBone in variant");

          // retrieve the default/previous value of the bone
          const WExposedBone* pDefVal = reinterpret_cast<const WExposedBone*>(boneValues[idx].GetData());

          WExposedBone b;
          b.m_sName = pDefVal->m_sName;     // same as the key
          b.m_sParent = pDefVal->m_sParent; // this is what we don't have and therefore needed to retrieve the default values
          b.m_Transform = bone.Value();

          WVariant var;
          var.CopyTypedObject(&b, WGetStaticRTTI<WExposedBone>());

          proxy.SetValue(pComponent, pBonesProperty, var, bone.Key()).AssertSuccess();
        }

        // found a component/property to apply bones to, so we can stop
        break;
      }
    }

    pHistory->FinishTransaction();
  }
  SetActiveLayer(activeLayer).LogFailure();
}

void WScene2Document::UpdateLayers()
{
  WSet<WUuid> layersBefore;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    layersBefore.Insert(it.Key());
  }
  WSet<WUuid> layersAfter;
  WMap<WUuid, WUuid> LayerToSceneObject;
  const WSceneDocumentSettings* pSettings = GetSettings<WSceneDocumentSettings>();
  for (const WSceneLayerBase* pLayerBase : pSettings->m_Layers)
  {
    if (const WSceneLayer* pLayer = WDynamicCast<const WSceneLayer*>(pLayerBase))
    {
      layersAfter.Insert(pLayer->m_Layer);
      WUuid objectGuid = m_Context.GetObjectGUID(WGetStaticRTTI<WSceneLayer>(), pLayer);
      LayerToSceneObject.Insert(pLayer->m_Layer, objectGuid);
    }
  }

  WSet<WUuid> layersRemoved = layersBefore;
  layersRemoved.Difference(layersAfter);
  for (auto it = layersRemoved.GetIterator(); it.IsValid(); ++it)
  {
    LayerRemoved(it.Key());
  }

  WSet<WUuid> layersAdded = layersAfter;
  layersAdded.Difference(layersBefore);
  for (auto it = layersAdded.GetIterator(); it.IsValid(); ++it)
  {
    LayerAdded(it.Key(), LayerToSceneObject[it.Key()]);
  }
}

void WScene2Document::SendLayerVisibility()
{
  WLayerVisibilityChangedMsgToEngine msg;
  for (auto& layer : m_Layers)
  {
    if (!layer.Value().m_bVisible)
    {
      // We are sending the hidden state because the default state is visible so we have less to send and less often.
      msg.m_HiddenLayers.PushBack(layer.Key());
    }
  }
  SendMessageToEngine(&msg);
}

void WScene2Document::LayerAdded(const WUuid& layerGuid, const WUuid& layerObjectGuid)
{
  LayerInfo info;
  info.m_pLayer = nullptr;
  info.m_bVisible = true;
  info.m_objectGuid = layerObjectGuid;
  m_Layers.Insert(layerGuid, info);

  WScene2LayerEvent e;
  e.m_Type = WScene2LayerEvent::Type::LayerAdded;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  // #TODO Decide whether to load a layer or not (persist as meta data? / user preferences?)
  SetLayerLoaded(layerGuid, true).LogFailure();
}

void WScene2Document::LayerRemoved(const WUuid& layerGuid)
{
  // Make sure removed layer is not active
  if (m_ActiveLayerGuid == layerGuid)
  {
    SetActiveLayer(GetGuid()).LogFailure();
  }

  SetLayerLoaded(layerGuid, false).LogFailure();

  WScene2LayerEvent e;
  e.m_Type = WScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  m_Layers.Remove(layerGuid);
}

void WScene2Document::ActiveLayerGameObjectEventHandler(const WGameObjectEvent& e)
{
  // forward all game object events from the active layer
  m_GameObjectEvents.Broadcast(e);
}

WStatus WScene2Document::CreateLayer(const char* szName, WUuid& out_layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  WStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  const WDocumentTypeDescriptor* pLayerDesc = WDocumentManager::GetDescriptorForDocumentType("Layer");

  WStringBuilder targetDirectory = GetDocumentPath();
  targetDirectory.RemoveFileExtension();
  targetDirectory.Append("_data");
  targetDirectory.AppendPath(szName);
  targetDirectory.Append(".", pLayerDesc->m_sFileExtension.GetData());

  WSceneDocument* pLayerDoc = nullptr;
  if (WOSFile::ExistsFile(targetDirectory))
  {
    WDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = WDynamicCast<WSceneDocument*>(WQtEditorApp::GetSingleton()->OpenDocument(targetDirectory, WDocumentFlags::None, pRoot));

    if (m_Layers.Contains(pLayerDoc->GetGuid()))
    {
      return WStatus(WFmt("A layer named '{}' already exists in this scene.", szName));
    }
  }
  else
  {
    WDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc = WDynamicCast<WSceneDocument*>(WQtEditorApp::GetSingleton()->CreateDocument(targetDirectory, WDocumentFlags::None, pRoot));
    if (!pLayerDoc)
    {
      return WStatus(WFmt("Failed to create new layer '{0}'", targetDirectory));
    }
  }

  WObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  WStringBuilder sTransactionText;
  pAccessor->StartTransaction(WFmt("Add Layer - '{}'", szName).GetText(sTransactionText));
  {
    auto pRoot = m_pSceneObjectManager->GetObject(GetSettingsObject()->GetGuid());
    WInt32 uiCount = 0;
    W_VERIFY(pAccessor->GetCountByName(pRoot, "Layers", uiCount).Succeeded(), "Failed to get layer count.");
    WUuid sceneLayerGuid;
    W_VERIFY(pAccessor->AddObjectByName(pRoot, "Layers", uiCount, WGetStaticRTTI<WSceneLayer>(), sceneLayerGuid).Succeeded(), "Failed to add layer to scene.");
    auto pLayer = pAccessor->GetObject(sceneLayerGuid);
    W_VERIFY(pAccessor->SetValueByName(pLayer, "Layer", pLayerDoc->GetGuid()).Succeeded(), "Failed to set layer GUID.");
  }
  pAccessor->FinishTransaction();

  LayerInfo* pInfo = nullptr;
  W_ASSERT_DEV(m_Layers.Contains(pLayerDoc->GetGuid()), "FinishTransaction should have triggered UpdateLayers and filled m_Layers.");
  // We need to manually emit this here as when the layer doc was loaded DocumentManagerEventHandler will not fire as the document was not added as a layer yet.
  if (m_Layers.TryGetValue(pLayerDoc->GetGuid(), pInfo) && pInfo->m_pLayer != pLayerDoc)
  {
    pInfo->m_pLayer = pLayerDoc;

    WScene2LayerEvent e;
    e.m_Type = WScene2LayerEvent::Type::LayerLoaded;
    e.m_layerGuid = pLayerDoc->GetGuid();
    m_LayerEvents.Broadcast(e);
  }
  out_layerGuid = pLayerDoc->GetGuid();
  return WStatus(W_SUCCESS);
}

WStatus WScene2Document::DeleteLayer(const WUuid& layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  WStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return WStatus("Unknown layer guid. Layer can't be deleted.");
  }

  if (!pInfo->m_objectGuid.IsValid())
  {
    return WStatus("Layer object guid not set, layer object unknown.");
  }

  const WDocumentObject* pObject = GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  if (!pObject)
  {
    return WStatus("Layer object no longer valid.");
  }

  WStringBuilder sName("<Unknown>");
  {
    auto assetInfo = WAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (assetInfo.isValid())
    {
      sName = WPathUtils::GetFileName(assetInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
    else
    {
      return WStatus("Could not resolve layer in WAssetCurator.");
    }
  }

  WObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  WStringBuilder sTransactionText;
  pAccessor->StartTransaction(WFmt("Remove Layer - '{}'", sName).GetText(sTransactionText));
  {
    W_VERIFY(pAccessor->RemoveObject(pObject).Succeeded(), "Failed to remove Layer.");
  }
  pAccessor->FinishTransaction();
  return WStatus(W_SUCCESS);
}

const WUuid& WScene2Document::GetActiveLayer() const
{
  return m_ActiveLayerGuid;
}

WStatus WScene2Document::SetActiveLayer(const WUuid& layerGuid)
{
  W_ASSERT_DEV(!m_pCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");
  W_ASSERT_DEV(!m_pSceneCommandHistory || !m_pSceneCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");

  if (layerGuid == m_ActiveLayerGuid)
    return WStatus(W_SUCCESS);

  m_ActiveLayerGoEvUnsubscriber.Unsubscribe();

  if (layerGuid == GetGuid()) // "Main" layer (this document)
  {
    WDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = WDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
    m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
    m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
    m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = WDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }
  else
  {
    WDocument* pDoc = WDocumentManager::GetDocumentByGuid(layerGuid);
    if (!pDoc)
      return WStatus("Unloaded layer can't be made active.");

    WDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = WDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(pDoc->GetObjectManager()->GetStorage());
    m_pCommandHistory->SwapStorage(pDoc->GetCommandHistory()->GetStorage());
    m_pSelectionManager->SwapStorage(pDoc->GetSelectionManager()->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(pDoc->m_DocumentObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = WDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    WGameObjectDocument* pGoDoc = WDynamicCast<WGameObjectDocument*>(pDoc);
    W_ASSERT_DEBUG(pGoDoc, "");
    pGoDoc->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WScene2Document::ActiveLayerGameObjectEventHandler, this), m_ActiveLayerGoEvUnsubscriber);
  }

  const bool bVisualizers = WVisualizerManager::GetSingleton()->GetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid));

  WVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), false);

  m_ActiveLayerGuid = layerGuid;
  m_pActiveSubDocument = GetLayerDocument(layerGuid);

  {
    WScene2LayerEvent e;
    e.m_Type = WScene2LayerEvent::Type::ActiveLayerChanged;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);
  }
  {
    WSelectionManagerEvent se;
    se.m_pDocument = this;
    se.m_pObject = nullptr;
    se.m_Type = WSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionManager->GetStorage()->m_Events.Broadcast(se);
  }
  {
    WCommandHistoryEvent ce;
    ce.m_pDocument = this;
    ce.m_Type = WCommandHistoryEvent::Type::HistoryChanged;
    m_pCommandHistory->GetStorage()->m_Events.Broadcast(ce);
  }
  {
    WDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = WDocumentEvent::Type::ModifiedChanged;

    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);
  }
  {
    WActiveLayerChangedMsgToEngine msg;
    msg.m_ActiveLayer = layerGuid;
    SendMessageToEngine(&msg);
  }

  WVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), bVisualizers);

  // Set selection to object that contains the active layer
  if (const WDocumentObject* pLayerObject = GetLayerObject(layerGuid))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }
  return WStatus(W_SUCCESS);
}

bool WScene2Document::IsLayerLoaded(const WUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer != nullptr;
  }
  return false;
}

WStatus WScene2Document::SetLayerLoaded(const WUuid& layerGuid, bool bLoaded)
{
  if (GetGameMode() != GameMode::Enum::Off)
    return WStatus("Simulation must be stopped to change a layer's loaded state.");

  if (layerGuid == GetGuid() && !bLoaded)
    return WStatus("Cannot unload the scene itself.");

  // We can't unload the active layer
  if (!bLoaded && m_ActiveLayerGuid == layerGuid)
  {
    WStatus res = SetActiveLayer(GetGuid());
    if (res.Failed())
      return res;
  }

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return WStatus("Unknown layer guid. Layer can't be loaded / unloaded.");
  }

  if ((pInfo->m_pLayer != nullptr) == bLoaded)
    return WStatus(W_SUCCESS);

  if (bLoaded)
  {
    WStringBuilder sAbsPath;
    if (layerGuid == GetGuid())
    {
      sAbsPath = GetDocumentPath();
    }
    else
    {
      auto assetInfo = WAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (assetInfo.isValid())
      {
        sAbsPath = assetInfo->m_pAssetInfo->m_Path;
      }
      else
      {
        return WStatus("Could not resolve layer in WAssetCurator.");
      }
    }

    WDocument* pDoc = nullptr;
    // Pass our root into it to indicate what the parent context of the layer is.
    WDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    if (WDocument* pLayer = WQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, WDocumentFlags::None, pRoot))
    {
      if (layerGuid != GetGuid() && pLayer->GetMainDocument() != this)
      {
        return WStatus("Layer already open in another window.");
      }

      // In case we are responding to e.g. an redo 'Add Layer' the layer is already loaded in the editor but we still want to enforce that the event is fired every time after adding a layer.
      if (pInfo->m_pLayer != pLayer)
      {
        pInfo->m_pLayer = WDynamicCast<WSceneDocument*>(pLayer);

        WScene2LayerEvent e;
        e.m_Type = WScene2LayerEvent::Type::LayerLoaded;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }

      return WStatus(W_SUCCESS);
    }
    else
    {
      return WStatus("Could not load layer, see log for more information.");
    }
  }
  else
  {
    if (pInfo->m_pLayer == nullptr)
      return WStatus(W_SUCCESS);

    // Unload document (save and close)
    WDocumentManager* pManager = pInfo->m_pLayer->GetDocumentManager();
    pManager->CloseDocument(pInfo->m_pLayer);
    pInfo->m_pLayer = nullptr;

    // WScene2LayerEvent e;
    // e.m_Type = WScene2LayerEvent::Type::LayerUnloaded;
    // e.m_layerGuid = layerGuid;
    // m_LayerEvents.Broadcast(e);

    return WStatus(W_SUCCESS);
  }
}

void WScene2Document::GetAllLayers(WDynamicArray<WUuid>& out_layerGuids)
{
  out_layerGuids.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    out_layerGuids.PushBack(it.Key());
  }
}

void WScene2Document::GetLoadedLayers(WDynamicArray<WSceneDocument*>& out_layers) const
{
  out_layers.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer)
    {
      out_layers.PushBack(it.Value().m_pLayer);
    }
  }
}

bool WScene2Document::IsLayerVisible(const WUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_bVisible;
  }
  return false;
}

WStatus WScene2Document::SetLayerVisible(const WUuid& layerGuid, bool bVisible)
{
  LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    if (pInfo->m_bVisible != bVisible)
    {
      pInfo->m_bVisible = bVisible;
      {
        WScene2LayerEvent e;
        e.m_Type = bVisible ? WScene2LayerEvent::Type::LayerVisible : WScene2LayerEvent::Type::LayerInvisible;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }
      SendLayerVisibility();
    }
    return WStatus(W_SUCCESS);
  }
  return WStatus("Unknown layer.");
}

const WDocumentObject* WScene2Document::GetLayerObject(const WUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  }
  return nullptr;
}

WSceneDocument* WScene2Document::GetLayerDocument(const WUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer;
  }
  return nullptr;
}

WGameObjectDocument* WScene2Document::GetRedirectedGameObjectDoc()
{
  if (m_ActiveLayerGuid == GetGuid())
    return this;

  return GetLayerDocument(m_ActiveLayerGuid);
}

bool WScene2Document::IsAnyLayerModified() const
{
  for (auto& layer : m_Layers)
  {
    auto pLayer = layer.Value().m_pLayer;
    if (pLayer && pLayer->IsModified())
      return true;
  }

  return false;
}
