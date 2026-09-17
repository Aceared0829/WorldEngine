#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WDocument;

/// Base class for all preferences.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// Pass the 'Domain' and 'Visibility' to the constructor to configure whether the preference class
/// is per application, per project or per document, and whether the data is shared among all users
/// or custom for every user.
class W_EDITORFRAMEWORK_DLL WPreferences : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPreferences, WReflectedClass);

public:
  enum class Domain
  {
    Application,
    Project,
    Document
  };

  /// Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  template <typename TYPE>
  static TYPE* QueryPreferences(const WDocument* pDocument = nullptr)
  {
    static_assert((std::is_base_of<WPreferences, TYPE>::value == true), "All preferences objects must be derived from WPreferences");
    return static_cast<TYPE*>(QueryPreferences(WGetStaticRTTI<TYPE>(), pDocument));
  }

  /// Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  static WPreferences* QueryPreferences(const WRTTI* pRtti, const WDocument* pDocument = nullptr);

  /// Saves all preferences that are tied to the given document
  static void SaveDocumentPreferences(const WDocument* pDocument);

  /// Removes all preferences for the given document. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearDocumentPreferences(const WDocument* pDocument);

  /// Saves all project specific preferences.
  static void SaveProjectPreferences();

  /// Removes all project specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearProjectPreferences();

  /// Saves all application specific preferences.
  static void SaveApplicationPreferences();

  /// Removes all application specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearApplicationPreferences();

  //// Fills the list with all currently known preferences
  static void GatherAllPreferences(WDynamicArray<WPreferences*>& out_allPreferences);

  /// Whether the preferences are app, project or document specific
  Domain GetDomain() const { return m_Domain; }

  /// Within the same domain and visibility the name must be unique, but across those it can be reused.
  WString GetName() const;

  /// If these preferences are per document, the pointer is valid, otherwise nullptr.
  const WDocument* GetDocumentAssociation() const { return m_pDocument; }

  /// A simple event that can be fired when any preference property changes. No specific change details are given.
  WEvent<WPreferences*> m_ChangedEvent;

  /// Call this to broadcast that this preference object was modified.
  void TriggerPreferencesChangedEvent() { m_ChangedEvent.Broadcast(this); }

protected:
  WPreferences(Domain domain, const char* szUniqueName);

  WString GetFilePath() const;

private:
  static void SavePreferences(const WDocument* pDocument, Domain domain);
  static void ClearPreferences(const WDocument* pDocument, Domain domain);

  void Load();
  void Save() const;



private:
  Domain m_Domain;
  WString m_sUniqueName;
  const WDocument* m_pDocument;

  static WMap<const WDocument*, WMap<const WRTTI*, WPreferences*>> s_Preferences;
};
