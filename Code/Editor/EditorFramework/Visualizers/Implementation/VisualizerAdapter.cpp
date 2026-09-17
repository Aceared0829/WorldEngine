#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

WVisualizerAdapter::WVisualizerAdapter()
{
  m_pVisualizerAttr = nullptr;
  m_pObject = nullptr;
  m_bVisualizerIsVisible = true;

  WQtDocumentWindow::s_Events.AddEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentWindowEventHandler, this));
}

WVisualizerAdapter::~WVisualizerAdapter()
{
  WQtDocumentWindow::s_Events.RemoveEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void WVisualizerAdapter::SetVisualizer(const WVisualizerAttribute* pAttribute, const WDocumentObject* pObject)
{
  m_pVisualizerAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(WMakeDelegate(&WVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bVisualizerIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}



void WVisualizerAdapter::DocumentObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == WDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pVisualizerAttr->m_sProperty1 || e.m_sProperty == m_pVisualizerAttr->m_sProperty2 || e.m_sProperty == m_pVisualizerAttr->m_sProperty3 || e.m_sProperty == m_pVisualizerAttr->m_sProperty4 || e.m_sProperty == m_pVisualizerAttr->m_sProperty5 || e.m_sProperty == m_pVisualizerAttr->m_sProperty6)
      {
        Update();
      }
    }
  }
}

void WVisualizerAdapter::DocumentWindowEventHandler(const WQtDocumentWindowEvent& e)
{
  if (e.m_Type == WQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void WVisualizerAdapter::DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & WDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bVisualizerIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

WTransform WVisualizerAdapter::GetObjectTransform() const
{
  WTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t).IgnoreResult();

  return t;
}

WObjectAccessorBase* WVisualizerAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const WAbstractProperty* WVisualizerAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}
