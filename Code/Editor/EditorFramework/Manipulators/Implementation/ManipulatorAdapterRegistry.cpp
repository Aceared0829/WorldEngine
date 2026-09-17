#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>

W_IMPLEMENT_SINGLETON(WManipulatorAdapterRegistry);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ManipulatorAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ManipulatorManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WManipulatorAdapterRegistry);
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = WManipulatorAdapterRegistry::GetSingleton();
    W_DEFAULT_DELETE(ptr);
  }
 
W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WManipulatorAdapterRegistry::WManipulatorAdapterRegistry()
  : m_SingletonRegistrar(this)
{
  WManipulatorManager::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));
}

WManipulatorAdapterRegistry::~WManipulatorAdapterRegistry()
{
  WManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(
    WMakeDelegate(&WManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}

void WManipulatorAdapterRegistry::QueryGridSettings(const WDocument* pDocument, WGridSettingsMsgToEngine& out_gridSettings)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    adapt->QueryGridSettings(out_gridSettings);
  }
}

void WManipulatorAdapterRegistry::ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  if (e.m_pManipulator == nullptr || e.m_bHideManipulators)
    return;

  for (const auto& sel : *e.m_pSelection)
  {
    WManipulatorAdapter* pAdapter = m_Factory.CreateObject(e.m_pManipulator->GetDynamicRTTI());

    if (pAdapter)
    {
      m_DocumentAdapters[e.m_pDocument].m_Adapters.PushBack(pAdapter);
      pAdapter->SetManipulator(e.m_pManipulator, sel.m_pObject);
    }
  }
}

void WManipulatorAdapterRegistry::ClearAdapters(const WDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    W_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
