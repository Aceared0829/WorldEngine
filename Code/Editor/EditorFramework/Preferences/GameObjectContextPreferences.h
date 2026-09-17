#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class WGameObjectContextPreferencesUser : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectContextPreferencesUser, WPreferences);

public:
  WGameObjectContextPreferencesUser();

  WUuid GetContextDocument() const;
  void SetContextDocument(WUuid val);
  WUuid GetContextObject() const;
  void SetContextObject(WUuid val);

protected:
  WUuid m_ContextDocument;
  WUuid m_ContextObject;
};
