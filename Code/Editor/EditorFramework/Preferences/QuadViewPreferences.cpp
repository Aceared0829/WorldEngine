#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <Foundation/Serialization/GraphPatch.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WEngineViewPreferences, WNoBase, 2, WRTTIDefaultAllocator<WEngineViewPreferences>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CamPos", m_vCamPos),
    W_MEMBER_PROPERTY("CamDir", m_vCamDir),
    W_MEMBER_PROPERTY("CamUp", m_vCamUp),
    W_ENUM_MEMBER_PROPERTY("Perspective", WSceneViewPerspective, m_PerspectiveMode),
    W_ENUM_MEMBER_PROPERTY("RenderMode", WViewRenderMode, m_RenderMode),
    W_MEMBER_PROPERTY("FOV", m_fFov),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class WSceneViewPreferencesPatch_1_2 : public WGraphPatch
  {
  public:
    WSceneViewPreferencesPatch_1_2()
      : WGraphPatch("WSceneViewPreferences", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      ref_context.RenameClass("WEngineViewPreferences");
    }
  };
  WSceneViewPreferencesPatch_1_2 g_WSceneViewPreferencesPatch_1_2;
} // namespace

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WQuadViewPreferencesUser, 1, WRTTIDefaultAllocator<WQuadViewPreferencesUser>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("QuadView", m_bQuadView)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ViewSingle", m_ViewSingle)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ViewQuad0", m_ViewQuad0)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ViewQuad1", m_ViewQuad1)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ViewQuad2", m_ViewQuad2)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ViewQuad3", m_ViewQuad3)->AddAttributes(new WHiddenAttribute()),
    W_ARRAY_ACCESSOR_PROPERTY("FavoriteCams", FavCams_GetCount, FavCams_GetCam, FavCams_SetCam, FavCams_Insert, FavCams_Remove)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WQuadViewPreferencesUser::WQuadViewPreferencesUser()
  : WPreferences(Domain::Document, "View")
{
  m_bQuadView = false;

  m_ViewSingle.m_vCamPos.Set(3, 0.5f, 1);
  m_ViewSingle.m_vCamDir = -m_ViewSingle.m_vCamPos.GetNormalized();
  WVec3 vRight = m_ViewSingle.m_vCamDir.CrossRH(WVec3(0, 0, 1));
  m_ViewSingle.m_vCamUp = vRight.CrossRH(m_ViewSingle.m_vCamDir).GetNormalized();
  m_ViewSingle.m_PerspectiveMode = WSceneViewPerspective::Perspective;
  m_ViewSingle.m_RenderMode = WViewRenderMode::Default;
  m_ViewSingle.m_fFov = 70.0f;

  // Top Left: Top Down
  m_ViewQuad0.m_vCamPos.SetZero();
  m_ViewQuad0.m_vCamDir.Set(0, 0, -1);
  m_ViewQuad0.m_vCamUp.Set(1, 0, 0);
  m_ViewQuad0.m_PerspectiveMode = WSceneViewPerspective::Orthogonal_Top;
  m_ViewQuad0.m_RenderMode = WViewRenderMode::WireframeMonochrome;
  m_ViewQuad0.m_fFov = 20.0f;

  // Top Right: Perspective
  m_ViewQuad1.m_vCamPos = m_ViewSingle.m_vCamPos;
  m_ViewQuad1.m_vCamDir = m_ViewSingle.m_vCamDir;
  m_ViewQuad1.m_vCamUp = m_ViewSingle.m_vCamUp;
  m_ViewQuad1.m_PerspectiveMode = WSceneViewPerspective::Perspective;
  m_ViewQuad1.m_RenderMode = WViewRenderMode::Default;
  m_ViewQuad1.m_fFov = 70.0f;

  // Bottom Left: Front to Back
  m_ViewQuad2.m_vCamPos.SetZero();
  m_ViewQuad2.m_vCamDir.Set(-1, 0, 0);
  m_ViewQuad2.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad2.m_PerspectiveMode = WSceneViewPerspective::Orthogonal_Front;
  m_ViewQuad2.m_RenderMode = WViewRenderMode::WireframeMonochrome;
  m_ViewQuad2.m_fFov = 20.0f;

  // Bottom Right: Right to Left
  m_ViewQuad3.m_vCamPos.SetZero();
  m_ViewQuad3.m_vCamDir.Set(0, -1, 0);
  m_ViewQuad3.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad3.m_PerspectiveMode = WSceneViewPerspective::Orthogonal_Right;
  m_ViewQuad3.m_RenderMode = WViewRenderMode::WireframeMonochrome;
  m_ViewQuad3.m_fFov = 20.0f;
}
