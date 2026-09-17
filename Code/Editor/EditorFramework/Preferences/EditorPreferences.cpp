#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorPreferencesUser, 1, WRTTIDefaultAllocator<WEditorPreferencesUser>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new WDefaultValueAttribute(false)),
    W_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("MaxAssetProcessors", m_uiMaxAssetProcessors)->AddAttributes(new WDefaultValueAttribute(8), new WClampValueAttribute(1, 8)),
    W_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new WDefaultValueAttribute(70.0f), new WClampValueAttribute(10.0f, 150.0f)),
    W_MEMBER_PROPERTY("CameraRotationSpeed", m_fCameraRotationSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 100.0f)),
    W_MEMBER_PROPERTY("MaxFramerate", m_uiMaxFramerate)->AddAttributes(new WDefaultValueAttribute(60)),
    W_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.2f, 5.0f)),
    W_ACCESSOR_PROPERTY("ShapeIconSize", GetShapeIconSize, SetShapeIconSize)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.2f, 5.0f)),
    W_ACCESSOR_PROPERTY("ShapeIconFadeDistance", GetShapeIconFadeDistance, SetShapeIconFadeDistance)->AddAttributes(new WDefaultValueAttribute(75.0f), new WClampValueAttribute(0.01f, 10000.0f)),
    W_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    W_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(15.0f)), new WHiddenAttribute()),
    W_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new WDefaultValueAttribute(0.125f), new WHiddenAttribute()),
    W_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new WDefaultValueAttribute(0.25f), new WHiddenAttribute()),
    W_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("CustomPrecompiledToolsFolder", m_sCustomPrecompiledToolsFolder),
    W_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("CombinedEditorAndEngineLogs", m_bCombinedEditorAndEngineLogs)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),
    W_MEMBER_PROPERTY("AssetBrowserShowItemsInSubFolders", m_bAssetBrowserShowItemsInSubFolders)->AddAttributes(new WDefaultValueAttribute(true), new WHiddenAttribute()),
    W_MEMBER_PROPERTY("AutoSaveMinutes", m_uiAutoSaveMinutes)->AddAttributes(new WDefaultValueAttribute(5), new WClampValueAttribute(0, 24 * 60)),

    // START GROUP Engine View Light Settings
    W_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new WDefaultValueAttribute(true), new WGroupAttribute("Engine View Light Settings")),
    W_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new WDefaultValueAttribute(true), new WClampValueAttribute(0.0f, 2.0f)),
    W_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new WDefaultValueAttribute(WStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new WAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    W_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 20.0f)),
    W_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(70.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(360.0f))),
    W_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    W_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new WDefaultValueAttribute(10.0f)),
    W_MEMBER_PROPERTY("Fog", m_bFog),
    W_MAP_ACCESSOR_PROPERTY("RecentLists", GetRecentLists, GetRecentList, SetRecentList, RemoveRecentList)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEditorPreferencesUser::WEditorPreferencesUser()
  : WPreferences(Domain::Application, "General")
{
  WQtSearchableMenuRecentList::SetStorage(&m_RecentLists);
}

WEditorPreferencesUser::~WEditorPreferencesUser()
{
  WQtSearchableMenuRecentList::SetStorage(nullptr);
}

const WRangeView<const char*, WUInt32> WEditorPreferencesUser::GetRecentLists() const
{
  return WRangeView<const char*, WUInt32>([]() -> WUInt32
    { return 0; },
    [this]() -> WUInt32
    { return m_RecentLists.GetCount(); },
    [](WUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const WUInt32& uiIt) -> const char*
    {
      auto it = m_RecentLists.GetIterator();
      for (WUInt32 i = 0; i < uiIt; ++i)
      {
        ++it;
      }
      return it.Key().GetData();
    });
}

void WEditorPreferencesUser::SetRecentList(const char* szKey, const WString& sValue)
{
  WDynamicArray<WString>& list = m_RecentLists[szKey];
  list.Clear();

  WTempHybridArray<WStringView, 32> entries;
  WStringView(sValue).Split(false, entries, ";");

  for (WStringView sEntry : entries)
  {
    list.PushBack(sEntry);
  }
}

void WEditorPreferencesUser::RemoveRecentList(const char* szKey)
{
  m_RecentLists.Remove(szKey);
}

bool WEditorPreferencesUser::GetRecentList(const char* szKey, WString& out_sValue) const
{
  auto it = m_RecentLists.Find(szKey);

  if (!it.IsValid())
    return false;

  WStringBuilder sTmp;

  for (const WString& sEntry : it.Value())
  {
    if (!sTmp.IsEmpty())
    {
      sTmp.Append(";");
    }

    sTmp.Append(sEntry.GetView());
  }

  out_sValue = sTmp;
  return true;
}

void WEditorPreferencesUser::ApplyDefaultValues(WEngineViewLightSettings& ref_settings)
{
  ref_settings.SetSkyBox(m_bSkyBox);
  ref_settings.SetSkyLight(m_bSkyLight);
  ref_settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  ref_settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  ref_settings.SetDirectionalLight(m_bDirectionalLight);
  ref_settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  ref_settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  ref_settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  ref_settings.SetFog(m_bFog);
}

void WEditorPreferencesUser::SetAsDefaultValues(const WEngineViewLightSettings& settings)
{
  m_bSkyBox = settings.GetSkyBox();
  m_bSkyLight = settings.GetSkyLight();
  m_sSkyLightCubeMap = settings.GetSkyLightCubeMap();
  m_fSkyLightIntensity = settings.GetSkyLightIntensity();
  m_bDirectionalLight = settings.GetDirectionalLight();
  m_DirectionalLightAngle = settings.GetDirectionalLightAngle();
  m_bDirectionalLightShadows = settings.GetDirectionalLightShadows();
  m_fDirectionalLightIntensity = settings.GetDirectionalLightIntensity();
  m_bFog = settings.GetFog();
  TriggerPreferencesChangedEvent();
}

void WEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;

  WQtTypeMenu::s_bShowInDevelopmentFeatures = b;
}

void WEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;

  WTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
}

void WEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
  SyncGlobalSettingsToEngine();
}

void WEditorPreferencesUser::SetShapeIconSize(float f)
{
  m_fShapeIconSize = f;
  SyncGlobalSettingsToEngine();
}

void WEditorPreferencesUser::SetShapeIconFadeDistance(float f)
{
  m_fShapeIconFadeDistance = f;
  SyncGlobalSettingsToEngine();
}

void WEditorPreferencesUser::SetMaxFramerate(WUInt16 uiFPS)
{
  if (m_uiMaxFramerate == uiFPS)
    return;

  m_uiMaxFramerate = uiFPS;
}

void WEditorPreferencesUser::SyncGlobalSettingsToEngine()
{
  WGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = m_fGizmoSize;
  msg.m_fShapeIconScale = m_fShapeIconSize;
  msg.m_fShapeIconFadeDistance = m_fShapeIconFadeDistance;

  WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void WQtEditorApp::LoadEditorPreferences()
{
  W_PROFILE_SCOPE("Preferences");
  WPreferences::QueryPreferences<WEditorPreferencesUser>();
}
