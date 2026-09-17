#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

WQtSubstancePackageAssetWindow::WQtSubstancePackageAssetWindow(WSubstancePackageAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  if (pDocument->m_SelectedOutput.IsValid() == false)
  {
    auto pMetaData = pDocument->GetAssetDocumentInfo()->GetMetaInfo<WSubstancePackageAssetMetaData>();
    if (pMetaData->m_OutputUuids.GetCount() > 0)
      pDocument->m_SelectedOutput = pMetaData->m_OutputUuids[0];
  }

  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "SubstanceAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "SubstanceAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SubstanceAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(WVec3(-2, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(WVec3(0), WVec3(0.0f), WVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    WQtViewWidgetContainer* pContainer = new WQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, nullptr);

    m_pDockManager->setCentralWidget(pContainer);
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SubstanceAssetDockWidget");
    pPropertyPanel->setWindowTitle("Substance Package Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator((WAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void WQtSubstancePackageAssetWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtSubstancePackageAssetWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const WSubstancePackageAssetDocument* pDoc = static_cast<const WSubstancePackageAssetDocument*>(GetDocument());

    {
      WDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetChannelMode";
      msg.m_iValue = pDoc->m_ChannelMode.GetValue();
      msg.m_fValue = 0.5f;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      WDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetLodLevel";
      msg.m_iValue = pDoc->m_iTextureLod;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      WStringBuilder tmp;

      WDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetTexture";
      msg.m_sValue = WConversionUtils::ToString(pDoc->m_SelectedOutput, tmp);
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSubstanceSelectOutputAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSubstanceSelectOutputAction::WSubstanceSelectOutputAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
}

void WSubstanceSelectOutputAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  WSubstancePackageAssetDocument* pDocument = static_cast<WSubstancePackageAssetDocument*>(m_Context.m_pDocument);
  auto pMetaData = pDocument->GetAssetDocumentInfo()->GetMetaInfo<WSubstancePackageAssetMetaData>();

  out_entries.Clear();
  out_entries.Reserve(pMetaData->m_OutputNames.GetCount());

  for (WUInt32 i = 0; i < pMetaData->m_OutputNames.GetCount(); ++i)
  {
    auto& item = out_entries.ExpandAndGetRef();
    item.m_sDisplay = pMetaData->m_OutputNames[i];

    const WUuid& uuid = pMetaData->m_OutputUuids[i];
    item.m_UserValue = uuid;
    item.m_CheckState = (uuid == pDocument->m_SelectedOutput) ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  }
}

void WSubstanceSelectOutputAction::Execute(const WVariant& value)
{
  if (value.IsA<WUuid>())
  {
    WSubstancePackageAssetDocument* pDocument = static_cast<WSubstancePackageAssetDocument*>(m_Context.m_pDocument);
    pDocument->m_SelectedOutput = value.Get<WUuid>();
  }
}

//////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WSubstancePackageAssetActions::s_hSelectedOutput;
WActionDescriptorHandle WSubstancePackageAssetActions::s_hTextureChannelMode;
WActionDescriptorHandle WSubstancePackageAssetActions::s_hLodSlider;

void WSubstancePackageAssetActions::RegisterActions()
{
  s_hSelectedOutput = W_REGISTER_DYNAMIC_MENU("SubstancePackageAsset.SelectedOutput", WSubstanceSelectOutputAction, ":/AssetIcons/SubstanceDesigner.svg");
  s_hTextureChannelMode = W_REGISTER_DYNAMIC_MENU("SubstancePackageAsset.ChannelMode", WTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = W_REGISTER_ACTION_0("SubstancePackageAsset.LodSlider", WActionScope::Document, "Texture 2D", "", WTextureLodSliderAction);
}

void WSubstancePackageAssetActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hSelectedOutput);
  WActionManager::UnregisterAction(s_hTextureChannelMode);
  WActionManager::UnregisterAction(s_hLodSlider);
}

void WSubstancePackageAssetActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectedOutput, "", 14.0f);
  pMap->MapAction(s_hLodSlider, "", 15.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 16.0f);
}
