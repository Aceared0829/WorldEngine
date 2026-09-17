#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPreferences, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMap<const WDocument*, WMap<const WRTTI*, WPreferences*>> WPreferences::s_Preferences;

WPreferences::WPreferences(Domain domain, const char* szUniqueName)
{
  m_Domain = domain;
  m_sUniqueName = szUniqueName;
  m_pDocument = nullptr;
}

WPreferences* WPreferences::QueryPreferences(const WRTTI* pRtti, const WDocument* pDocument)
{
  W_ASSERT_DEV(WQtEditorApp::GetSingleton() != nullptr, "Editor app is not available in this process");

  auto it = s_Preferences[pDocument].Find(pRtti);

  if (it.IsValid())
    return it.Value();

  auto pAlloc = pRtti->GetAllocator();
  W_ASSERT_DEV(pAlloc != nullptr, "Invalid allocator for preferences type");

  if (!pAlloc->CanAllocate())
  {
    W_ASSERT_DEV(pAlloc->CanAllocate(), "Cannot create a preferences object that does not have a proper allocator");
    return nullptr;
  }

  WPreferences* pPref = pAlloc->Allocate<WPreferences>();
  pPref->m_pDocument = pDocument;
  s_Preferences[pDocument][pRtti] = pPref;

  if (pPref->m_Domain == Domain::Document)
  {
    W_ASSERT_DEV(pDocument != nullptr, "Preferences of this type can only be used per document");
  }
  else
  {
    W_ASSERT_DEV(pDocument == nullptr, "Preferences of this type cannot be used with a document");
  }

  pPref->Load();
  return pPref;
}

WString WPreferences::GetFilePath() const
{
  WStringBuilder path;

  if (m_Domain == Domain::Application)
  {
    path = WApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Project)
  {
    path = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Document)
  {
    path = WApplicationServices::GetSingleton()->GetDocumentPreferencesFolder(m_pDocument);
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  return path;
}

void WPreferences::Load()
{
  WFileReader file;
  if (file.Open(GetFilePath()).Failed())
    return;

  WReflectionSerializer::ReadObjectPropertiesFromDDL(file, *GetDynamicRTTI(), this);
}

void WPreferences::Save() const
{
  bool bNothingToSerialize = true;

  WTempHybridArray<const WAbstractProperty*, 32> allProperties;
  GetDynamicRTTI()->GetAllProperties(allProperties);

  for (const WAbstractProperty* pProp : allProperties)
  {
    if (pProp->GetCategory() == WPropertyCategory::Constant || pProp->GetFlags().IsAnySet(WPropertyFlags::ReadOnly))
      continue;

    bNothingToSerialize = false;
    break;
  }

  if (bNothingToSerialize)
    return;

  WDeferredFileWriter file;
  file.SetOutput(GetFilePath());

  WReflectionSerializer::WriteObjectToDDL(file, GetDynamicRTTI(), this, false, WOpenDdlWriter::TypeStringMode::Compliant);

  if (file.Close().Failed())
    WLog::Error("Failed to open file for writing '{0}'.", GetFilePath());
}


void WPreferences::SavePreferences(const WDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid(); ++it)
  {
    auto pPref = it.Value();

    if (pPref->m_Domain == domain)
      pPref->Save();
  }
}

void WPreferences::ClearPreferences(const WDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid();)
  {
    WPreferences* pPref = it.Value();

    if (pPref->m_Domain == domain)
    {
      pPref->GetDynamicRTTI()->GetAllocator()->Deallocate(pPref);
      it = docPrefs.Remove(it);
    }
    else
      ++it;
  }
}

void WPreferences::SaveDocumentPreferences(const WDocument* pDocument)
{
  SavePreferences(pDocument, Domain::Document);
}

void WPreferences::ClearDocumentPreferences(const WDocument* pDocument)
{
  ClearPreferences(pDocument, Domain::Document);
}

void WPreferences::SaveProjectPreferences()
{
  SavePreferences(nullptr, Domain::Project);
}

void WPreferences::ClearProjectPreferences()
{
  ClearPreferences(nullptr, Domain::Project);
}

void WPreferences::SaveApplicationPreferences()
{
  SavePreferences(nullptr, Domain::Application);
}

void WPreferences::ClearApplicationPreferences()
{
  ClearPreferences(nullptr, Domain::Application);
}

void WPreferences::GatherAllPreferences(WDynamicArray<WPreferences*>& out_allPreferences)
{
  out_allPreferences.Clear();
  out_allPreferences.Reserve(s_Preferences.GetCount() * 2);

  for (auto itDoc = s_Preferences.GetIterator(); itDoc.IsValid(); ++itDoc)
  {
    for (auto itType = itDoc.Value().GetIterator(); itType.IsValid(); ++itType)
    {
      out_allPreferences.PushBack(itType.Value());
    }
  }
}

WString WPreferences::GetName() const
{
  WStringBuilder s;

  if (m_Domain == Domain::Document)
  {
    s.Set(m_sUniqueName, ": ");
    s.Append(WPathUtils::GetFileName(m_pDocument->GetDocumentPath()));
  }
  else
  {
    if (m_Domain == Domain::Application)
      s.Append("Application");
    else if (m_Domain == Domain::Project)
      s.Append("Project");

    s.Append(": ", m_sUniqueName);
  }

  return s;
}
