#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdVec4i.h>

namespace WSimdConversion
{
  W_ALWAYS_INLINE WVec3 ToVec3(const WSimdVec4f& v)
  {
    WVec4 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<WVec3*>(&tmp.x);
  }

  W_ALWAYS_INLINE WSimdVec4f ToVec3(const WVec3& v)
  {
    WSimdVec4f tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  W_ALWAYS_INLINE WVec3I32 ToVec3i(const WSimdVec4i& v)
  {
    WVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<WVec3I32*>(&tmp.x);
  }

  W_ALWAYS_INLINE WSimdVec4i ToVec3i(const WVec3I32& v)
  {
    WSimdVec4i tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  W_ALWAYS_INLINE WVec4 ToVec4(const WSimdVec4f& v)
  {
    WVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  W_ALWAYS_INLINE WSimdVec4f ToVec4(const WVec4& v)
  {
    WSimdVec4f tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  W_ALWAYS_INLINE WVec4I32 ToVec4i(const WSimdVec4i& v)
  {
    WVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  W_ALWAYS_INLINE WSimdVec4i ToVec4i(const WVec4I32& v)
  {
    WSimdVec4i tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  W_ALWAYS_INLINE WQuat ToQuat(const WSimdQuat& q)
  {
    WQuat tmp;
    q.m_v.Store<4>(&tmp.x);
    return tmp;
  }

  W_ALWAYS_INLINE WSimdQuat ToQuat(const WQuat& q)
  {
    WSimdVec4f tmp;
    tmp.Load<4>(&q.x);
    return WSimdQuat(tmp);
  }

  W_ALWAYS_INLINE WTransform ToTransform(const WSimdTransform& t)
  {
    return WTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline WSimdTransform ToTransform(const WTransform& t)
  {
    return WSimdTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  W_ALWAYS_INLINE WMat4 ToMat4(const WSimdMat4f& m)
  {
    WMat4 tmp;
    m.GetAsArray(tmp.m_fElementsCM, WMatrixLayout::ColumnMajor);
    return tmp;
  }

  W_ALWAYS_INLINE WSimdMat4f ToMat4(const WMat4& m)
  {
    return WSimdMat4f::MakeFromColumnMajorArray(m.m_fElementsCM);
  }

  W_ALWAYS_INLINE WBoundingBoxSphere ToBBoxSphere(const WSimdBBoxSphere& b)
  {
    WVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return WBoundingBoxSphere::MakeFromCenterExtents(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  W_ALWAYS_INLINE WSimdBBoxSphere ToBBoxSphere(const WBoundingBoxSphere& b)
  {
    return WSimdBBoxSphere::MakeFromCenterExtents(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtents), b.m_fSphereRadius);
  }

  W_ALWAYS_INLINE WBoundingSphere ToBSphere(const WSimdBSphere& s)
  {
    WVec4 centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return WBoundingSphere::MakeFromCenterAndRadius(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  W_ALWAYS_INLINE WSimdBSphere ToBSphere(const WBoundingSphere& s)
  {
    return WSimdBSphere(ToVec3(s.m_vCenter), s.m_fRadius);
  }

  W_ALWAYS_INLINE WSimdBBox ToBBox(const WBoundingBox& b)
  {
    return WSimdBBox(ToVec3(b.m_vMin), ToVec3(b.m_vMax));
  }

  W_ALWAYS_INLINE WBoundingBox ToBBox(const WSimdBBox& b)
  {
    return WBoundingBox::MakeFromMinMax(ToVec3(b.m_Min), ToVec3(b.m_Max));
  }

}; // namespace WSimdConversion
