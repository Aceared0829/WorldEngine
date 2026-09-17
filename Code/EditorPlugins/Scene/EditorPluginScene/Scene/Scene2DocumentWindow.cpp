#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <QLayout>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtScene2DocumentWindow::WQtScene2DocumentWindow(WScene2Document* pDocument)
  : WQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](WQtEngineDocumentWindow* pWindow, WEngineViewConfig* pConfig) -> WQtEngineViewWidget*
  {
    WQtSceneViewWidget* pWidget = new WQtSceneViewWidget(nullptr, static_cast<WQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new WQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate([this](WGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<WGameObjectDocument*>(GetDocument()), this, this); });

  {
    WQtDocumentPanel* pViewPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pViewPanel->setObjectName("WQtDocumentPanel");
    pViewPanel->setWindowTitle("3D View");
    pViewPanel->setWidget(m_pQuadViewWidget);

    m_pDockManager->setCentralWidget(pViewPanel);
  }

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  SetTargetFramerate(pPreferences->GetMaxFramerate());

  {
    // Menu Bar
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2MenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2ToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  {
    // Panels
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();
    pPropertyPanel->layout()->setObjectName("PropertyPanelLayout");

    WQtDocumentPanel* pPanelTree = new WQtScenegraphPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPanelTree->show();

    WQtLayerPanel* pLayers = new WQtLayerPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pLayers->show();

    WQtPropertyGridWidget* pPropertyGrid = new WQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    W_VERIFY(connect(pPropertyGrid, &WQtPropertyGridWidget::ExtendContextMenu, this, &WQtScene2DocumentWindow::ExtendPropertyGridContextMenu), "");

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pLayers);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }
  FinishWindowCreation();
}

WQtScene2DocumentWindow::~WQtScene2DocumentWindow() = default;

bool WQtScene2DocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  WScene2Document* pDoc = static_cast<WScene2Document*>(GetDocument());
  if (pDoc && pDoc->IsAnyLayerModified())
  {
    QMessageBox::StandardButton res = WQtUiServices::MessageBoxQuestion("Save scene and all layers before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Yes);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      WStatus err = SaveAllLayers();

      if (err.Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

WStatus WQtScene2DocumentWindow::SaveAllLayers()
{
  WScene2Document* pDoc = static_cast<WScene2Document*>(GetDocument());

  WTempHybridArray<WSceneDocument*, 16> layers;
  pDoc->GetLoadedLayers(layers);

  for (auto pLayer : layers)
  {
    WStatus res = pLayer->SaveDocument();

    if (res.Failed())
    {
      return res;
    }
  }

  return WStatus(W_SUCCESS);
}
