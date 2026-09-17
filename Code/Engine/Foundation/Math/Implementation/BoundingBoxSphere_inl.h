#pragma once

template <typename Type>
W_FORCE_INLINE WBoundingBoxSphereTemplate<Type>::WBoundingBoxSphereTemplate()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vOrigin and m_vBoxHalfExtents are already initialized to NaN by their own constructor.
  const Type TypeNaN = WMath::NaN<Type>();
  m_fSphereRadius = TypeNaN;
#endif
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxSphereTemplate<Type>::WBoundingBoxSphereTemplate(const WBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtents = rhs.m_vBoxHalfExtents;
}

template <typename Type>
void WBoundingBoxSphereTemplate<Type>::operator=(const WBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtents = rhs.m_vBoxHalfExtents;
}

template <typename Type>
WBoundingBoxSphereTemplate<Type>::WBoundingBoxSphereTemplate(const WBoundingBoxTemplate<Type>& box)
  : m_vCenter(box.GetCenter())
{
  m_vBoxHalfExtents = box.GetHalfExtents();
  m_fSphereRadius = m_vBoxHalfExtents.GetLength();
}

template <typename Type>
WBoundingBoxSphereTemplate<Type>::WBoundingBoxSphereTemplate(const WBoundingSphereTemplate<Type>& sphere)
  : m_vCenter(sphere.m_vCenter)
  , m_fSphereRadius(sphere.m_fRadius)
{
  m_vBoxHalfExtents.Set(m_fSphereRadius);
}


template <typename Type>
W_FORCE_INLINE WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeZero()
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = 0;
  res.m_vBoxHalfExtents.SetZero();
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeInvalid()
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = -WMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  res.m_vBoxHalfExtents.Set(-WMath::MaxValue<Type>());
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeFromCenterExtents(const WVec3Template<Type>& vCenter, const WVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius)
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fSphereRadius = fSphereRadius;
  res.m_vBoxHalfExtents = vBoxHalfExtents;
  return res;
}

template <typename Type>
WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WVec3Template<Type>)*/)
{
  WBoundingBoxTemplate<Type> box = WBoundingBoxTemplate<Type>::MakeFromPoints(pPoints, uiNumPoints, uiStride);

  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();

  WBoundingSphereTemplate<Type> sphere = WBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(res.m_vCenter, 0.0f);
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  res.m_fSphereRadius = sphere.m_fRadius;
  return res;
}

template <typename Type>
WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeFromBox(const WBoundingBoxTemplate<Type>& box)
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();
  res.m_fSphereRadius = res.m_vBoxHalfExtents.GetLength();
  return res;
}

template <typename Type>
WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeFromSphere(const WBoundingSphereTemplate<Type>& sphere)
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = sphere.m_vCenter;
  res.m_fSphereRadius = sphere.m_fRadius;
  res.m_vBoxHalfExtents.Set(res.m_fSphereRadius);
  return res;
}

template <typename Type>
WBoundingBoxSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::MakeFromBoxAndSphere(const WBoundingBoxTemplate<Type>& box, const WBoundingSphereTemplate<Type>& sphere)
{
  WBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtents = box.GetHalfExtents();
  res.m_fSphereRadius = WMath::Min(res.m_vBoxHalfExtents.GetLength(), (sphere.m_vCenter - res.m_vCenter).GetLength() + sphere.m_fRadius);
  return res;
}

template <typename Type>
W_FORCE_INLINE bool WBoundingBoxSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fSphereRadius >= 0.0f && m_vBoxHalfExtents.IsValid() && (m_vBoxHalfExtents.x >= 0) && (m_vBoxHalfExtents.y >= 0) && (m_vBoxHalfExtents.z >= 0));
}

template <typename Type>
W_FORCE_INLINE bool WBoundingBoxSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || WMath::IsNaN(m_fSphereRadius) || m_vBoxHalfExtents.IsNaN());
}

template <typename Type>
W_FORCE_INLINE const WBoundingBoxTemplate<Type> WBoundingBoxSphereTemplate<Type>::GetBox() const
{
  return WBoundingBoxTemplate<Type>::MakeFromMinMax(m_vCenter - m_vBoxHalfExtents, m_vCenter + m_vBoxHalfExtents);
}

template <typename Type>
W_FORCE_INLINE const WBoundingSphereTemplate<Type> WBoundingBoxSphereTemplate<Type>::GetSphere() const
{
  return WBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(m_vCenter, m_fSphereRadius);
}

template <typename Type>
void WBoundingBoxSphereTemplate<Type>::ExpandToInclude(const WBoundingBoxSphereTemplate& rhs)
{
  WBoundingBoxTemplate<Type> box;
  box.m_vMin = m_vCenter - m_vBoxHalfExtents;
  box.m_vMax = m_vCenter + m_vBoxHalfExtents;
  box.ExpandToInclude(rhs.GetBox());

  WBoundingBoxSphereTemplate<Type> result = WBoundingBoxSphereTemplate<Type>::MakeFromBox(box);

  const Type fSphereRadiusA = (m_vCenter - result.m_vCenter).GetLength() + m_fSphereRadius;
  const Type fSphereRadiusB = (rhs.m_vCenter - result.m_vCenter).GetLength() + rhs.m_fSphereRadius;

  m_vCenter = result.m_vCenter;
  m_fSphereRadius = WMath::Min(result.m_fSphereRadius, WMath::Max(fSphereRadiusA, fSphereRadiusB));
  m_vBoxHalfExtents = result.m_vBoxHalfExtents;
}

template <typename Type>
void WBoundingBoxSphereTemplate<Type>::Transform(const WMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);
  const WVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fSphereRadius *= WMath::Max(Scale.x, Scale.y, Scale.z);

  WMat3Template<Type> mAbsRotation = mTransform.GetRotationalPart();
  for (WUInt32 i = 0; i < 9; ++i)
  {
    mAbsRotation.m_fElementsCM[i] = WMath::Abs(mAbsRotation.m_fElementsCM[i]);
  }

  m_vBoxHalfExtents = mAbsRotation.TransformDirection(m_vBoxHalfExtents).CompMin(WVec3Template<Type>(m_fSphereRadius));
}

template <typename Type>
W_FORCE_INLINE bool operator==(const WBoundingBoxSphereTemplate<Type>& lhs, const WBoundingBoxSphereTemplate<Type>& rhs)
{
  return lhs.m_vCenter == rhs.m_vCenter && lhs.m_vBoxHalfExtents == rhs.m_vBoxHalfExtents && lhs.m_fSphereRadius == rhs.m_fSphereRadius;
}

/// Checks whether this box and the other are not identical.
template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WBoundingBoxSphereTemplate<Type>& lhs, const WBoundingBoxSphereTemplate<Type>& rhs)
{
  return !(lhs == rhs);
}
