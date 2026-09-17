#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// WLUTAssetActions
//////////////////////////////////////////////////////////////////////////


void WLUTAssetActions::RegisterActions() {}

void WLUTAssetActions::UnregisterActions() {}

void WLUTAssetActions::MapActions(WStringView sMapping) {}


//////////////////////////////////////////////////////////////////////////
// WQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

WQtLUTAssetDocumentWindow::WQtLUTAssetDocumentWindow(WLUTAssetDocument* pDocument)
  : WQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
    WActionContext context;
    context.m_sMapping = "LUTAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "LUTAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("LUTAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    /*
        TODO: Add live 3D preview of the LUT with a slider for the strength etc.

        SetTargetFramerate(10);

        m_ViewConfig.m_Camera.LookAt(WVec3(-2, 0, 0), WVec3(0, 0, 0), WVec3(0, 0, 1));
        m_ViewConfig.ApplyPerspectiveSetting(90);

        m_pViewWidget = new WQtOrbitCamViewWidget(this, &m_ViewConfig);
        m_pViewWidget->ConfigureOrbitCameraVolume(WVec3(0), WVec3(1.0f), WVec3(-1, 0, 0));
        AddViewWidget(m_pViewWidget);
        WQtViewWidgetContainer* pContainer = new WQtViewWidgetContainer(this, m_pViewWidget, nullptr);

        m_pDockManager->setCentralWidget(pContainer);*/
  }

  {
    WQtDocumentPanel* pPropertyPanel = new WQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("LUTAssetDockWidget");
    pPropertyPanel->setWindowTitle("LUT Properties");
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
