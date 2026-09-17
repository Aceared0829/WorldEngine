#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// WTextureChannelModeAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureChannelModeAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WTextureChannelModeAction::WTextureChannelModeAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WEnumerationMenuAction(context, szName, szIconPath)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = WReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "ChannelMode");

  const WRTTI* pEnumRTTI = m_pValueProperty != nullptr ? m_pValueProperty->GetSpecificType() : WGetStaticRTTI<WTextureChannelMode>();
  InitEnumerationType(pEnumRTTI);
}

WInt64 WTextureChannelModeAction::GetValue() const
{
  WVariant value = 0;
  if (m_pValueProperty)
  {
    value = WReflectionUtils::GetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument);
  }
  return value.ConvertTo<WInt64>();
}

void WTextureChannelModeAction::Execute(const WVariant& value)
{
  if (m_pValueProperty)
  {
    WReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}

//////////////////////////////////////////////////////////////////////////
// WTextureLodSliderAction
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTextureLodSliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;


WTextureLodSliderAction::WTextureLodSliderAction(const WActionContext& context, const char* szName)
  : WSliderAction(context, szName)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = WReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "TextureLod");

  WVariant currentValue = -1;
  if (m_pValueProperty)
  {
    currentValue = WReflectionUtils::GetMemberPropertyValue(m_pValueProperty, pDocument);
  }

  SetRange(-1, 13);
  SetValue(currentValue.ConvertTo<int>());
}

void WTextureLodSliderAction::Execute(const WVariant& value)
{
  if (m_pValueProperty)
  {
    WReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}


//////////////////////////////////////////////////////////////////////////
// WTextureAssetActions
//////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WTextureAssetActions::s_hTextureChannelMode;
WActionDescriptorHandle WTextureAssetActions::s_hLodSlider;

void WTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = W_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", WTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = W_REGISTER_ACTION_0("TextureAsset.LodSlider", WActionScope::Document, "Texture 2D", "", WTextureLodSliderAction);
}

void WTextureAssetActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hTextureChannelMode);
  WActionManager::UnregisterAction(s_hLodSlider);
}

void WTextureAssetActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLodSlider, "", 14.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// WQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

WQtTextureAssetDocumentWindow::WQtTextureAssetDocumentWindow(WTextureAssetDocument* pDocument)
  : WQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "TextureAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
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
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new WQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void WQtTextureAssetDocumentWindow::InternalRedraw()
{
  WEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  WQtEngineDocumentWindow::InternalRedraw();
}

void WQtTextureAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const WTextureAssetDocument* pDoc = static_cast<const WTextureAssetDocument*>(GetDocument());
    const WTextureAssetProperties* pProps = pDoc->GetProperties();

    {
      WDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetChannelMode";
      msg.m_iValue = pDoc->m_ChannelMode.GetValue();
      msg.m_fValue = pProps->m_fAlphaThreshold;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      WDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetLodLevel";
      msg.m_iValue = pDoc->m_iTextureLod;
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
