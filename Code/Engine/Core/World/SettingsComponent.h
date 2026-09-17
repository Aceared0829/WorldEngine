#pragma once

#include <Core/World/Component.h>

/// Base class for settings components, of which only one per type should exist in each world.
///
/// Settings components are used to store global scene specific settings, e.g. for physics it would be the scene gravity,
/// for rendering it might be the time of day, fog settings, etc.
///
/// Components of this type should be managed by an WSettingsComponentManager, which makes it easy to query for the one instance
/// in the world.
class W_CORE_DLL WSettingsComponent : public WComponent
{
  W_ADD_DYNAMIC_REFLECTION(WSettingsComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WSettingsComponent

public:
  /// The constructor marks the component as modified.
  WSettingsComponent();
  ~WSettingsComponent();

  /// Marks the component as modified. Individual bits can be used to mark only specific settings (groups) as modified.
  void SetModified(WUInt32 uiBits = 0xFFFFFFFF) { m_uiSettingsModified |= uiBits; }

  /// Checks whether the component (or some settings group) was marked as modified.
  bool IsModified(WUInt32 uiBits = 0xFFFFFFFF) const { return (m_uiSettingsModified & uiBits) != 0; }

  /// Marks the settings as not-modified.
  void ResetModified(WUInt32 uiBits = 0xFFFFFFFF) { m_uiSettingsModified &= ~uiBits; }

private:
  WUInt32 m_uiSettingsModified = 0xFFFFFFFF;
};
