#pragma once

W_ALWAYS_INLINE WSimdMat4d::WSimdMat4d() = default;

inline WSimdMat4d WSimdMat4d::MakeFromValues(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3,
  double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4)
{
  WSimdMat4d res;
  res.m_col0.Set(f1r1, f1r2, f1r3, f1r4);
  res.m_col1.Set(f2r1, f2r2, f2r3, f2r4);
  res.m_col2.Set(f3r1, f3r2, f3r3, f3r4);
  res.m_col3.Set(f4r1, f4r2, f4r3, f4r4);
  return res;
}

inline WSimdMat4d WSimdMat4d::MakeFromColumns(const WSimdVec4d& vCol0, const WSimdVec4d& vCol1, const WSimdVec4d& vCol2, const WSimdVec4d& vCol3)
{
  WSimdMat4d res;
  res.m_col0 = vCol0;
  res.m_col1 = vCol1;
  res.m_col2 = vCol2;
  res.m_col3 = vCol3;
  return res;
}

inline WSimdMat4d WSimdMat4d::MakeFromRowMajorArray(const double* const pData)
{
  WSimdMat4d res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  res.Transpose();
  return res;
}

inline WSimdMat4d WSimdMat4d::MakeFromColumnMajorArray(const double* const pData)
{
  WSimdMat4d res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  return res;
}

inline void WSimdMat4d::GetAsArray(double* out_pData, WMatrixLayout::Enum layout) const
{
  WSimdMat4d tmp = *this;

  if (layout == WMatrixLayout::RowMajor)
  {
    tmp.Transpose();
  }

  tmp.m_col0.Store<4>(out_pData + 0);
  tmp.m_col1.Store<4>(out_pData + 4);
  tmp.m_col2.Store<4>(out_pData + 8);
  tmp.m_col3.Store<4>(out_pData + 12);
}

W_ALWAYS_INLINE WSimdMat4d WSimdMat4d::MakeZero()
{
  WSimdMat4d res;
  res.m_col0.SetZero();
  res.m_col1.SetZero();
  res.m_col2.SetZero();
  res.m_col3.SetZero();
  return res;
}

W_ALWAYS_INLINE WSimdMat4d WSimdMat4d::MakeIdentity()
{
  WSimdMat4d res;
  res.m_col0.Set(1, 0, 0, 0);
  res.m_col1.Set(0, 1, 0, 0);
  res.m_col2.Set(0, 0, 1, 0);
  res.m_col3.Set(0, 0, 0, 1);
  return res;
}

W_ALWAYS_INLINE WSimdMat4d WSimdMat4d::GetTranspose() const
{
  WSimdMat4d result = *this;
  result.Transpose();
  return result;
}

W_ALWAYS_INLINE WSimdMat4d WSimdMat4d::GetInverse(const WSimdDouble& fEpsilon) const
{
  WSimdMat4d result = *this;
  result.Invert(fEpsilon).IgnoreResult();
  return result;
}

inline bool WSimdMat4d::IsEqual(const WSimdMat4d& rhs, const WSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(rhs.m_col0, fEpsilon) && m_col1.IsEqual(rhs.m_col1, fEpsilon) && m_col2.IsEqual(rhs.m_col2, fEpsilon) &&
          m_col3.IsEqual(rhs.m_col3, fEpsilon))
    .AllSet<4>();
}

inline bool WSimdMat4d::IsIdentity(const WSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(WSimdVec4d(1, 0, 0, 0), fEpsilon) && m_col1.IsEqual(WSimdVec4d(0, 1, 0, 0), fEpsilon) &&
          m_col2.IsEqual(WSimdVec4d(0, 0, 1, 0), fEpsilon) && m_col3.IsEqual(WSimdVec4d(0, 0, 0, 1), fEpsilon))
    .AllSet<4>();
}

inline bool WSimdMat4d::IsValid() const
{
  return m_col0.IsValid<4>() && m_col1.IsValid<4>() && m_col2.IsValid<4>() && m_col3.IsValid<4>();
}

inline bool WSimdMat4d::IsNaN() const
{
  return m_col0.IsNaN<4>() || m_col1.IsNaN<4>() || m_col2.IsNaN<4>() || m_col3.IsNaN<4>();
}

W_ALWAYS_INLINE void WSimdMat4d::SetRows(const WSimdVec4d& vRow0, const WSimdVec4d& vRow1, const WSimdVec4d& vRow2, const WSimdVec4d& vRow3)
{
  m_col0 = vRow0;
  m_col1 = vRow1;
  m_col2 = vRow2;
  m_col3 = vRow3;

  Transpose();
}

W_ALWAYS_INLINE void WSimdMat4d::GetRows(WSimdVec4d& ref_vRow0, WSimdVec4d& ref_vRow1, WSimdVec4d& ref_vRow2, WSimdVec4d& ref_vRow3) const
{
  WSimdMat4d tmp = *this;
  tmp.Transpose();

  ref_vRow0 = tmp.m_col0;
  ref_vRow1 = tmp.m_col1;
  ref_vRow2 = tmp.m_col2;
  ref_vRow3 = tmp.m_col3;
}

W_ALWAYS_INLINE WSimdVec4d WSimdMat4d::TransformPosition(const WSimdVec4d& v) const
{
  WSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();
  result += m_col3;

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdMat4d::TransformDirection(const WSimdVec4d& v) const
{
  WSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();

  return result;
}

W_ALWAYS_INLINE WSimdMat4d WSimdMat4d::operator*(const WSimdMat4d& rhs) const
{
  WSimdMat4d result;

  result.m_col0 = m_col0 * rhs.m_col0.x();
  result.m_col0 += m_col1 * rhs.m_col0.y();
  result.m_col0 += m_col2 * rhs.m_col0.z();
  result.m_col0 += m_col3 * rhs.m_col0.w();

  result.m_col1 = m_col0 * rhs.m_col1.x();
  result.m_col1 += m_col1 * rhs.m_col1.y();
  result.m_col1 += m_col2 * rhs.m_col1.z();
  result.m_col1 += m_col3 * rhs.m_col1.w();

  result.m_col2 = m_col0 * rhs.m_col2.x();
  result.m_col2 += m_col1 * rhs.m_col2.y();
  result.m_col2 += m_col2 * rhs.m_col2.z();
  result.m_col2 += m_col3 * rhs.m_col2.w();

  result.m_col3 = m_col0 * rhs.m_col3.x();
  result.m_col3 += m_col1 * rhs.m_col3.y();
  result.m_col3 += m_col2 * rhs.m_col3.z();
  result.m_col3 += m_col3 * rhs.m_col3.w();

  return result;
}

W_ALWAYS_INLINE void WSimdMat4d::operator*=(const WSimdMat4d& rhs)
{
  *this = *this * rhs;
}

W_ALWAYS_INLINE bool WSimdMat4d::operator==(const WSimdMat4d& other) const
{
  return (m_col0 == other.m_col0 && m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3).AllSet<4>();
}

W_ALWAYS_INLINE bool WSimdMat4d::operator!=(const WSimdMat4d& other) const
{
  return !(*this == other);
}

W_ALWAYS_INLINE WSimdMat4d MultiplyAffine(const WSimdMat4d& lhs, const WSimdMat4d& rhs)
{
  WSimdMat4d result;

  result.m_col0 = lhs.m_col0 * rhs.m_col0.x();
  result.m_col0 += lhs.m_col1 * rhs.m_col0.y();
  result.m_col0 += lhs.m_col2 * rhs.m_col0.z();

  result.m_col1 = lhs.m_col0 * rhs.m_col1.x();
  result.m_col1 += lhs.m_col1 * rhs.m_col1.y();
  result.m_col1 += lhs.m_col2 * rhs.m_col1.z();

  result.m_col2 = lhs.m_col0 * rhs.m_col2.x();
  result.m_col2 += lhs.m_col1 * rhs.m_col2.y();
  result.m_col2 += lhs.m_col2 * rhs.m_col2.z();

  result.m_col3 = lhs.m_col0 * rhs.m_col3.x();
  result.m_col3 += lhs.m_col1 * rhs.m_col3.y();
  result.m_col3 += lhs.m_col2 * rhs.m_col3.z();
  result.m_col3 += lhs.m_col3;

  return result;
}
