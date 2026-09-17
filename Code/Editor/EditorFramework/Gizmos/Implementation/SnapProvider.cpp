#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Configuration/SubSystem.h>

WAngle WSnapProvider::s_RotationSnapValue = WAngle::MakeFromDegree(15.0f);
float WSnapProvider::s_fScaleSnapValue = 0.125f;
float WSnapProvider::s_fTranslationSnapValue = 0.25f;
WEventSubscriptionID WSnapProvider::s_UserPreferencesChanged = 0;

WEvent<const WSnapProviderEvent&> WSnapProvider::s_Events;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, SnapProvider)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "EditorFrameworkMain"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WSnapProvider::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WSnapProvider::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

void WSnapProvider::Startup()
{
  WQtEditorApp::m_Events.AddEventHandler(WMakeDelegate(&WSnapProvider::EditorEventHandler));
}

void WSnapProvider::Shutdown()
{
  if (s_UserPreferencesChanged)
  {
    WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(s_UserPreferencesChanged);
  }
  WQtEditorApp::m_Events.RemoveEventHandler(WMakeDelegate(&WSnapProvider::EditorEventHandler));
}

void WSnapProvider::EditorEventHandler(const WEditorAppEvent& e)
{
  if (e.m_Type == WEditorAppEvent::Type::EditorStarted)
  {
    WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
    PreferenceChangedEventHandler(pPreferences);
    s_UserPreferencesChanged = pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WSnapProvider::PreferenceChangedEventHandler));
  }
}

void WSnapProvider::PreferenceChangedEventHandler(WPreferences* pPreferenceBase)
{
  auto* pPreferences = static_cast<WEditorPreferencesUser*>(pPreferenceBase);
  SetRotationSnapValue(pPreferences->m_RotationSnapValue);
  SetScaleSnapValue(pPreferences->m_fScaleSnapValue);
  SetTranslationSnapValue(pPreferences->m_fTranslationSnapValue);
}

WAngle WSnapProvider::GetRotationSnapValue()
{
  return s_RotationSnapValue;
}

float WSnapProvider::GetScaleSnapValue()
{
  return s_fScaleSnapValue;
}

float WSnapProvider::GetTranslationSnapValue()
{
  return s_fTranslationSnapValue;
}

void WSnapProvider::SetRotationSnapValue(WAngle angle)
{
  if (s_RotationSnapValue == angle)
    return;

  s_RotationSnapValue = angle;

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_RotationSnapValue = angle;
  pPreferences->TriggerPreferencesChangedEvent();

  WSnapProviderEvent e;
  e.m_Type = WSnapProviderEvent::Type::RotationSnapChanged;
  s_Events.Broadcast(e);
}

void WSnapProvider::SetScaleSnapValue(float fPercentage)
{
  if (s_fScaleSnapValue == fPercentage)
    return;

  s_fScaleSnapValue = fPercentage;

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_fScaleSnapValue = fPercentage;
  pPreferences->TriggerPreferencesChangedEvent();

  WSnapProviderEvent e;
  e.m_Type = WSnapProviderEvent::Type::ScaleSnapChanged;
  s_Events.Broadcast(e);
}

void WSnapProvider::SetTranslationSnapValue(float fUnits)
{
  if (s_fTranslationSnapValue == fUnits)
    return;

  s_fTranslationSnapValue = fUnits;

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();
  pPreferences->m_fTranslationSnapValue = fUnits;
  pPreferences->TriggerPreferencesChangedEvent();

  WSnapProviderEvent e;
  e.m_Type = WSnapProviderEvent::Type::TranslationSnapChanged;
  s_Events.Broadcast(e);
}

void WSnapProvider::SnapTranslation(WVec3& value)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  value.x = WMath::RoundToMultiple(value.x, s_fTranslationSnapValue);
  value.y = WMath::RoundToMultiple(value.y, s_fTranslationSnapValue);
  value.z = WMath::RoundToMultiple(value.z, s_fTranslationSnapValue);
}

void WSnapProvider::SnapTranslationInLocalSpace(const WQuat& qRotation, WVec3& ref_vTranslation)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  const WQuat mInvRot = qRotation.GetInverse();

  WVec3 vLocalTranslation = mInvRot * ref_vTranslation;
  vLocalTranslation.x = WMath::RoundToMultiple(vLocalTranslation.x, s_fTranslationSnapValue);
  vLocalTranslation.y = WMath::RoundToMultiple(vLocalTranslation.y, s_fTranslationSnapValue);
  vLocalTranslation.z = WMath::RoundToMultiple(vLocalTranslation.z, s_fTranslationSnapValue);

  ref_vTranslation = qRotation * vLocalTranslation;
}

void WSnapProvider::SnapRotation(WAngle& ref_rotation)
{
  if (s_RotationSnapValue.GetRadian() != 0.0f)
  {
    ref_rotation = WAngle::MakeFromRadian(WMath::RoundToMultiple(ref_rotation.GetRadian(), s_RotationSnapValue.GetRadian()));
  }
}

void WSnapProvider::SnapScale(float& ref_fScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    ref_fScale = WMath::RoundToMultiple(ref_fScale, s_fScaleSnapValue);
  }
}

void WSnapProvider::SnapScale(WVec3& ref_vScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    SnapScale(ref_vScale.x);
    SnapScale(ref_vScale.y);
    SnapScale(ref_vScale.z);
  }
}

WVec3 WSnapProvider::GetScaleSnapped(const WVec3& vScale)
{
  WVec3 res = vScale;
  SnapScale(res);
  return res;
}
