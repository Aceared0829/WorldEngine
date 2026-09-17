#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdTransform.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Color.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Vec4.h>

W_DEFINE_AS_POD_TYPE(JPH::Vec3);

namespace WJoltConversionUtils
{
  W_ALWAYS_INLINE WVec3 ToVec3(const JPH::Vec3& v)
  {
    return WVec3(v.mF32[0], v.mF32[1], v.mF32[2]);
  }

  W_ALWAYS_INLINE WVec3 ToVec3(const JPH::Float3& v)
  {
    return reinterpret_cast<const WVec3&>(v);
  }

  W_ALWAYS_INLINE WColor ToColor(const JPH::ColorArg& c)
  {
    const JPH::Vec4 v4 = c.ToVec4();
    return reinterpret_cast<const WColor&>(v4);
  }

  W_ALWAYS_INLINE WSimdVec4f ToSimdVec3(const JPH::Vec3& v)
  {
    return WSimdVec4f(v.mF32[0], v.mF32[1], v.mF32[2], v.mF32[3]);
  }

  W_ALWAYS_INLINE JPH::Vec3 ToVec3(const WVec3& v)
  {
    return JPH::Vec3(v.x, v.y, v.z);
  }

  W_ALWAYS_INLINE JPH::Float3 ToFloat3(const WVec3& v)
  {
    return reinterpret_cast<const JPH::Float3&>(v);
  }

  W_ALWAYS_INLINE JPH::Vec3 ToVec3(const WSimdVec4f& v)
  {
    return reinterpret_cast<const JPH::Vec3&>(v);
  }

  W_ALWAYS_INLINE WQuat ToQuat(const JPH::Quat& q)
  {
    return reinterpret_cast<const WQuat&>(q);
  }

  W_ALWAYS_INLINE WSimdQuat ToSimdQuat(const JPH::Quat& q)
  {
    return reinterpret_cast<const WSimdQuat&>(q);
  }

  W_ALWAYS_INLINE JPH::Quat ToQuat(const WQuat& q)
  {
    return JPH::Quat(q.x, q.y, q.z, q.w);
  }

  W_ALWAYS_INLINE JPH::Quat ToQuat(const WSimdQuat& q)
  {
    return reinterpret_cast<const JPH::Quat&>(q);
  }

  W_ALWAYS_INLINE WTransform ToTransform(const JPH::Vec3& pos, const JPH::Quat& rot)
  {
    return WTransform(ToVec3(pos), ToQuat(rot));
  }

  W_ALWAYS_INLINE WTransform ToTransform(const JPH::Vec3& pos)
  {
    return WTransform(ToVec3(pos));
  }

} // namespace WJoltConversionUtils
