#pragma once

#include <Core/Utils/Blackboard.h>

namespace WInternal
{
  template <typename T>
  T ApplyBlackboardValueWithStrength(const T& currentValue, const WBlackboard& blackboard, WTempHashedString sName, WTempHashedString sNameStrength)
  {
    if (const auto* pEntry = blackboard.GetEntry(sName))
    {
      if (pEntry->m_Value.CanConvertTo<T>() == false)
      {
        return currentValue;
      }

      T newValue = pEntry->m_Value.ConvertTo<T>();
      if (const auto* pStrengthEntry = blackboard.GetEntry(sNameStrength))
      {
        const float fStrength = pStrengthEntry->m_Value.ConvertTo<float>();
        return WMath::Lerp(currentValue, newValue, fStrength);
      }

      return newValue;
    }

    return currentValue;
  }
} // namespace WInternal

#define W_STRENGTH_SUFFIX "_Strength"

#define W_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(currentValue, blackboard, name) \
  WInternal::ApplyBlackboardValueWithStrength(currentValue, blackboard, WTempHashedString(#name), WTempHashedString(#name W_STRENGTH_SUFFIX))
