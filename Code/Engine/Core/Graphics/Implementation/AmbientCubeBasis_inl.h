#pragma once

template <typename T>
W_ALWAYS_INLINE WAmbientCube<T>::WAmbientCube()
{
  WMemoryUtils::ZeroFillArray(m_Values);
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WAmbientCube<T>::WAmbientCube(const WAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
W_FORCE_INLINE void WAmbientCube<T>::operator=(const WAmbientCube<U>& other)
{
  for (WUInt32 i = 0; i < WAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
W_FORCE_INLINE bool WAmbientCube<T>::operator==(const WAmbientCube& other) const
{
  return WMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
W_ALWAYS_INLINE bool WAmbientCube<T>::operator!=(const WAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void WAmbientCube<T>::AddSample(const WVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += WMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += WMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += WMath::Abs(vDir.z) * value;
}

template <typename T>
T WAmbientCube<T>::Evaluate(const WVec3& vNormal) const
{
  WVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}

template <typename T>
WResult WAmbientCube<T>::Serialize(WStreamWriter& inout_stream) const
{
  return inout_stream.WriteArray(m_Values);
}

template <typename T>
WResult WAmbientCube<T>::Deserialize(WStreamReader& inout_stream)
{
  return inout_stream.ReadArray(m_Values);
}
