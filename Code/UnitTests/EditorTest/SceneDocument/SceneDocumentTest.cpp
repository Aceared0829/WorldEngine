#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/SceneDocument/SceneDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QMimeData>

static WEditorSceneDocumentTest s_EditorSceneDocumentTest;

const char* WEditorSceneDocumentTest::GetTestName() const
{
  return "Scene Document Tests";
}

void WEditorSceneDocumentTest::SetupSubTests()
{
  AddSubTest("Layer Operations", SubTests::ST_LayerOperations);
  AddSubTest("Prefab Operations", SubTests::ST_PrefabOperations);
  AddSubTest("Component Operations", SubTests::ST_ComponentOperations);
  AddSubTest("Object Property Path", SubTests::ST_ObjectPropertyPath);
}

WResult WEditorSceneDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return W_FAILURE;

  if (WStatus res = WAssetCurator::GetSingleton()->TransformAllAssets(); res.Failed())
  {
    WLog::Error("Asset transform failed: {}", res.GetMessageString());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WEditorSceneDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_pLayer = nullptr;
  m_SceneGuid = WUuid::MakeInvalid();
  m_LayerGuid = WUuid::MakeInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WEditorSceneDocumentTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_LayerOperations:
      LayerOperations();
      break;
    case SubTests::ST_PrefabOperations:
      PrefabOperations();
      break;
    case SubTests::ST_ComponentOperations:
      ComponentOperations();
      break;
    case SubTests::ST_ObjectPropertyPath:
      ObjectPropertyPath();
      break;
  }
  return WTestAppRun::Quit;
}

WResult WEditorSceneDocumentTest::CreateSimpleScene(const char* szSceneName)
{
  WStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<WScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(m_pDoc != nullptr))
      return W_FAILURE;

    W_ANALYSIS_ASSUME(m_pDoc != nullptr);
    m_SceneGuid = m_pDoc->GetGuid();
    ProcessEvents();
    W_TEST_STATUS(m_pDoc->CreateLayer("Layer1", m_LayerGuid));
    m_pLayer = WDynamicCast<WLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    if (!W_TEST_BOOL(m_pLayer != nullptr))
      return W_FAILURE;
  }
  return W_SUCCESS;
}

void WEditorSceneDocumentTest::CloseSimpleScene()
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    WTaskGroupID id = m_pDoc->SaveDocumentAsync(
      [&bSaved](WDocument* pDoc, WStatus res)
      {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(id));
    W_TEST_BOOL(bSaved);
    m_pDoc = nullptr;
    m_pLayer = nullptr;
    m_SceneGuid = WUuid::MakeInvalid();
    m_LayerGuid = WUuid::MakeInvalid();
  }
}

void WEditorSceneDocumentTest::LayerOperations()
{
  WStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath("LayerOperations.WScene");

  WScene2Document* pDoc = nullptr;
  WEventSubscriptionID layerEventsID = 0;
  WTempHybridArray<WScene2LayerEvent, 2> expectedEvents;
  WUuid sceneGuid;
  WUuid layer1Guid;
  WLayerDocument* pLayer1 = nullptr;

  auto TestLayerEvents = [&expectedEvents](const WScene2LayerEvent& e)
  {
    if (W_TEST_BOOL(!expectedEvents.IsEmpty()))
    {
      // If we pass in an invalid guid it's considered fine as we might not know the ID, e.g. when creating a layer.
      W_TEST_BOOL(!expectedEvents[0].m_layerGuid.IsValid() || expectedEvents[0].m_layerGuid == e.m_layerGuid);
      W_TEST_BOOL(expectedEvents[0].m_Type == e.m_Type);
      expectedEvents.RemoveAtAndCopy(0);
    }
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    pDoc = static_cast<WScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(pDoc != nullptr))
      return;

    W_ANALYSIS_ASSUME(pDoc != nullptr);
    sceneGuid = pDoc->GetGuid();
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Create Layer")
  {
    W_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    W_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    W_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerAdded, WUuid()});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerLoaded, WUuid()});
    W_TEST_STATUS(pDoc->CreateLayer("Layer1", layer1Guid));

    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    W_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    W_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    W_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    W_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));
    pLayer1 = WDynamicCast<WLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    W_TEST_BOOL(pLayer1 != nullptr);

    WTempHybridArray<WSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    W_TEST_INT(layers.GetCount(), 2);
    W_TEST_BOOL(layers.Contains(pLayer1));
    W_TEST_BOOL(layers.Contains(pDoc));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Undo/Redo Layer Creation")
  {
    // Undo / redo
    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    W_TEST_STATUS(pDoc->SetActiveLayer(sceneGuid));
    // Initial scene setup exists in the scene undo stack
    const WUInt32 uiInitialUndoStackSize = pDoc->GetCommandHistory()->GetUndoStackSize();
    W_TEST_BOOL(uiInitialUndoStackSize >= 1);
    W_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), uiInitialUndoStackSize);
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    W_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    W_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));

    pLayer1 = WDynamicCast<WLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    W_TEST_BOOL(pLayer1 != nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Save and Close Document")
  {
    bool bSaved = false;
    WTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](WDocument* pDoc, WStatus res)
      {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(id));
    W_TEST_BOOL(bSaved);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reload Document")
  {
    pDoc = static_cast<WScene2Document*>(m_pApplication->m_pEditorApp->OpenDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(pDoc != nullptr))
      return;

    W_ANALYSIS_ASSUME(pDoc != nullptr);
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();

    W_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    W_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    W_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    pLayer1 = WDynamicCast<WLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    WTempHybridArray<WSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    W_TEST_INT(layers.GetCount(), 2);
    W_TEST_BOOL(layers.Contains(pLayer1));
    W_TEST_BOOL(layers.Contains(pDoc));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Toggle Layer Visibility")
  {
    W_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    W_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    W_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    W_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    W_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    W_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));
    W_TEST_BOOL(!pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerVisible, layer1Guid});
    W_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, true));
    W_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Toggle Layer Loaded")
  {
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    W_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));

    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    W_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, false));

    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    W_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, true));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Delete Layer")
  {
    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    W_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));

    expectedEvents.PushBack({WScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    W_TEST_STATUS(pDoc->DeleteLayer(layer1Guid));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Undo/Redo Layer Deletion")
  {
    W_TEST_INT(pDoc->GetCommandHistory()->GetUndoStackSize(), 1);
    W_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), 1);
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    W_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    W_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    W_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({WScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    W_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    WTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](WDocument* pDoc, WStatus res)
      {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(id));
    W_TEST_BOOL(bSaved);
  }
}


void WEditorSceneDocumentTest::PrefabOperations()
{
  if (CreateSimpleScene("PrefabOperations.WScene").Failed())
    return;

  const WDocumentObject* pPrefab1 = nullptr;
  const WDocumentObject* pPrefab2 = nullptr;
  auto pAccessor = m_pDoc->GetObjectAccessor();

  W_TEST_BLOCK(WTestBlock::Enabled, "Drag&Drop Prefabs")
  {
    const char* szSpherePrefab = "{ 195cdef0-45f1-0642-9fea-5dea95057fb9 }";
    pPrefab1 = DropAsset(m_pDoc, szSpherePrefab);
    W_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    W_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));

    // Test undo / redo of prefab creation
    ProcessEvents(1);
    W_TEST_STATUS(m_pDoc->GetCommandHistory()->Undo(1));
    W_TEST_STATUS(m_pDoc->GetCommandHistory()->Redo(1));

    pPrefab2 = DropAsset(m_pDoc, szSpherePrefab, true);
    W_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    W_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Modify Runtime Prefab Exposed Parameters")
  {
    m_pDoc->GetSelectionManager()->SetSelection(pPrefab1);

    const WDocumentObject* pPrefabComponent = pAccessor->GetChildObjectByName(pPrefab1, "Components", 0);
    const WAbstractProperty* pPropExposedParameters = pPrefabComponent->GetType()->FindPropertyByName("Parameters");
    const auto* pAttrib = pPropExposedParameters->GetAttributeByType<WExposedParametersAttribute>();
    const WAbstractProperty* pPropParameterSource = pPrefabComponent->GetType()->FindPropertyByName(pAttrib->GetParametersSource());

    // Proxy exposed parameters array with exposed parameter type
    WUniquePtr<WExposedParameterCommandAccessor> pProxy = W_DEFAULT_NEW(WExposedParameterCommandAccessor, pAccessor, pPropExposedParameters, pPropParameterSource);
    WUniquePtr<WExposedParametersAsTypeCommandAccessor> pTypeProxy = W_DEFAULT_NEW(WExposedParametersAsTypeCommandAccessor, pProxy.Borrow());

    WTempHybridArray<WPropertySelection, 1> selection;
    selection.PushBack({pPrefabComponent, WVariant()});
    const WRTTI* pCommonType = pProxy->GetCommonExposedParamsType(selection);
    W_TEST_BOOL(pCommonType->GetTypeName().StartsWith("WExposedParameters_"));
    const WAbstractProperty* pPropSortingDepthOffset = pCommonType->FindPropertyByName("SortingDepthOffset");
    const WAbstractProperty* pPropColor = pCommonType->FindPropertyByName("Color");
    const WAbstractProperty* pPropMaterial = pCommonType->FindPropertyByName("Material");

    // Create default state on exposed parameter type proxy. This now thinks it's working on a type with fields (the exposed parameters)
    WDefaultObjectState defaultState(pCommonType, pTypeProxy.Borrow(), selection);
    W_TEST_STRING(defaultState.GetStateProviderName(), "Exposed Parameters");
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropSortingDepthOffset));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropColor));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropMaterial));

    // Make modifications on the proxy type and observe default state change
    {
      pTypeProxy->StartTransaction("Modify Exposed Parameters");
      W_TEST_STATUS(pTypeProxy->SetValue(pPrefabComponent, pPropSortingDepthOffset, 0.1f));
      pTypeProxy->FinishTransaction();
    }

    W_TEST_BOOL(!defaultState.IsDefaultValue(pPropSortingDepthOffset));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropColor));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropMaterial));

    {
      pTypeProxy->StartTransaction("Modify Exposed Parameters");
      W_TEST_STATUS(defaultState.RevertProperty(pPropSortingDepthOffset));
      W_TEST_STATUS(pTypeProxy->SetValue(pPrefabComponent, pPropColor, WColor::AliceBlue));


      W_TEST_STATUS(defaultState.RevertProperty(pPropMaterial));
      pTypeProxy->FinishTransaction();
    }

    W_TEST_BOOL(defaultState.IsDefaultValue(pPropSortingDepthOffset));
    W_TEST_BOOL(!defaultState.IsDefaultValue(pPropColor));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropMaterial));

    {
      pTypeProxy->StartTransaction("Modify Exposed Parameters");
      W_TEST_STATUS(pTypeProxy->SetValue(pPrefabComponent, pPropMaterial, "{ 7c71fc2d-2a1a-4938-9305-f3a0f26d73cc }")); // Lit material
      W_TEST_STATUS(defaultState.RevertProperty(pPropColor));
      pTypeProxy->FinishTransaction();
    }

    W_TEST_BOOL(defaultState.IsDefaultValue(pPropSortingDepthOffset));
    W_TEST_BOOL(defaultState.IsDefaultValue(pPropColor));
    W_TEST_BOOL(!defaultState.IsDefaultValue(pPropMaterial));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Create Nodes and check default state")
  {
    const char* szSphereMesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }";
    const WDocumentObject* pSphere1 = DropAsset(m_pDoc, szSphereMesh);
    const WDocumentObject* pSphere2 = DropAsset(m_pDoc, szSphereMesh);

    pAccessor->StartTransaction("Modify objects");
    W_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "Name", "Sphere1"));
    W_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalPosition", WVec3(1.0f, 0.0f, 0.0f)));
    W_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalRotation", WQuat(1.0f, 0.0f, 0.0f, 0.0f)));
    W_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalScaling", WVec3(1.0f, 2.0f, 3.0f)));
    W_TEST_STATUS(pAccessor->InsertValueByName(pSphere1, "Tags", "SkyLight", -1));
    const WDocumentObject* pMeshComponent = pAccessor->GetObject(pAccessor->GetByName<WVariantArray>(pSphere1, "Components")[0].Get<WUuid>());
    W_TEST_STATUS(pAccessor->InsertValueByName(pMeshComponent, "Materials", "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }", 0));

    WUuid pSphereRef;
    W_TEST_STATUS(pAccessor->AddObjectByName(pSphere1, "Components", -1, WGetStaticRTTI<WSphereReflectionProbeComponent>(), pSphereRef));

    pAccessor->FinishTransaction();

    {
      // Check that modifications above changed properties from their default state.
      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pSphere1, WVariant()});
      WDefaultObjectState defaultState(pSphere1->GetType(), pAccessor, selection);
      W_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");

      W_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("Components"));

      // Does default state match that of pSphere2 which is unmodified?
      auto MatchesDefaultValue = [&](WDefaultObjectState& ref_defaultState, const char* szProperty)
      {
        WVariant defaultValue = ref_defaultState.GetDefaultValue(szProperty);
        WVariant sphere2value;
        W_TEST_STATUS(pAccessor->GetValueByName(pSphere2, szProperty, sphere2value));
        W_TEST_BOOL(defaultValue == sphere2value);
      };

      MatchesDefaultValue(defaultState, "Name");
      MatchesDefaultValue(defaultState, "LocalPosition");
      MatchesDefaultValue(defaultState, "LocalRotation");
      MatchesDefaultValue(defaultState, "LocalScaling");
      MatchesDefaultValue(defaultState, "Tags");
    }

    {
      // pSphere2 should be unmodified except for the component array.
      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pSphere2, WVariant()});
      WDefaultObjectState defaultState(pSphere2->GetType(), pAccessor, selection);
      W_TEST_BOOL(defaultState.IsDefaultValue("Name"));
      W_TEST_BOOL(defaultState.IsDefaultValue("LocalPosition"));
      W_TEST_BOOL(defaultState.IsDefaultValue("LocalRotation"));
      W_TEST_BOOL(defaultState.IsDefaultValue("LocalScaling"));
      W_TEST_BOOL(defaultState.IsDefaultValue("Tags"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Multi-selection should not be default if one in the selection is not.
      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pSphere1, WVariant()});
      selection.PushBack({pSphere2, WVariant()});
      WDefaultObjectState defaultState(pSphere1->GetType(), pAccessor, selection);
      W_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      W_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Default state object array
      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pSphere1, WVariant()});
      WDefaultContainerState defaultState(pSphere1->GetType(), pAccessor, selection, "Components");
      W_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      W_TEST_BOOL(defaultState.GetDefaultContainer() == WVariantArray());
      W_TEST_BOOL(defaultState.GetDefaultElement(0) == WUuid());
      W_TEST_BOOL(!defaultState.IsDefaultContainer());
      // We currently do not supporting reverting an index of a non-value type container. Thus, they are always the default state.
      W_TEST_BOOL(defaultState.IsDefaultElement(0));
      W_TEST_BOOL(defaultState.IsDefaultElement(1));
    }

    {
      // Default state value array
      WTempHybridArray<WPropertySelection, 1> selection;
      selection.PushBack({pMeshComponent, WVariant()});
      WDefaultContainerState defaultState(pMeshComponent->GetType(), pAccessor, selection, "Materials");
      W_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      W_TEST_BOOL(defaultState.GetDefaultContainer() == WVariantArray());
      W_TEST_BOOL(defaultState.GetDefaultElement(0) == "");
      W_TEST_BOOL(!defaultState.IsDefaultContainer());
      W_TEST_BOOL(!defaultState.IsDefaultElement(0));

      WDefaultObjectState defaultObjectState(pMeshComponent->GetType(), pAccessor, selection);
      W_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      W_TEST_BOOL(defaultObjectState.GetDefaultValue("Materials") == WVariantArray());
      W_TEST_BOOL(!defaultObjectState.IsDefaultValue("Materials"));
    }

    WDeque<const WDocumentObject*> selection;
    selection.PushBack(pSphere1);
    selection.PushBack(pSphere2);
    m_pDoc->GetSelectionManager()->SetSelection(selection);
  }

  WUuid prefabGuid;
  const WDocumentObject* pPrefab3 = nullptr;
  W_TEST_BLOCK(WTestBlock::Enabled, "Create Prefab from Selection")
  {
    // ProcessEvents(999999999);

    WStringBuilder sPrefabName;
    sPrefabName = m_sProjectPath;
    sPrefabName.AppendPath("Spheres.WPrefab");
    W_TEST_BOOL(m_pDoc->CreatePrefabDocumentFromSelection(sPrefabName, WGetStaticRTTI<WGameObject>(), {}, {}, [](WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>&) { /* do nothing */ }).Succeeded());
    m_pDoc->ScheduleSendObjectSelection();
    pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
    W_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    W_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuid));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Move Prefabs to Layer")
  {
    W_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    W_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    W_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());

    // Copy & paste should retain the order in the tree view, not the selection array so we push the elements in a random order here.
    WDeque<const WDocumentObject*> assets;
    assets.PushBack(pPrefab1);
    assets.PushBack(pPrefab2);
    assets.PushBack(pPrefab3);
    WDeque<const WDocumentObject*> newObjects;

    MoveObjectsToLayer(m_pDoc, assets, m_LayerGuid, newObjects);

    W_TEST_BOOL(m_pDoc->GetActiveLayer() == m_SceneGuid);
    W_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab1->GetGuid()) == nullptr);
    W_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab2->GetGuid()) == nullptr);
    W_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab3->GetGuid()) == nullptr);

    W_TEST_INT(newObjects.GetCount(), assets.GetCount());
    pPrefab1 = newObjects[0];
    pPrefab2 = newObjects[1];
    pPrefab3 = newObjects[2];

    W_TEST_STATUS(m_pDoc->SetActiveLayer(m_LayerGuid));

    W_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    W_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    W_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pLayer->GetObjectManager());

    W_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    W_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    W_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    W_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
    W_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    WUuid prefabGuidOut;
    W_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuidOut));
    W_TEST_BOOL(prefabGuid == prefabGuidOut);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Change Prefab Type")
  {
    WVariant oldIndex = pPrefab1->GetPropertyIndex();
    WTempHybridArray<const WDocumentObject*, 8> selection;
    {
      selection.PushBack(pPrefab1);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab1 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      W_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
      W_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
      W_TEST_BOOL(oldIndex == pPrefab1->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab2->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab2);
      m_pDoc->ConvertToEnginePrefab(selection);
      pPrefab2 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      W_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
      W_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
      W_TEST_BOOL(oldIndex == pPrefab2->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab3->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab3);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      W_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid()));
      WUuid prefabGuidOut;
      W_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      W_TEST_BOOL(prefabGuid == prefabGuidOut);
      W_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
    }
  }

  auto IsObjectDefault = [&](const WDocumentObject* pChild)
  {
    WTempHybridArray<WPropertySelection, 1> selection;
    selection.PushBack({pChild, WVariant()});
    WDefaultObjectState defaultState(pChild->GetType(), pAccessor, selection);
    // The root node of the prefab is not actually part of the prefab in the sense that it is just the container and does not actually exist in the prefab itself.
    const char* szExpectedProvider = pChild == pPrefab3 ? "Attribute" : "Prefab";
    W_TEST_STRING(defaultState.GetStateProviderName(), szExpectedProvider);

    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Hidden | WPropertyFlags::ReadOnly))
        continue;

      W_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "Modify Editor Prefab")
  {
    WVariant oldIndex = pPrefab3->GetPropertyIndex();
    const WAbstractProperty* pProp = pPrefab3->GetType()->FindPropertyByName("Children");
    const WAbstractProperty* pCompProp = pPrefab3->GetType()->FindPropertyByName("Components");

    // ProcessEvents(999999999);

    CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);

    // ProcessEvents(999999999);
    {
      m_pDoc->UpdatePrefabs();
      // Update prefabs replaces object instances with new ones with the same IDs so the old ones are in the undo history now.
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Remove part of the prefab
      WTempHybridArray<WVariant, 16> values;
      W_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      W_TEST_INT(values.GetCount(), 2);
      const WDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<WUuid>());
      const WDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<WUuid>());
      pAccessor->StartTransaction("Delete child0");
      pAccessor->RemoveObject(pChild1).AssertSuccess();
      pAccessor->FinishTransaction();
      W_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 1);
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert prefab
      WTempHybridArray<const WDocumentObject*, 2> selection;
      selection.PushBack(pPrefab3);
      m_pDoc->RevertPrefabs(selection);

      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      WUuid prefabGuidOut;
      W_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      W_TEST_BOOL(prefabGuid == prefabGuidOut);
      W_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
      W_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
    }

    {
      // Modify the prefab
      WTempHybridArray<WVariant, 16> values;
      W_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      W_TEST_INT(values.GetCount(), 2);
      const WDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<WUuid>());
      const WDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<WUuid>());

      pAccessor->StartTransaction("Modify Prefab");
      WUuid compGuid;
      W_TEST_STATUS(pAccessor->AddObjectByName(pChild0, "Components", -1, WRTTI::FindTypeByName("WBeamComponent"), compGuid));
      const WDocumentObject* pComp = pAccessor->GetObject(compGuid);

      const WDocumentObject* pChild1Comp = pAccessor->GetChildObjectByName(pChild1, "Components", 0);
      W_TEST_STATUS(pAccessor->RemoveObject(pChild1Comp));
      pAccessor->FinishTransaction();

      W_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
      W_TEST_INT(pAccessor->GetCount(pChild0, pCompProp), 3);
      W_TEST_INT(pAccessor->GetCount(pChild1, pCompProp), 0);

      // Check default states
      {
        WTempHybridArray<WPropertySelection, 1> selection;
        selection.PushBack({pChild0, WVariant()});
        WDefaultContainerState defaultObjectState(pChild0->GetType(), pAccessor, selection, "Components");
        W_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        W_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        WTempHybridArray<WPropertySelection, 1> selection;
        selection.PushBack({pChild1, WVariant()});
        WDefaultContainerState defaultObjectState(pChild1->GetType(), pAccessor, selection, "Components");
        W_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        W_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        WTempHybridArray<WPropertySelection, 1> selection;
        selection.PushBack({pComp, WVariant()});
        WDefaultObjectState defaultObjectState(pComp->GetType(), pAccessor, selection);
        W_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      }
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert via default state
      const WDocumentObject* pChild1 = pAccessor->GetChildObjectByName(pPrefab3, "Children", 0);
      const WDocumentObject* pChild2 = pAccessor->GetChildObjectByName(pPrefab3, "Children", 1);
      {
        WTempHybridArray<WPropertySelection, 1> selection;
        selection.PushBack({pChild1, WVariant()});
        selection.PushBack({pChild2, WVariant()});
        WDefaultContainerState defaultState(pChild1->GetType(), pAccessor, selection, "Components");

        pAccessor->StartTransaction("Revert children");
        defaultState.RevertContainer().AssertSuccess();
        pAccessor->FinishTransaction();
      }
    }

    {
      // Verify prefab was reverted
      WTempHybridArray<WVariant, 16> values;
      W_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      W_TEST_INT(values.GetCount(), 2);
      const WDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<WUuid>());
      const WDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<WUuid>());

      values.Clear();
      W_TEST_STATUS(pAccessor->GetValuesByName(pChild0, "Components", values));
      W_TEST_INT(values.GetCount(), 2);

      W_TEST_STATUS(pAccessor->GetValuesByName(pChild1, "Components", values));
      W_TEST_INT(values.GetCount(), 1);

      CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);
    }
  }

  ProcessEvents(10);
  CloseSimpleScene();
}

void WEditorSceneDocumentTest::ComponentOperations()
{
  if (CreateSimpleScene("ComponentOperations.WScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  const WDocumentObject* pRoot = CreateGameObject(m_pDoc);

  WDeque<const WDocumentObject*> selection;
  selection.PushBack(pRoot);
  m_pDoc->GetSelectionManager()->SetSelection(selection);

  auto CreateComponent = [&](const WRTTI* pType, const WDocumentObject* pParent) -> const WDocumentObject*
  {
    WUuid compGuid;
    W_TEST_STATUS(pAccessor->AddObjectByName(pParent, "Components", -1, pType, compGuid));
    return pAccessor->GetObject(compGuid);
  };

  auto IsObjectDefault = [&](const WDocumentObject* pChild)
  {
    WTempHybridArray<WPropertySelection, 1> selection;
    selection.PushBack({pChild, WVariant()});
    WDefaultObjectState defaultState(pChild->GetType(), pAccessor, selection);

    WTempHybridArray<const WAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Hidden | WPropertyFlags::ReadOnly))
        continue;

      WVariant defaultValue = defaultState.GetDefaultValue(pProp);
      W_TEST_BOOL(WDefaultStateProvider::DoesVariantMatchProperty(defaultValue, pProp));
      WVariant currentValue;
      W_TEST_STATUS(pAccessor->GetValue(pChild, pProp, currentValue));
      W_TEST_BOOL(WDefaultStateProvider::DoesVariantMatchProperty(currentValue, pProp));
      W_TEST_BOOL(defaultValue == currentValue);
      W_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  WDynamicArray<const WRTTI*> componentTypes;
  WRTTI::ForEachDerivedType<WComponent>([&](const WRTTI* pRtti)
    { componentTypes.PushBack(pRtti); });

  WSet<const WRTTI*> blacklist;
  // The scene already has one and the code asserts otherwise. There needs to be a general way of preventing two settings components from existing at the same time.
  blacklist.Insert(WRTTI::FindTypeByName("WSkyLightComponent"));

  pAccessor->StartTransaction("Modify objects");

  for (auto pType : componentTypes)
  {
    if (pType->GetTypeFlags().IsSet(WTypeFlags::Abstract) || blacklist.Contains(pType))
      continue;

    auto pComp = CreateComponent(pType, pRoot);

    CheckHierarchy(pAccessor, pComp, IsObjectDefault);
  }

  pAccessor->FinishTransaction();
  ProcessEvents(10);

  W_TEST_BLOCK(WTestBlock::Enabled, "Re-open document")
  {
    WUuid layerGuid = m_LayerGuid;
    CloseSimpleScene();
    ProcessEvents(10);

    m_pDoc = WDynamicCast<WScene2Document*>(OpenDocument("ComponentOperations.WScene"));
    W_TEST_BOOL(m_pDoc != nullptr);
    m_SceneGuid = m_pDoc->GetGuid();

    WTempHybridArray<WUuid, 2> layers;
    m_pDoc->GetAllLayers(layers);

    W_TEST_BOOL(layers.Contains(layerGuid));
    m_LayerGuid = layerGuid;

    W_TEST_BOOL(m_pDoc->SetLayerLoaded(m_LayerGuid, true).Succeeded());
    m_pLayer = WDynamicCast<WLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    W_TEST_BOOL(m_pLayer != nullptr);
  }

  CloseSimpleScene();
}


void WEditorSceneDocumentTest::ObjectPropertyPath()
{
  if (CreateSimpleScene("ObjectPropertyPath.WScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  auto CreateComponent = [&](const WDocumentObject* pParent) -> const WDocumentObject*
  {
    pAccessor->StartTransaction("AddComponent"_wsv);
    WUuid compGuid;
    W_TEST_STATUS(pAccessor->AddObjectByName(pParent, "Components", -1, WRTTI::FindTypeByName("WDecalComponent"), compGuid));
    const WDocumentObject* pComp = pAccessor->GetObject(compGuid);
    W_TEST_STATUS(pAccessor->InsertValueByName(pComp, "Decals", "", 0));
    pAccessor->FinishTransaction();
    return pComp;
  };

  const WDocumentObject* pRoot = CreateGameObject(m_pDoc, nullptr, "Root"_wsv);
  const WDocumentObject* pDummy = CreateGameObject(m_pDoc, pRoot, "Dummy"_wsv);
  const WDocumentObject* pC1 = CreateGameObject(m_pDoc, pDummy, "C"_wsv);
  const WDocumentObject* pComp1 = CreateComponent(pC1);

  const WDocumentObject* pA = CreateGameObject(m_pDoc, pRoot, "A"_wsv);
  const WDocumentObject* pB = CreateGameObject(m_pDoc, pA, "B"_wsv);
  const WDocumentObject* pC2 = CreateGameObject(m_pDoc, pB, "C"_wsv);
  const WDocumentObject* pComp2 = CreateComponent(pC2);

  W_TEST_BLOCK(WTestBlock::Enabled, "GameObject property")
  {
    WObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    WPropertyReference propertyRef{pC2->GetGuid(), pC2->GetType()->FindPropertyByName("Active"_wsv)};

    WStringBuilder sObjectSearchSequence;
    WStringBuilder sComponentType;
    WStringBuilder sPropertyPath;
    W_TEST_STATUS(WObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    W_TEST_STRING(sObjectSearchSequence, "A/B/C");
    W_TEST_BOOL(sComponentType.IsEmpty());
    W_TEST_STRING(sPropertyPath, "Active");

    WTempHybridArray<WPropertyReference, 2> properties;
    W_TEST_BOOL(WObjectPropertyPath::ResolvePath(context, properties, "A/B/D", "", sPropertyPath).Failed());                                    // Path does not exist.
    W_TEST_BOOL(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "WPointLightComponent", sPropertyPath).Failed()); // Component does not exist.
    W_TEST_BOOL(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "", "Bla").Failed());                              // Property does not exist.
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "", sPropertyPath));
    W_TEST_INT(properties.GetCount(), 1);
    W_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, "C", "", sPropertyPath)); // ambiguous target
    W_TEST_INT(properties.GetCount(), 2);
    WPropertyReference propertyRef2{pC1->GetGuid(), pC2->GetType()->FindPropertyByName("Active"_wsv)};
    W_TEST_BOOL(properties[0] == propertyRef2);
    W_TEST_BOOL(properties[1] == propertyRef);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Component member property")
  {
    WObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    WPropertyReference propertyRef{pComp2->GetGuid(), pComp2->GetType()->FindPropertyByName("Color"_wsv)};

    WStringBuilder sObjectSearchSequence;
    WStringBuilder sComponentType;
    WStringBuilder sPropertyPath;
    W_TEST_STATUS(WObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    W_TEST_STRING(sObjectSearchSequence, "A/B/C");
    W_TEST_STRING(sComponentType, "WDecalComponent");
    W_TEST_STRING(sPropertyPath, "Color");

    WTempHybridArray<WPropertyReference, 2> properties;
    W_TEST_BOOL(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, "Bla").Failed()); // Property does not exist.
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, sPropertyPath));
    W_TEST_INT(properties.GetCount(), 1);
    W_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, "C", sComponentType, sPropertyPath)); // ambiguous target
    W_TEST_INT(properties.GetCount(), 2);
    WPropertyReference propertyRef2{pComp1->GetGuid(), pComp1->GetType()->FindPropertyByName("Color"_wsv)};
    W_TEST_BOOL(properties[0] == propertyRef2);
    W_TEST_BOOL(properties[1] == propertyRef);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Component array property")
  {
    WObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    WPropertyReference propertyRef{pComp2->GetGuid(), pComp2->GetType()->FindPropertyByName("Decals"_wsv), 0};

    WStringBuilder sObjectSearchSequence;
    WStringBuilder sComponentType;
    WStringBuilder sPropertyPath;
    W_TEST_STATUS(WObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    W_TEST_STRING(sObjectSearchSequence, "A/B/C");
    W_TEST_STRING(sComponentType, "WDecalComponent");
    W_TEST_STRING(sPropertyPath, "Decals[0]");

    WTempHybridArray<WPropertyReference, 2> properties;
    W_TEST_BOOL(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, "Decals[1]").Failed()); // Index out of range.
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, sPropertyPath));
    W_TEST_INT(properties.GetCount(), 1);
    W_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    W_TEST_STATUS(WObjectPropertyPath::ResolvePath(context, properties, "C", sComponentType, sPropertyPath)); // ambiguous target
    W_TEST_INT(properties.GetCount(), 2);
    WPropertyReference propertyRef2{pComp1->GetGuid(), pComp1->GetType()->FindPropertyByName("Decals"_wsv), 0};
    W_TEST_BOOL(properties[0] == propertyRef2);
    W_TEST_BOOL(properties[1] == propertyRef);
  }

  CloseSimpleScene();
}

void WEditorSceneDocumentTest::CheckHierarchy(WObjectAccessorBase* pAccessor, const WDocumentObject* pRoot, WDelegate<void(const WDocumentObject* pChild)> functor)
{
  WDeque<const WDocumentObject*> objects;
  objects.PushBack(pRoot);
  while (!objects.IsEmpty())
  {
    const WDocumentObject* pCurrent = objects[0];
    objects.PopFront();
    {
      functor(pCurrent);
    }

    for (auto pChild : pCurrent->GetChildren())
    {
      objects.PushBack(pChild);
    }
  }
}
