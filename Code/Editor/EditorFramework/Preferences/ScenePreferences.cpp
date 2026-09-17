#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScenePreferencesUser, 1, WRTTIDefaultAllocator<WScenePreferencesUser>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ShowGrid", m_bShowGrid),
    W_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new WDefaultValueAttribute(10), new WClampValueAttribute(1, 30)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScenePreferencesUser::WScenePreferencesUser()
  : WPreferences(Domain::Document, "Scene")
{
  m_iCameraSpeed = 9;
}

void WScenePreferencesUser::SetCameraSpeed(WInt32 value)
{
  m_iCameraSpeed = WMath::Clamp(value, 0, 24);

  // Kiff, inform the men!
  TriggerPreferencesChangedEvent();
}

void WScenePreferencesUser::SetShowGrid(bool bShow)
{
  m_bShowGrid = bShow;

  TriggerPreferencesChangedEvent();
}
