#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QLayout>
#include <QStackedWidget>

namespace
{
  std::unique_ptr<WQtDocumentTreeModel> CreateGameObjectTreeModel(WSceneDocument* pDocument)
  {
    std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtScenegraphModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children"));
    pModel->AddAdapter(new WQtGameObjectAdapter(pDocument->GetObjectManager()));
    return std::move(pModel);
  }

  std::unique_ptr<WQtDocumentTreeModel> CreateSceneTreeModel(WScene2Document* pDocument)
  {
    std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtScenegraphModel(pDocument->GetSceneObjectManager()));
    pModel->AddAdapter(new WQtDummyAdapter(pDocument->GetSceneObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children"));
    pModel->AddAdapter(new WQtGameObjectAdapter(pDocument->GetSceneObjectManager(), pDocument->GetSceneDocumentObjectMetaData(), pDocument->GetSceneGameObjectMetaData()));
    return std::move(pModel);
  }
} // namespace

WQtScenegraphPanel::WQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, WSceneDocument* pDocument)
  : WQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("WQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateGameObjectTreeModel(pDocument);
  m_pMainGameObjectWidget = new WQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_pStack->addWidget(m_pMainGameObjectWidget);
}

WQtScenegraphPanel::WQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, WScene2Document* pDocument)
  : WQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("WQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateSceneTreeModel(pDocument);
  m_pMainGameObjectWidget = new WQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel), pDocument->GetSceneSelectionManager());
  m_LayerWidgets[pDocument->GetGuid()] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);

  pDocument->m_LayerEvents.AddEventHandler(WMakeDelegate(&WQtScenegraphPanel::LayerEventHandler, this), m_LayerEventUnsubscriber);
  WTempHybridArray<WSceneDocument*, 16> layers;
  pDocument->GetLoadedLayers(layers);
  for (WSceneDocument* pLayer : layers)
  {
    if (pLayer != pDocument)
      LayerLoaded(pLayer->GetGuid());
  }
  ActiveLayerChanged(pDocument->GetActiveLayer());
}

WQtScenegraphPanel::~WQtScenegraphPanel() = default;

void WQtScenegraphPanel::LayerEventHandler(const WScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case WScene2LayerEvent::Type::LayerLoaded:
      LayerLoaded(e.m_layerGuid);
      break;
    case WScene2LayerEvent::Type::LayerUnloaded:
      LayerUnloaded(e.m_layerGuid);
      break;
    case WScene2LayerEvent::Type::ActiveLayerChanged:
    {
      ActiveLayerChanged(e.m_layerGuid);
    }
    default:
      break;
  }
}

void WQtScenegraphPanel::LayerLoaded(const WUuid& layerGuid)
{
  W_ASSERT_DEV(!m_LayerWidgets.Contains(layerGuid), "LayerLoaded was fired twice for the same layer.");

  auto pScene2 = static_cast<WScene2Document*>(m_pSceneDocument);
  auto pLayer = pScene2->GetLayerDocument(layerGuid);
  auto pCustomModel = CreateGameObjectTreeModel(pLayer);
  m_pMainGameObjectWidget = new WQtGameObjectWidget(this, pLayer, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_LayerWidgets[layerGuid] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void WQtScenegraphPanel::LayerUnloaded(const WUuid& layerGuid)
{
  W_ASSERT_DEV(m_LayerWidgets.Contains(layerGuid), "LayerUnloaded was fired without the layer being loaded first.");

  WQtGameObjectWidget* pWidget = m_LayerWidgets[layerGuid];
  m_pStack->removeWidget(pWidget);
  m_LayerWidgets.Remove(layerGuid);
  delete pWidget;

  auto pScene2 = static_cast<WScene2Document*>(m_pSceneDocument);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void WQtScenegraphPanel::ActiveLayerChanged(const WUuid& layerGuid)
{
  // migrate the search filter text to the other layer
  if (WQtGameObjectWidget* pPrev = qobject_cast<WQtGameObjectWidget*>(m_pStack->currentWidget()))
  {
    QString sText = pPrev->GetFilterWidget().text();
    m_LayerWidgets[layerGuid]->GetFilterWidget().setText(sText);
  }

  m_pStack->setCurrentWidget(m_LayerWidgets[layerGuid]);
}
