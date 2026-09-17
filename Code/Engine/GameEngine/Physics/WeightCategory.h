#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

struct W_GAMEENGINE_DLL WWeightCategory
{
  WHashedString m_sName;
  float m_fMass = 10.0f;
  WString m_sDescription;
};

class W_GAMEENGINE_DLL WWeightCategoryConfig
{
public:
  WWeightCategoryConfig();
  ~WWeightCategoryConfig();

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/WeightCategories.cfg"_wsv;

  /// value for when a key doesn't exist
  static constexpr const WUInt8 InvalidKey = 255;

  /// any key smaller than this is not a valid weight category key, but may represent another option
  static constexpr const WUInt8 FirstValidKey = 10;

  /// the impulse type key with this value corresponds to "<Default>", which stands for a good default mass (that may be different for different component types)
  static constexpr const WUInt8 DefaultValueKey = 0;

  /// the impulse type key with this value corresponds to "<Custom Mass>"
  static constexpr const WUInt8 CustomMassKey = 1;

  /// the impulse type key with this value corresponds to "<Custom Density>"
  static constexpr const WUInt8 CustomDensityKey = 2;

  WResult Save(WStringView sFile = s_sConfigFile) const;
  WResult Load(WStringView sFile = s_sConfigFile);

  /// Returns the index of the element with the searched name, or InvalidKey, if it doesn't exist.
  WUInt8 FindByName(WTempHashedString sName) const;

  /// Returns the next free key, or InvalidKey, if the list is full.
  WUInt8 GetFreeKey() const;

  /// Returns the mass according to the weight category.
  ///
  /// * The default mass, if uiWeightCategory == DefaultValueKey
  /// * The custom mass, if uiWeightCategory == CustomMassKey
  /// * 0, if uiWeightCategory == CustomDensityKey
  /// * the mapped mass (scaled and clamped) for an existing key
  /// * the default value, if the key doesn't exist
  float GetMassForWeightCategory(WUInt8 uiWeightCategory, float fDefaultMass, float fCustomMass, float fWeightScale, float fMinMass = 1.0f, float fMaxMass = 1000.0f) const;

  WArrayMap<WUInt8, WWeightCategory> m_Categories;
};
