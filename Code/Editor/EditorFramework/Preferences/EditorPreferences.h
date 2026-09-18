#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RangeView.h>

class WEngineViewLightSettings;

/// Stores editor specific preferences for the current user
class W_EDITORFRAMEWORK_DLL WEditorPreferencesUser : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WEditorPreferencesUser, WPreferences);

public:
  WEditorPreferencesUser();
  ~WEditorPreferencesUser();

  void ApplyDefaultValues(WEngineViewLightSettings& ref_settings);
  void SetAsDefaultValues(const WEngineViewLightSettings& settings);

  float m_fPerspectiveFieldOfView = 70.0f;
  float m_fCameraRotationSpeed = 1.0f;
  WAngle m_RotationSnapValue = WAngle::MakeFromDegree(15.0f);
  float m_fScaleSnapValue = 0.125f;
  float m_fTranslationSnapValue = 0.25f;
  bool m_bUsePrecompiledTools = true;
  WString m_sCustomPrecompiledToolsFolder;
  bool m_bLoadLastProjectAtStartup = true;
  bool m_bShowSplashscreen = false;
  bool m_bExpandSceneTreeOnSelection = true;
  bool m_bBackgroundAssetProcessing = true;
  WUInt8 m_uiMaxAssetProcessors = 8;
  bool m_bHighlightUntranslatedUI = false;
  bool m_bAssetBrowserShowItemsInSubFolders = true;

  // Language code of the editor UI, e.g. "en" or "zh-CN". Must match a folder name below
  // Data/Tools/WEditor/Localization/. The value is applied while the editor starts up, so a
  // change only takes effect after a restart.
  WString m_sLanguage = "en";

  // Auto-save interval in minutes. 0 = off.
  WUInt32 m_uiAutoSaveMinutes = 5;

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  WString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;
  bool m_bDirectionalLight = true;
  WAngle m_DirectionalLightAngle = WAngle::MakeFromDegree(70.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;
  bool m_bFog = false;
  bool m_bClearEditorLogsOnPlay = true;
  bool m_bCombinedEditorAndEngineLogs = true;

  void SetShowInDevelopmentFeatures(bool b);
  bool GetShowInDevelopmentFeatures() const
  {
    return m_bShowInDevelopmentFeatures;
  }

  void SetHighlightUntranslatedUI(bool b);
  bool GetHighlightUntranslatedUI() const
  {
    return m_bHighlightUntranslatedUI;
  }

  void SetLanguage(const WString& sLanguage);
  const WString& GetLanguage() const
  {
    return m_sLanguage;
  }

  /// Returns the language code that the editor UI is currently using. Falls back to "en" when no
  /// preference is available or the stored value is empty.
  static const char* GetActiveLanguage();

  /// Name of the folder below Data/Tools/WEditor/Localization/ that holds the translation files
  /// for the given language, e.g. "en" or "zh-CN". Does not touch the file system.
  static WString GetLocalizationFolder(const char* szLanguage);

  void SetGizmoSize(float f);
  float GetGizmoSize() const { return m_fGizmoSize; }

  void SetShapeIconSize(float f);
  float GetShapeIconSize() const { return m_fShapeIconSize; }

  void SetShapeIconFadeDistance(float f);
  float GetShapeIconFadeDistance() const { return m_fShapeIconFadeDistance; }

  void SetMaxFramerate(WUInt16 uiFPS);
  WUInt16 GetMaxFramerate() const { return m_uiMaxFramerate; }

  // The 'recently used' lists of the searchable menus, one list per use case (see WQtSearchableMenuRecentList).
  // Reflected as a map of strings, where each value holds the entries of one list, separated by semicolons.
  const WRangeView<const char*, WUInt32> GetRecentLists() const;   // [ property ]
  void SetRecentList(const char* szKey, const WString& sValue);     // [ property ]
  void RemoveRecentList(const char* szKey);                          // [ property ]
  bool GetRecentList(const char* szKey, WString& out_sValue) const; // [ property ]

  WMap<WString, WDynamicArray<WString>> m_RecentLists;

  void SyncGlobalSettingsToEngine();

private:
  float m_fGizmoSize = 1.5f;
  float m_fShapeIconSize = 1.0f;
  float m_fShapeIconFadeDistance = 75.0f;
  bool m_bShowInDevelopmentFeatures = false;
  WUInt16 m_uiMaxFramerate = 60;
};
