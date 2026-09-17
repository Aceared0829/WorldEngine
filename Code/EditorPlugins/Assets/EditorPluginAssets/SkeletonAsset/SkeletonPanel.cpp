#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

WQtSkeletonPanel::WQtSkeletonPanel(ads::CDockManager* pDockManager, QWidget* pParent, WSkeletonAssetDocument* pDocument)
  : WQtDocumentPanel(pDockManager, pParent, pDocument)
{
  m_pSkeletonDocument = pDocument;

  setObjectName("SkeletonPanel");
  setWindowTitle("Skeleton");

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout());
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
  m_pFilterWidget = new WQtSearchWidget(this);
  connect(m_pFilterWidget, &WQtSearchWidget::textChanged, this,
    [this](const QString& sText)
    { m_pTreeWidget->GetProxyFilterModel()->SetFilterText(sText); });

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtDocumentTreeModel(pDocument->GetObjectManager()));
  pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children"));
  pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WEditableSkeleton>(), "Children"));
  pModel->AddAdapter(new WQtJointAdapter(pDocument));

  m_pTreeWidget = new WQtDocumentTreeView(this, pDocument, std::move(pModel));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->expandAll();
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);
}

WQtSkeletonPanel::~WQtSkeletonPanel() = default;
