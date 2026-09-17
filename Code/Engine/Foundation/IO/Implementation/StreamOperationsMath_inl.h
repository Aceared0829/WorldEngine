#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

// WFloat16

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WFloat16 value)
{
  inout_stream.WriteWordValue(&value).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WFloat16& ref_value)
{
  inout_stream.ReadWordValue(&ref_value).AssertSuccess();
  return inout_stream;
}

// WFloat16Vec2

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WFloat16Vec2& value)
{
  inout_stream.WriteBytes(&value, sizeof(WFloat16Vec2)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WFloat16Vec2& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WFloat16Vec2)) == sizeof(WFloat16Vec2), "End of stream reached.");
  return inout_stream;
}

// WFloat16Vec3

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WFloat16Vec3& value)
{
  inout_stream.WriteBytes(&value, sizeof(WFloat16Vec3)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WFloat16Vec3& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WFloat16Vec3)) == sizeof(WFloat16Vec3), "End of stream reached.");
  return inout_stream;
}

// WFloat16Vec4

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WFloat16Vec4& value)
{
  inout_stream.WriteBytes(&value, sizeof(WFloat16Vec4)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WFloat16Vec4& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WFloat16Vec4)) == sizeof(WFloat16Vec4), "End of stream reached.");
  return inout_stream;
}

// WVec2Template

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WVec2Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(WVec2Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WVec2Template<Type>& ref_vValue)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(WVec2Template<Type>)) == sizeof(WVec2Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WVec2Template<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WVec2Template<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WVec2Template<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WVec2Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WVec3Template

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WVec3Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(WVec3Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WVec3Template<Type>& ref_vValue)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(WVec3Template<Type>)) == sizeof(WVec3Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WVec3Template<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WVec3Template<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WVec3Template<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WVec3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WVec4Template

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WVec4Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(WVec4Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WVec4Template<Type>& ref_vValue)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(WVec4Template<Type>)) == sizeof(WVec4Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WVec4Template<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WVec4Template<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WVec4Template<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WVec4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WMat3Template

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WMat3Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WMat3Template<Type>& ref_mValue)
{
  W_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 9) == sizeof(Type) * 9, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WMat3Template<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WMat3Template<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WMat3Template<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WMat3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WMat4Template

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WMat4Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WMat4Template<Type>& ref_mValue)
{
  W_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 16) == sizeof(Type) * 16, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WMat4Template<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WMat4Template<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WMat4Template<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WMat4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WTransformTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WTransformTemplate<Type>& value)
{
  inout_stream << value.m_qRotation;
  inout_stream << value.m_vPosition;
  inout_stream << value.m_vScale;

  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WTransformTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_qRotation;
  inout_stream >> out_value.m_vPosition;
  inout_stream >> out_value.m_vScale;

  return inout_stream;
}

// WPlaneTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WPlaneTemplate<Type>& value)
{
  inout_stream.WriteBytes(&value, sizeof(WPlaneTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WPlaneTemplate<Type>& out_value)
{
  W_VERIFY(inout_stream.ReadBytes(&out_value, sizeof(WPlaneTemplate<Type>)) == sizeof(WPlaneTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WPlaneTemplate<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WPlaneTemplate<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WPlaneTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WQuatTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WQuatTemplate<Type>& qValue)
{
  inout_stream.WriteBytes(&qValue, sizeof(WQuatTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WQuatTemplate<Type>& ref_qValue)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_qValue, sizeof(WQuatTemplate<Type>)) == sizeof(WQuatTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WQuatTemplate<Type>* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WQuatTemplate<Type>) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WQuatTemplate<Type>* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WQuatTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WBoundingBoxTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WBoundingBoxTemplate<Type>& value)
{
  inout_stream << value.m_vMax;
  inout_stream << value.m_vMin;
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WBoundingBoxTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vMax;
  inout_stream >> out_value.m_vMin;
  return inout_stream;
}

// WBoundingSphereTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WBoundingSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fRadius;
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WBoundingSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fRadius;
  return inout_stream;
}

// WBoundingBoxSphereTemplate

template <typename Type>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WBoundingBoxSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fSphereRadius;
  inout_stream << value.m_vBoxHalfExtents;
  return inout_stream;
}

template <typename Type>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WBoundingBoxSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fSphereRadius;
  inout_stream >> out_value.m_vBoxHalfExtents;
  return inout_stream;
}

// WColor
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WColor& value)
{
  inout_stream.WriteBytes(&value, sizeof(WColor)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WColor& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WColor)) == sizeof(WColor), "End of stream reached.");
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WColor* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WColor) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WColor* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WColor) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WColorGammaUB
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WColorGammaUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(WColorGammaUB)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WColorGammaUB& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WColorGammaUB)) == sizeof(WColorGammaUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WColorGammaUB* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WColorGammaUB) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WColorGammaUB* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WColorGammaUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WAngle
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WAngle& value)
{
  inout_stream << value.GetRadian();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WAngle& out_value)
{
  float fRadian;
  inout_stream >> fRadian;
  out_value.SetRadian(fRadian);
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WAngle* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WAngle) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WAngle* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WAngle) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WColor8Unorm
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WColorLinearUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(WColorLinearUB)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WColorLinearUB& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WColorLinearUB)) == sizeof(WColorLinearUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
WResult SerializeArray(WStreamWriter& inout_stream, const WColorLinearUB* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WColorLinearUB) * uiCount);
}

template <typename Type>
WResult DeserializeArray(WStreamReader& inout_stream, WColorLinearUB* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WColorLinearUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// WColorLinear16f
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WColorLinear16f& value)
{
  inout_stream.WriteBytes(&value, sizeof(WColorLinear16f)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WColorLinear16f& ref_value)
{
  W_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(WColorLinear16f)) == sizeof(WColorLinear16f), "End of stream reached.");
  return inout_stream;
}
