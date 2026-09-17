#pragma once

W_ALWAYS_INLINE WSimdQuat::WSimdQuat() = default;

W_ALWAYS_INLINE WSimdQuat::WSimdQuat(const WSimdVec4f& v)
  : m_v(v)
{
}

W_ALWAYS_INLINE const WSimdQuat WSimdQuat::MakeIdentity()
{
  return WSimdQuat(WSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f));
}

W_ALWAYS_INLINE WSimdQuat WSimdQuat::MakeFromElements(WSimdFloat x, WSimdFloat y, WSimdFloat z, WSimdFloat w)
{
  return WSimdQuat(WSimdVec4f(x, y, z, w));
}

inline WSimdQuat WSimdQuat::MakeFromAxisAndAngle(const WSimdVec4f& vRotationAxis, const WSimdFloat& fAngle)
{
  ///\todo optimize
  const WAngle halfAngle = WAngle::MakeFromRadian(fAngle) * 0.5f;
  float s = WMath::Sin(halfAngle);
  float c = WMath::Cos(halfAngle);

  WSimdQuat res;
  res.m_v = vRotationAxis * s;
  res.m_v.SetW(c);
  return res;
}

W_ALWAYS_INLINE void WSimdQuat::Normalize()
{
  m_v.Normalize<4>();
}

inline WResult WSimdQuat::GetRotationAxisAndAngle(WSimdVec4f& ref_vAxis, WSimdFloat& ref_fAngle, const WSimdFloat& fEpsilon) const
{
  ///\todo optimize
  const WAngle acos = WMath::ACos(float(m_v.w().Max(-1).Min(1)));
  const float d = WMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = acos * 2.0f;

  return W_SUCCESS;
}

W_ALWAYS_INLINE WSimdMat4f WSimdQuat::GetAsMat4() const
{
  const WSimdVec4f xyz = m_v;
  const WSimdVec4f x2y2z2 = xyz + xyz;
  const WSimdVec4f xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const WSimdVec4f yy2_xx2_xx2 = xx2yy2zz2.Get<WSwizzle::YXXX>();
  const WSimdVec4f zz2_zz2_yy2 = xx2yy2zz2.Get<WSwizzle::ZZYX>();
  WSimdVec4f diagonal = WSimdVec4f(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(WSimdFloat::MakeZero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const WSimdVec4f x_y_x = xyz.Get<WSwizzle::XYXX>();
  const WSimdVec4f y2_z2_z2 = x2y2z2.Get<WSwizzle::YZZX>();
  const WSimdVec4f base = x_y_x.CompMul(y2_z2_z2);

  const WSimdVec4f z2_x2_y2 = x2y2z2.Get<WSwizzle::ZXYX>();
  const WSimdVec4f offset = z2_x2_y2 * m_v.w();

  const WSimdVec4f adds = base + offset;
  const WSimdVec4f subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const WSimdVec4f addX_u_diaX_u = adds.GetCombined<WSwizzle::XXXX>(diagonal);
  const WSimdVec4f subZ_u_diaW_u = subs.GetCombined<WSwizzle::ZXWX>(diagonal);
  const WSimdVec4f col0 = addX_u_diaX_u.GetCombined<WSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const WSimdVec4f subX_u_diaY_u = subs.GetCombined<WSwizzle::XXYX>(diagonal);
  const WSimdVec4f addY_u_diaW_u = adds.GetCombined<WSwizzle::YXWX>(diagonal);
  const WSimdVec4f col1 = subX_u_diaY_u.GetCombined<WSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const WSimdVec4f addZ_u_subY_u = adds.GetCombined<WSwizzle::ZXYX>(subs);
  const WSimdVec4f col2 = addZ_u_subY_u.GetCombined<WSwizzle::XZZW>(diagonal);

  return WSimdMat4f::MakeFromColumns(col0, col1, col2, WSimdVec4f(0, 0, 0, 1));
}

W_ALWAYS_INLINE bool WSimdQuat::IsValid(const WSimdFloat& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

W_ALWAYS_INLINE bool WSimdQuat::IsNaN() const
{
  return m_v.IsNaN<4>();
}

W_ALWAYS_INLINE WSimdQuat WSimdQuat::operator-() const
{
  return WSimdQuat(m_v.FlipSign(WSimdVec4b(true, true, true, false)));
}

W_ALWAYS_INLINE WSimdVec4f WSimdQuat::operator*(const WSimdVec4f& v) const
{
  WSimdVec4f t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

W_ALWAYS_INLINE WSimdQuat WSimdQuat::operator*(const WSimdQuat& q2) const
{
  WSimdQuat q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

W_ALWAYS_INLINE bool WSimdQuat::operator==(const WSimdQuat& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

W_ALWAYS_INLINE bool WSimdQuat::operator!=(const WSimdQuat& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
