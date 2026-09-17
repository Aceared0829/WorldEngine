#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/TypeTraits.h>

#define W_DECLARE_VARIANCE_HASH_HELPER(TYPE)                        \
  template <>                                                        \
  struct WHashHelper<TYPE>                                          \
  {                                                                  \
    W_ALWAYS_INLINE static WUInt32 Hash(const TYPE& value)         \
    {                                                                \
      return WHashingUtils::xxHash32(&value, sizeof(TYPE));         \
    }                                                                \
    W_ALWAYS_INLINE static bool Equal(const TYPE& a, const TYPE& b) \
    {                                                                \
      return a == b;                                                 \
    }                                                                \
  };

struct W_FOUNDATION_DLL WVarianceTypeBase
{
  W_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WVarianceTypeBase);

struct W_FOUNDATION_DLL WVarianceTypeFloat : public WVarianceTypeBase
{
  W_DECLARE_POD_TYPE();
  WVarianceTypeFloat() = default;
  WVarianceTypeFloat(float value, float fVariance = 0.0f)
    : m_Value(value)
  {
    m_fVariance = fVariance;
  }

  bool operator==(const WVarianceTypeFloat& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const WVarianceTypeFloat& rhs) const
  {
    return !(*this == rhs);
  }
  float m_Value = 0;
};

W_DECLARE_VARIANCE_HASH_HELPER(WVarianceTypeFloat);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WVarianceTypeFloat);
W_DECLARE_CUSTOM_VARIANT_TYPE(WVarianceTypeFloat);

struct W_FOUNDATION_DLL WVarianceTypeTime : public WVarianceTypeBase
{
  W_DECLARE_POD_TYPE();
  WVarianceTypeTime() = default;
  WVarianceTypeTime(WTime value, float fVariance = 0.0f)
    : m_Value(value)
  {
    m_fVariance = fVariance;
  }

  bool operator==(const WVarianceTypeTime& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const WVarianceTypeTime& rhs) const
  {
    return !(*this == rhs);
  }
  WTime m_Value;
};

W_DECLARE_VARIANCE_HASH_HELPER(WVarianceTypeTime);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WVarianceTypeTime);
W_DECLARE_CUSTOM_VARIANT_TYPE(WVarianceTypeTime);

struct W_FOUNDATION_DLL WVarianceTypeAngle : public WVarianceTypeBase
{
  W_DECLARE_POD_TYPE();
  WVarianceTypeAngle() = default;
  WVarianceTypeAngle(WAngle value, float fVariance = 0.0f)
    : m_Value(value)
  {
    m_fVariance = fVariance;
  }

  bool operator==(const WVarianceTypeAngle& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const WVarianceTypeAngle& rhs) const
  {
    return !(*this == rhs);
  }
  WAngle m_Value;
};

W_DECLARE_VARIANCE_HASH_HELPER(WVarianceTypeAngle);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WVarianceTypeAngle);
W_DECLARE_CUSTOM_VARIANT_TYPE(WVarianceTypeAngle);
