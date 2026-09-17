#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Math/Declarations.h>

struct WEditorAppEvent;
class WPreferences;

struct WSnapProviderEvent
{
  enum class Type
  {
    RotationSnapChanged,
    ScaleSnapChanged,
    TranslationSnapChanged
  };

  Type m_Type;
};

class W_EDITORFRAMEWORK_DLL WSnapProvider
{
public:
  static void Startup();
  static void Shutdown();

  static WAngle GetRotationSnapValue();
  static float GetScaleSnapValue();
  static float GetTranslationSnapValue();

  static void SetRotationSnapValue(WAngle angle);
  static void SetScaleSnapValue(float fPercentage);
  static void SetTranslationSnapValue(float fUnits);

  /// Rounds each component to the closest translation snapping value
  static void SnapTranslation(WVec3& value);

  /// Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const WQuat& qRotation, WVec3& ref_vTranslation);

  static void SnapRotation(WAngle& ref_rotation);

  static void SnapScale(float& ref_fScale);
  static void SnapScale(WVec3& ref_vScale);

  static WVec3 GetScaleSnapped(const WVec3& vScale);

  static WEvent<const WSnapProviderEvent&> s_Events;

private:
  static void EditorEventHandler(const WEditorAppEvent& e);
  static void PreferenceChangedEventHandler(WPreferences* pPreferenceBase);

  static WAngle s_RotationSnapValue;
  static float s_fScaleSnapValue;
  static float s_fTranslationSnapValue;
  static WEventSubscriptionID s_UserPreferencesChanged;
};
