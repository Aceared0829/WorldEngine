#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/GameObjectContextPreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectContextPreferencesUser, 1, WRTTIDefaultAllocator<WGameObjectContextPreferencesUser>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ContextDocument", m_ContextDocument)->AddAttributes(new WHiddenAttribute),
    W_MEMBER_PROPERTY("ContextObject", m_ContextObject)->AddAttributes(new WHiddenAttribute),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WGameObjectContextPreferencesUser::WGameObjectContextPreferencesUser()
  : WPreferences(Domain::Document, "GameObjectContext")
{
}

WUuid WGameObjectContextPreferencesUser::GetContextDocument() const
{
  return m_ContextDocument;
}


void WGameObjectContextPreferencesUser::SetContextDocument(WUuid val)
{
  m_ContextDocument = val;
  TriggerPreferencesChangedEvent();
}

WUuid WGameObjectContextPreferencesUser::GetContextObject() const
{
  return m_ContextObject;
}

void WGameObjectContextPreferencesUser::SetContextObject(WUuid val)
{
  m_ContextObject = val;
  TriggerPreferencesChangedEvent();
}
