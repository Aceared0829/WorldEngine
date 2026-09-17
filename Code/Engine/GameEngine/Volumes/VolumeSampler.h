#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/SpatialData.h>
#include <Foundation/Types/Variant.h>

class WBlackboard;

/// A volume sampler is used to sample the registered values from volumes at a given position. It also takes care of interpolation over time of those values.
class W_GAMEENGINE_DLL WVolumeSampler
{
public:
  WVolumeSampler();
  ~WVolumeSampler();

  void RegisterValue(WHashedString sName, WVariant defaultValue, WTime interpolationDuration = WTime::MakeZero());
  void DeregisterValue(WHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(const WWorld& world, WSpatialData::Category spatialCategory, const WVec3& vGlobalPosition, WTime deltaTime, WBlackboard* pTargetBlackboard = nullptr);

  WVariant GetValue(WTempHashedString sName) const
  {
    if (const Value* pValue = m_Values.GetValue(sName))
    {
      return pValue->m_CurrentValue;
    }

    return WVariant();
  }

  static WUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

private:
  struct Value
  {
    WVariant m_DefaultValue;
    WVariant m_CurrentValue;
    double m_fInterpolationFactor = -1.0;

    WHashedString m_sStrengthName;
    float m_fCurrentStrength = 0.0f;
  };

  WHashTable<WHashedString, Value> m_Values;
};
