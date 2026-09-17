#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_IMPLEMENT_SINGLETON(WPropertyMetaState);

W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyMetaState)

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WPropertyMetaState);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (WPropertyMetaState::GetSingleton())
    {
      auto ptr = WPropertyMetaState::GetSingleton();
      W_DEFAULT_DELETE(ptr);
    }
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WPropertyMetaState::WPropertyMetaState()
  : m_SingletonRegistrar(this)
{
}

void WPropertyMetaState::GetTypePropertiesState(const WDocumentObject* pObject, WMap<WString, WPropertyUiState>& out_propertyStates)
{
  WPropertyMetaStateEvent eventData;
  eventData.m_pPropertyStates = &out_propertyStates;
  eventData.m_pObject = pObject;

  m_Events.Broadcast(eventData);
}

void WPropertyMetaState::GetTypePropertiesState(const WArrayPtr<WPropertySelection>& items, WMap<WString, WPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetTypePropertiesState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility = WMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

void WPropertyMetaState::GetContainerElementsState(const WDocumentObject* pObject, const char* szProperty, WHashTable<WVariant, WPropertyUiState>& out_propertyStates)
{
  WContainerElementMetaStateEvent eventData;
  eventData.m_pContainerElementStates = &out_propertyStates;
  eventData.m_pObject = pObject;
  eventData.m_szProperty = szProperty;

  m_ContainerEvents.Broadcast(eventData);
}

void WPropertyMetaState::GetContainerElementsState(const WArrayPtr<WPropertySelection>& items, const char* szProperty, WHashTable<WVariant, WPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp2.Clear();
    GetContainerElementsState(sel.m_pObject, szProperty, m_Temp2);

    for (auto it = m_Temp2.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility = WMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}
