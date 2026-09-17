#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>

W_IMPLEMENT_SINGLETON(WVisualizerAdapterRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualizerAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "VisualizerManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WVisualizerAdapterRegistry);
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = WVisualizerAdapterRegistry::GetSingleton();
    W_DEFAULT_DELETE(ptr);
  }
 
W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WVisualizerAdapterRegistry::WVisualizerAdapterRegistry()
  : m_SingletonRegistrar(this)
{
  WVisualizerManager::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));
}

WVisualizerAdapterRegistry::~WVisualizerAdapterRegistry()
{
  WVisualizerManager::GetSingleton()->m_Events.RemoveEventHandler(WMakeDelegate(&WVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}


void WVisualizerAdapterRegistry::CreateAdapters(const WDocument* pDocument, const WDocumentObject* pObject)
{
  const auto& attributes = pObject->GetTypeAccessor().GetType()->GetAttributes();

  for (const auto pAttr : attributes)
  {
    if (pAttr->IsInstanceOf<WVisualizerAttribute>())
    {
      WVisualizerAdapter* pAdapter = m_Factory.CreateObject(pAttr->GetDynamicRTTI());

      if (pAdapter)
      {
        m_DocumentAdapters[pDocument].m_Adapters.PushBack(pAdapter);
        pAdapter->SetVisualizer(static_cast<const WVisualizerAttribute*>(pAttr), pObject);
      }
    }
  }

  for (const auto pChild : pObject->GetChildren())
  {
    CreateAdapters(pDocument, pChild);
  }
}

void WVisualizerAdapterRegistry::VisualizerManagerEventHandler(const WVisualizerManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  for (const auto sel : *e.m_pSelection)
  {
    CreateAdapters(e.m_pDocument, sel);
  }
}

void WVisualizerAdapterRegistry::ClearAdapters(const WDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    W_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
