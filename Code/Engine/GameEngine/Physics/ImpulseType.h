#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

struct W_GAMEENGINE_DLL WImpulseType
{
  WHashedString m_sName;
  float m_fDefaultValue;
  WString m_sDescription;
  WArrayMap<WUInt8, float> m_WeightOverrides; // maps from WeightCategory index to impulse value
};

class W_GAMEENGINE_DLL WImpulseTypeConfig
{
public:
  WImpulseTypeConfig();
  ~WImpulseTypeConfig();

  /// value for when a key doesn't exist
  static constexpr const WUInt8 InvalidKey = 255;

  /// any key smaller than this is not a valid impulse type key, but may represent another option
  static constexpr const WUInt8 FirstValidKey = 10;

  /// the impulse type key with this value corresponds to "<Custom Value>"
  static constexpr const WUInt8 CustomValueKey = 0;

  /// the impulse type key with this value corresponds to "<None>"
  static constexpr const WUInt8 NoValueKey = 1;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/ImpulseTypes.cfg"_wsv;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);

  /// Returns the key of the element with the searched name, or InvalidKey, if it doesn't exist.
  WUInt8 FindByName(WTempHashedString sName) const;

  /// Returns the next free key, or InvalidKey, if the list is full.
  WUInt8 GetFreeKey() const;

  /// Looks up the impulse type and returns either the default impulse value or the override impulse for the given weight category.
  ///
  /// Returns 1 for uiImpulseType == CustomValueKey and for non-existing impulse types.
  /// Returns 0 for uiImpulseType == NoValueKey
  ///
  /// Multiply the returned value by the normalized impulse direction vector.
  float GetImpulseForWeight(WUInt8 uiImpulseType, WUInt8 uiWeightCategory) const;

  // maps from impulse type index to type
  WArrayMap<WUInt8, WImpulseType> m_Types;
};
