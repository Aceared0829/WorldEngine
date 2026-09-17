#include <EditorTest/EditorTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/MaterialDocument/MaterialDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <QBackingStore>
#include <QMimeData>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

static WMaterialDocumentTest s_MaterialDocumentTest;

const char* WMaterialDocumentTest::GetTestName() const
{
  return "Material Document Tests";
}

void WMaterialDocumentTest::SetupSubTests()
{
  AddSubTest("Create New Material FromShader", SubTests::ST_CreateNewMaterialFromShader);
  AddSubTest("Create New Material FromBase", SubTests::ST_CreateNewMaterialFromBase);
  AddSubTest("Create New Material FromVSE", SubTests::ST_CreateNewMaterialFromVSE);
}

WResult WMaterialDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return W_FAILURE;

  if (WStatus res = WAssetCurator::GetSingleton()->TransformAllAssets(); res.Failed())
  {
    WLog::Error("Asset transform failed: {}", res.GetMessageString());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WMaterialDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_MaterialGuid = WUuid::MakeInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WMaterialDocumentTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_CreateNewMaterialFromShader:
      CreateMaterialFromShader();
      break;
    case SubTests::ST_CreateNewMaterialFromBase:
      CreateMaterialFromBase();
      break;
    case SubTests::ST_CreateNewMaterialFromVSE:
      CreateMaterialFromVSE();
      break;
  }
  return WTestAppRun::Quit;
}

WResult WMaterialDocumentTest::CreateMaterial(const char* szSceneName)
{
  WStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  W_TEST_BLOCK(WTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow));
    if (!W_TEST_BOOL(m_pDoc != nullptr))
      return W_FAILURE;

    W_ANALYSIS_ASSUME(m_pDoc != nullptr);
    m_MaterialGuid = m_pDoc->GetGuid();
    ProcessEvents();
  }
  return W_SUCCESS;
}

void WMaterialDocumentTest::CloseMaterial()
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
    m_MaterialGuid = WUuid::MakeInvalid();
  }
}

const WDocumentObject* WMaterialDocumentTest::GetShaderProperties(const WDocumentObject* pMaterialProperties)
{
  auto pAccessor = m_pDoc->GetObjectAccessor();
  WVariant vChildGuild;
  W_TEST_STATUS(pAccessor->GetValueByName(pMaterialProperties, "ShaderProperties", vChildGuild));
  W_TEST_BOOL(vChildGuild.IsValid() && vChildGuild.CanConvertTo<WUuid>());
  WUuid childGuild = vChildGuild.Get<WUuid>();
  return pAccessor->GetObject(childGuild);
}

void WMaterialDocumentTest::CaptureMaterialImage()
{
  WQtEngineDocumentWindow* pWindow = qobject_cast<WQtEngineDocumentWindow*>(WQtDocumentWindow::FindWindowByDocument(m_pDoc));
  if (!W_TEST_BOOL(pWindow != nullptr))
    return;

  W_ANALYSIS_ASSUME(pWindow != nullptr);
  auto viewWidgets = pWindow->GetViewWidgets();

  if (!W_TEST_BOOL(!viewWidgets.IsEmpty()))
    return;

  WQtEngineViewWidget::InteractionContext ctxt;
  ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
  WQtEngineViewWidget::SetInteractionContext(ctxt);

  viewWidgets[0]->m_pViewConfig->m_RenderMode = WViewRenderMode::DiffuseColor;
  viewWidgets[0]->m_pViewConfig->m_Perspective = WSceneViewPerspective::Perspective;
  viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

  WActionContext ctx2;
  ctx2.m_pDocument = m_pDoc;
  ctx2.m_pWindow = viewWidgets[0];

  WActionManager::ExecuteAction(nullptr, "View.SkyBox", ctx2, false).AssertSuccess();
  ProcessEvents();

  WSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ForceNoFallbackAcquisition";
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  msg.m_sWhatToDo = "ReloadResources";
  msg.m_sPayload = "ReloadAllResources";
  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

  ProcessEvents();
  WaitFrames(8);

  W_TEST_BOOL(CaptureImage(pWindow, "MatFromShader").Succeeded());
  W_TEST_IMAGE(1, 100);
}

void WMaterialDocumentTest::CreateMaterialFromShader()
{
  if (CreateMaterial("CreateMaterialFromShader.WMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();
  const WDocumentObject* pProperties = m_pDoc->GetSelectionManager()->GetCurrentObject();

  pAccessor->StartTransaction("Change Property 'Shader Mode'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 1));
  pAccessor->FinishTransaction();

  ProcessEvents();

  pAccessor->StartTransaction("Change Property 'Shader'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "Shader", "Shaders/Materials/DefaultMaterial.WShader"));
  pAccessor->FinishTransaction();

  ProcessEvents();

  const WDocumentObject* pShaderProperties = GetShaderProperties(pProperties);
  pAccessor->StartTransaction("Change Property 'SHADING MODE'");
  W_TEST_STATUS(pAccessor->SetValueByName(pShaderProperties, "SHADING_MODE", 1));
  pAccessor->FinishTransaction();

  CaptureMaterialImage();

  CloseMaterial();
}


void WMaterialDocumentTest::CreateMaterialFromBase()
{
  if (CreateMaterial("CreateMaterialFromBase.WMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();
  const WDocumentObject* pProperties = m_pDoc->GetSelectionManager()->GetCurrentObject();

  pAccessor->StartTransaction("Change Property 'Shader Mode'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 0));
  pAccessor->FinishTransaction();

  ProcessEvents();

  pAccessor->StartTransaction("Change Property 'BaseMaterial'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "BaseMaterial", "{ 05af8d07-0b38-44a6-8d50-49731ae2625d }"));
  pAccessor->FinishTransaction();

  ProcessEvents();

  CaptureMaterialImage();

  CloseMaterial();
}

void WMaterialDocumentTest::CreateMaterialFromVSE()
{
  if (CreateMaterial("CreateMaterialFromVSE.WMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();
  auto pHistory = m_pDoc->GetCommandHistory();
  const WDocumentObject* pProperties = m_pDoc->GetSelectionManager()->GetCurrentObject();

  ProcessEvents();

  {
    WTestLogInterface log;
    WTestLogSystemScope logSystemScope(&log, true);
    log.ExpectMessage("Visual Shader graph is empty", WLogMsgType::ErrorMsg, 1);

    pAccessor->StartTransaction("Change Property 'Shader Mode'");
    W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 2));
    pAccessor->FinishTransaction();
  }
  ProcessEvents();

  WUuid materialOutputGuid = WUuid::MakeUuid();
  pAccessor->StartTransaction("Add Node");
  {
    WAddObjectCommand cmd;
    cmd.m_pType = WRTTI::FindTypeByName("ShaderNode::MaterialOutput");
    cmd.m_NewObjectGuid = materialOutputGuid;
    cmd.m_Index = -1;

    W_TEST_STATUS(pHistory->AddCommand(cmd));

    WMoveNodeCommand move;
    move.m_Object = cmd.m_NewObjectGuid;
    move.m_NewPos = {0, 0};
    W_TEST_STATUS(pHistory->AddCommand(move));
  }
  pAccessor->FinishTransaction();

  ProcessEvents();

  WUuid parameterColorGuid = WUuid::MakeUuid();
  pAccessor->StartTransaction("Add Node");
  {
    WAddObjectCommand cmd;
    cmd.m_pType = WRTTI::FindTypeByName("ShaderNode::ParameterColor");
    cmd.m_NewObjectGuid = parameterColorGuid;
    cmd.m_Index = -1;

    W_TEST_STATUS(pHistory->AddCommand(cmd));

    WMoveNodeCommand move;
    move.m_Object = cmd.m_NewObjectGuid;
    move.m_NewPos = {-200, 60};
    W_TEST_STATUS(pHistory->AddCommand(move));
  }
  pAccessor->FinishTransaction();

  ProcessEvents();

  {
    pAccessor->StartTransaction("Connect Nodes");
    auto pNodeManager = static_cast<WVisualGraphObjectManager*>(m_pDoc->GetObjectManager());
    auto pMateriaOutput = pAccessor->GetObject(materialOutputGuid);
    auto pParameterColor = pAccessor->GetObject(parameterColorGuid);

    const WVisualGraphPin* pValue = pNodeManager->GetOutputPinByName(pParameterColor, "Value");
    const WVisualGraphPin* pBaseColor = pNodeManager->GetInputPinByName(pMateriaOutput, "BaseColor");
    if (W_TEST_BOOL(pValue && pBaseColor))
    {
      W_TEST_STATUS(WNodeCommands::AddAndConnectCommand(pHistory, pNodeManager->GetConnectionType(), *pValue, *pBaseColor));
    }
    pAccessor->FinishTransaction();
  }

  // Bug: Shader won't update until transformed and shader mode is switched back and forth.
  W_TEST_STATUS(m_pDoc->SaveDocument());
  WAssetCurator::GetSingleton()->TransformAsset(m_MaterialGuid, WTransformFlags::ForceTransform);
  ProcessEvents();

  pAccessor->StartTransaction("Change Property 'Shader Mode'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 0));
  pAccessor->FinishTransaction();

  ProcessEvents();

  pAccessor->StartTransaction("Change Property 'Shader Mode'");
  W_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 2));
  pAccessor->FinishTransaction();

  ProcessEvents();

  const WDocumentObject* pShaderProperties = GetShaderProperties(pProperties);
  pAccessor->StartTransaction("Change Properties");
  W_TEST_STATUS(pAccessor->SetValueByName(pShaderProperties, "SHADING_MODE", 1));
  W_TEST_STATUS(pAccessor->SetValueByName(pShaderProperties, "CustomColor", WColor::DarkGoldenRod));
  pAccessor->FinishTransaction();

  ProcessEvents();
  CaptureMaterialImage();

  CloseMaterial();
}
