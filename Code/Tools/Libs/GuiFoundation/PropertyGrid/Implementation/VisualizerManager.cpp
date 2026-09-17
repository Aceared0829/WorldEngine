#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

W_IMPLEMENT_SINGLETON(WVisualizerManager);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualizerManager)

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WVisualizerManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (WVisualizerManager::GetSingleton())
    {
      auto ptr = WVisualizerManager::GetSingleton();
      W_DEFAULT_DELETE(ptr);
    }
  }


W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WVisualizerManager::WVisualizerManager()
  : m_SingletonRegistrar(this)
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WVisualizerManager::DocumentManagerEventHandler, this));
}

WVisualizerManager::~WVisualizerManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WVisualizerManager::DocumentManagerEventHandler, this));
}

void WVisualizerManager::SetVisualizersActive(const WDocument* pDoc, bool bActive)
{
  if (m_DocsSubscribed[pDoc].m_bActivated == bActive)
    return;

  m_DocsSubscribed[pDoc].m_bActivated = bActive;

  SendEventToRecreateVisualizers(pDoc);
}

bool WVisualizerManager::GetVisualizersActive(const WDocument* pDoc)
{
  return m_DocsSubscribed[pDoc].m_bActivated;
}

void WVisualizerManager::SelectionEventHandler(const WSelectionManagerEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  SendEventToRecreateVisualizers(event.m_pDocument);
}

void WVisualizerManager::SendEventToRecreateVisualizers(const WDocument* pDoc)
{
  if (m_DocsSubscribed[pDoc].m_bActivated)
  {
    const auto& sel = pDoc->GetSelectionManager()->GetSelection();

    WVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;
    m_Events.Broadcast(e);
  }
  else
  {
    WDeque<const WDocumentObject*> sel;

    WVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;

    m_Events.Broadcast(e);
  }
}

void WVisualizerManager::DocumentManagerEventHandler(const WDocumentManager::Event& e)
{
  if (e.m_Type == WDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WVisualizerManager::StructureEventHandler, this));
  }

  if (e.m_Type == WDocumentManager::Event::Type::DocumentClosing)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WVisualizerManager::StructureEventHandler, this));

    SetVisualizersActive(e.m_pDocument, false);
  }
}

void WVisualizerManager::StructureEventHandler(const WDocumentObjectStructureEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  if (!event.m_pDocument->GetSelectionManager()->IsSelectionEmpty() &&
      (event.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectAdded ||
        event.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectRemoved))
  {
    SendEventToRecreateVisualizers(event.m_pDocument);
  }
}
