#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <QVBoxLayout>

WQtLayerPanel::WQtLayerPanel(ads::CDockManager* pDockManager, QWidget* pParent, WScene2Document* pDocument)
  : WQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;
  m_pDelegate = new WQtLayerDelegate(this, pDocument);

  std::unique_ptr<WQtLayerModel> pModel(new WQtLayerModel(m_pSceneDocument));
  pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetSceneObjectManager(), WGetStaticRTTI<WSceneDocumentSettings>(), "Layers"));
  pModel->AddAdapter(new WQtLayerAdapter(pDocument));

  m_pTreeWidget = new WQtDocumentTreeView(this, pDocument, std::move(pModel), m_pSceneDocument->GetLayerSelectionManager());
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  W_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
    "signal/slot connection failed");

  setWidget(m_pTreeWidget);

  {
    // Tool Bar
    WQtToolBarActionMapView* pToolBar = new WQtToolBarActionMapView("Toolbar", this);
    WActionContext context;
    context.m_sMapping = "EditorPluginScene_LayerToolbar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("LayerPanel_ToolBar");
    setToolBar(pToolBar);
  }
}

WQtLayerPanel::~WQtLayerPanel() = default;

void WQtLayerPanel::OnRequestContextMenu(QPoint pos)
{
  WQtMenuActionMapView menu(nullptr);

  WActionContext context;
  context.m_sMapping = "EditorPluginScene_LayerContextMenu";
  context.m_pDocument = m_pSceneDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}
