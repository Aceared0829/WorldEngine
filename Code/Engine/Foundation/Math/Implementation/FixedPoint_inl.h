#pragma once

#include <Foundation/Math/Math.h>

template <WUInt8 DecimalBits>
const WFixedPoint<DecimalBits>& WFixedPoint<DecimalBits>::operator=(WInt32 iVal)
{
  m_iValue = iVal << DecimalBits;
  return *this;
}

template <WUInt8 DecimalBits>
const WFixedPoint<DecimalBits>& WFixedPoint<DecimalBits>::operator=(float fVal)
{
  m_iValue = (WInt32)WMath::Round(fVal * (1 << DecimalBits));
  return *this;
}

template <WUInt8 DecimalBits>
const WFixedPoint<DecimalBits>& WFixedPoint<DecimalBits>::operator=(double fVal)
{
  m_iValue = (WInt32)WMath::Round(fVal * (1 << DecimalBits));
  return *this;
}

template <WUInt8 DecimalBits>
WInt32 WFixedPoint<DecimalBits>::ToInt() const
{
  return (WInt32)(m_iValue >> DecimalBits);
}

template <WUInt8 DecimalBits>
float WFixedPoint<DecimalBits>::ToFloat() const
{
  return (float)((double)m_iValue / (double)(1 << DecimalBits));
}

template <WUInt8 DecimalBits>
double WFixedPoint<DecimalBits>::ToDouble() const
{
  return ((double)m_iValue / (double)(1 << DecimalBits));
}

template <WUInt8 DecimalBits>
void WFixedPoint<DecimalBits>::operator*=(const WFixedPoint<DecimalBits>& rhs)
{
  // lhs and rhs are in N:M format (N Bits for the Integer part, M Bits for the fractional part)
  // after multiplication, it will be in 2N:2M format

  const WInt64 TempLHS = m_iValue;
  const WInt64 TempRHS = rhs.m_iValue;

  WInt64 TempRes = TempLHS * TempRHS;

  // the lower DecimalBits Bits are nearly of no concern (we throw them away anyway), except for the upper most Bit
  // that is Bit '(DecimalBits - 1)' and its Bitmask is therefore '(1 << (DecimalBits - 1))'
  // If that Bit is set, then the lowest DecimalBits represent a value of more than '0.5' (of their range)
  // so '(TempRes & (1 << (DecimalBits - 1))) ' is either 0 or 1 depending on whether the lower DecimalBits Bits represent a value larger than 0.5 or
  // not we shift that Bit one to the left and add it to the original value and thus 'round up' the result
  TempRes += ((TempRes & (1 << (DecimalBits - 1))) << 1);

  TempRes >>= DecimalBits; // result format: 2N:M

  // the upper N Bits are thrown away during conversion from 64 Bit to 32 Bit
  m_iValue = (WInt32)TempRes;
}

template <WUInt8 DecimalBits>
void WFixedPoint<DecimalBits>::operator/=(const WFixedPoint<DecimalBits>& rhs)
{
  WInt64 TempLHS = m_iValue;
  const WInt64 TempRHS = rhs.m_iValue;

  TempLHS <<= 31;

  WInt64 TempRes = TempLHS / TempRHS;

  // same rounding concept as in multiplication
  TempRes += ((TempRes & (1 << (31 - DecimalBits - 1))) << 1);

  TempRes >>= (31 - DecimalBits);

  // here we throw away the upper 32 Bits again (not needed anymore)
  m_iValue = (WInt32)TempRes;
}


template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator+(const WFixedPoint<DecimalBits>& lhs, const WFixedPoint<DecimalBits>& rhs)
{
  WFixedPoint<DecimalBits> res = lhs;
  res += rhs;
  return res;
}

template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator-(const WFixedPoint<DecimalBits>& lhs, const WFixedPoint<DecimalBits>& rhs)
{
  WFixedPoint<DecimalBits> res = lhs;
  res -= rhs;
  return res;
}

template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator*(const WFixedPoint<DecimalBits>& lhs, const WFixedPoint<DecimalBits>& rhs)
{
  WFixedPoint<DecimalBits> res = lhs;
  res *= rhs;
  return res;
}

template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator/(const WFixedPoint<DecimalBits>& lhs, const WFixedPoint<DecimalBits>& rhs)
{
  WFixedPoint<DecimalBits> res = lhs;
  res /= rhs;
  return res;
}


template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator*(const WFixedPoint<DecimalBits>& lhs, WInt32 rhs)
{
  WFixedPoint<DecimalBits> ret = lhs;
  ret *= rhs;
  return ret;
}

template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator*(WInt32 lhs, const WFixedPoint<DecimalBits>& rhs)
{
  WFixedPoint<DecimalBits> ret = rhs;
  ret *= lhs;
  return ret;
}

template <WUInt8 DecimalBits>
WFixedPoint<DecimalBits> operator/(const WFixedPoint<DecimalBits>& lhs, WInt32 rhs)
{
  WFixedPoint<DecimalBits> ret = lhs;
  ret /= rhs;
  return ret;
}
