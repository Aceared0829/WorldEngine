#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>


WQtGameObjectWidget::WQtGameObjectWidget(QWidget* pParent, WGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<WQtDocumentTreeModel> pCustomModel, WSelectionManager* pSelection)
{
  setObjectName("WQtGameObjectWidget");

  m_pDocument = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;
  m_pDelegate = new WQtGameObjectDelegate(this, pDocument);

  setLayout(new QVBoxLayout());
  setContentsMargins(0, 0, 0, 0);
  layout()->setObjectName("QVBoxLayout1");
  layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new WQtSearchWidget(this);
  m_pFilterWidget->setObjectName("WQtSearchWidget");
  m_pFilterWidget->setPlaceholderText("Search by name or component type");
  m_pFilterWidget->setToolTip("Search by object name or component type name.\nUse 'ref:{GUID}' to show only objects that reference a specific asset.");
  connect(m_pFilterWidget, &WQtSearchWidget::textChanged, this, &WQtGameObjectWidget::OnFilterTextChanged);

  layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new WQtDocumentTreeView(this, pDocument, std::move(pCustomModel), pSelection);
  m_pTreeWidget->setObjectName("WQtDocumentTreeView");
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  layout()->addWidget(m_pTreeWidget);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WQtGameObjectWidget::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  W_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  W_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");
}

WQtGameObjectWidget::~WQtGameObjectWidget()
{
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WQtGameObjectWidget::DocumentSceneEventHandler, this));
}


void WQtGameObjectWidget::DocumentSceneEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::TriggerShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;
    case WGameObjectEvent::Type::TriggerExpandScenegraph:
    {
      m_pTreeWidget->expandAll();
    }
    break;
    case WGameObjectEvent::Type::TriggerSetScenegraphFilter:
    {
      m_pFilterWidget->setText(QString::fromUtf8(e.m_sPayload.GetData()));
    }
    break;

    default:
      break;
  }
}

void WQtGameObjectWidget::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void WQtGameObjectWidget::OnRequestContextMenu(QPoint pos)
{
  WQtMenuActionMapView menu(nullptr);

  WActionContext context;
  context.m_sMapping = m_sContextMenuMapping;
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void WQtGameObjectWidget::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}


//////////////////////////////////////////////////////////////////////////

WQtGameObjectPanel::WQtGameObjectPanel(ads::CDockManager* pDockManager, QWidget* pParent, WGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<WQtDocumentTreeModel> pCustomModel)
  : WQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("WQtGameObjectPanel");

  m_pMainWidget = new WQtGameObjectWidget(this, pDocument, szContextMenuMapping, std::move(pCustomModel));
  setWidget(m_pMainWidget);
}

WQtGameObjectPanel::~WQtGameObjectPanel() = default;
