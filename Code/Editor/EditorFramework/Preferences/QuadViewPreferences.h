#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/Preferences.h>

struct W_EDITORFRAMEWORK_DLL WEngineViewPreferences
{
  WVec3 m_vCamPos = WVec3::MakeZero();
  WVec3 m_vCamDir = WVec3::MakeAxisX();
  WVec3 m_vCamUp = WVec3::MakeAxisZ();
  WSceneViewPerspective::Enum m_PerspectiveMode = WSceneViewPerspective::Perspective;
  WViewRenderMode::Enum m_RenderMode = WViewRenderMode::Default;
  float m_fFov = 70.0f;
};
W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WEngineViewPreferences);

class W_EDITORFRAMEWORK_DLL WQuadViewPreferencesUser : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WQuadViewPreferencesUser, WPreferences);

public:
  WQuadViewPreferencesUser();

  bool m_bQuadView;
  WEngineViewPreferences m_ViewSingle;
  WEngineViewPreferences m_ViewQuad0;
  WEngineViewPreferences m_ViewQuad1;
  WEngineViewPreferences m_ViewQuad2;
  WEngineViewPreferences m_ViewQuad3;

  WUInt32 FavCams_GetCount() const { return 10; }
  WEngineViewPreferences FavCams_GetCam(WUInt32 i) const { return m_FavoriteCamera[i]; }
  void FavCams_SetCam(WUInt32 i, WEngineViewPreferences cam) { m_FavoriteCamera[i] = cam; }
  void FavCams_Insert(WUInt32 uiIndex, WEngineViewPreferences cam) {}
  void FavCams_Remove(WUInt32 uiIndex) {}

  WEngineViewPreferences m_FavoriteCamera[10];
};
