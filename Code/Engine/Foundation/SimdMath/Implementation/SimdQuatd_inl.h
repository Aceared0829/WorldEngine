#pragma once

W_ALWAYS_INLINE WSimdQuatd::WSimdQuatd() = default;

W_ALWAYS_INLINE WSimdQuatd::WSimdQuatd(const WSimdVec4d& v)
  : m_v(v)
{
}

W_ALWAYS_INLINE const WSimdQuatd WSimdQuatd::MakeIdentity()
{
  return WSimdQuatd(WSimdVec4d(0.0f, 0.0f, 0.0f, 1.0f));
}

W_ALWAYS_INLINE WSimdQuatd WSimdQuatd::MakeFromElements(WSimdDouble x, WSimdDouble y, WSimdDouble z, WSimdDouble w)
{
  return WSimdQuatd(WSimdVec4d(x, y, z, w));
}

inline WSimdQuatd WSimdQuatd::MakeFromAxisAndAngle(const WSimdVec4d& vRotationAxis, const WSimdDouble& fAngle)
{
  ///\todo optimize
  const WAngled halfAngle = WAngled::MakeFromRadian(fAngle) * 0.5;
  double s = WMath::Sin(halfAngle);
  double c = WMath::Cos(halfAngle);

  WSimdQuatd res;
  res.m_v = vRotationAxis * s;
  res.m_v.SetW(c);
  return res;
}

W_ALWAYS_INLINE void WSimdQuatd::Normalize()
{
  m_v.Normalize<4>();
}

inline WResult WSimdQuatd::GetRotationAxisAndAngle(WSimdVec4d& ref_vAxis, WSimdDouble& ref_fAngle, const WSimdDouble& fEpsilon) const
{
  ///\todo optimize
  const WAngleTemplate<double> acos = WMath::ACos<double>(m_v.w().Max(-1).Min(1));
  const double d = WMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = (acos * double(2)).GetRadian();

  return W_SUCCESS;
}

W_ALWAYS_INLINE WSimdMat4d WSimdQuatd::GetAsMat4() const
{
  const WSimdVec4d xyz = m_v;
  const WSimdVec4d x2y2z2 = xyz + xyz;
  const WSimdVec4d xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const WSimdVec4d yy2_xx2_xx2 = xx2yy2zz2.Get<WSwizzle::YXXX>();
  const WSimdVec4d zz2_zz2_yy2 = xx2yy2zz2.Get<WSwizzle::ZZYX>();
  WSimdVec4d diagonal = WSimdVec4d(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(WSimdDouble::MakeZero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const WSimdVec4d x_y_x = xyz.Get<WSwizzle::XYXX>();
  const WSimdVec4d y2_z2_z2 = x2y2z2.Get<WSwizzle::YZZX>();
  const WSimdVec4d base = x_y_x.CompMul(y2_z2_z2);

  const WSimdVec4d z2_x2_y2 = x2y2z2.Get<WSwizzle::ZXYX>();
  const WSimdVec4d offset = z2_x2_y2 * m_v.w();

  const WSimdVec4d adds = base + offset;
  const WSimdVec4d subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const WSimdVec4d addX_u_diaX_u = adds.GetCombined<WSwizzle::XXXX>(diagonal);
  const WSimdVec4d subZ_u_diaW_u = subs.GetCombined<WSwizzle::ZXWX>(diagonal);
  const WSimdVec4d col0 = addX_u_diaX_u.GetCombined<WSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const WSimdVec4d subX_u_diaY_u = subs.GetCombined<WSwizzle::XXYX>(diagonal);
  const WSimdVec4d addY_u_diaW_u = adds.GetCombined<WSwizzle::YXWX>(diagonal);
  const WSimdVec4d col1 = subX_u_diaY_u.GetCombined<WSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const WSimdVec4d addZ_u_subY_u = adds.GetCombined<WSwizzle::ZXYX>(subs);
  const WSimdVec4d col2 = addZ_u_subY_u.GetCombined<WSwizzle::XZZW>(diagonal);

  return WSimdMat4d::MakeFromColumns(col0, col1, col2, WSimdVec4d(0.0, 0.0, 0.0, 1.0));
}

W_ALWAYS_INLINE bool WSimdQuatd::IsValid(const WSimdDouble& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

W_ALWAYS_INLINE bool WSimdQuatd::IsNaN() const
{
  return m_v.IsNaN<4>();
}

W_ALWAYS_INLINE WSimdQuatd WSimdQuatd::operator-() const
{
  return WSimdQuatd(m_v.FlipSign(WSimdVec4bWide(true, true, true, false)));
}

W_ALWAYS_INLINE WSimdVec4d WSimdQuatd::operator*(const WSimdVec4d& v) const
{
  WSimdVec4d t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

W_ALWAYS_INLINE WSimdQuatd WSimdQuatd::operator*(const WSimdQuatd& q2) const
{
  WSimdQuatd q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

W_ALWAYS_INLINE bool WSimdQuatd::operator==(const WSimdQuatd& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

W_ALWAYS_INLINE bool WSimdQuatd::operator!=(const WSimdQuatd& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
