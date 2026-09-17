#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProjectPreferencesUser, 1, WRTTIDefaultAllocator<WProjectPreferencesUser>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Players", m_PlayerApps)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ExportFolder", m_sExportFolder)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("SharedMaterialFolder", m_sSharedMaterialFolder)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("MeshLodPrefix", m_sMeshLodPrefix)->AddAttributes(new WHiddenAttribute(), new WDefaultValueAttribute("$LOD")),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WProjectPreferencesUser::WProjectPreferencesUser()
  : WPreferences(Domain::Project, "General")
{
}


void WQtEditorApp::LoadProjectPreferences()
{
  W_PROFILE_SCOPE("LoadProjectPreferences");
  WPreferences::QueryPreferences<WProjectPreferencesUser>();
}
