#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

/// A 32x32 matrix of named filters that can be configured to enable or disable collisions
class W_GAMEENGINE_DLL WCollisionFilterConfig
{
public:
  WCollisionFilterConfig();
  ~WCollisionFilterConfig();

  void SetGroupName(WUInt32 uiGroup, WStringView sName);

  WStringView GetGroupName(WUInt32 uiGroup) const;

  void EnableCollision(WUInt32 uiGroup1, WUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(WUInt32 uiGroup1, WUInt32 uiGroup2) const;

  inline WUInt32 GetFilterMask(WUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// Returns how many groups have non-empty names
  WUInt32 GetNumNamedGroups() const;

  /// Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in
  /// between)
  WUInt32 GetNamedGroupIndex(WUInt32 uiGroup) const;

  /// Returns WInvalidIndex if no group with the given name exists.
  WUInt32 GetFilterGroupByName(WStringView sName) const;

  /// Searches for a group without a name and returns the index or WInvalidIndex if none found.
  WUInt32 FindUnnamedGroup() const;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/CollisionLayers.cfg"_wsv;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);


private:
  WUInt32 m_GroupMasks[32];
  WString m_GroupNames[32];
};
