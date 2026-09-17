#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

WSimdQuatd WSimdQuatd::MakeShortestRotation(const WSimdVec4d& vDirFrom, const WSimdVec4d& vDirTo)
{
  const WSimdVec4d v0 = vDirFrom.GetNormalized<3>();
  const WSimdVec4d v1 = vDirTo.GetNormalized<3>();

  const WSimdDouble fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return WSimdQuatd::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return WSimdQuatd::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), WAngle::MakeFromRadian(WMath::Pi<float>()));
  }

  const WSimdVec4d c = v0.CrossRH(v1);
  const WSimdDouble s = ((fDot + WSimdDouble(1.0f)) * WSimdDouble(2.0f)).GetSqrt();

  WSimdQuatd res;
  res.m_v = c / s;
  res.m_v.SetW(s * WSimdDouble(0.5f));
  res.Normalize();
  return res;
}

WSimdQuatd WSimdQuatd::MakeSlerp(const WSimdQuatd& qFrom, const WSimdQuatd& qTo, const WSimdDouble& t)
{
  W_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const WSimdDouble one = 1.0f;
  const WSimdDouble qdelta = 1.0f - 0.001f;

  const WSimdDouble fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  WSimdDouble cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  WSimdDouble t0, t1;

  if (cosTheta < qdelta)
  {
    WAngle theta = WMath::ACos(float(cosTheta));

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const WSimdDouble iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const WAngle tTheta = (float)t * theta;

    WSimdDouble s0 = WMath::Sin(theta - tTheta);
    WSimdDouble s1 = WMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  WSimdQuatd res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool WSimdQuatd::IsEqualRotation(const WSimdQuatd& qOther, const WSimdDouble& fEpsilon) const
{
  WSimdVec4d vA1, vA2;
  WSimdDouble fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == W_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == W_FAILURE)
    return false;

  WAngle A1 = WAngle::MakeFromRadian(float(fA1));
  WAngle A2 = WAngle::MakeFromRadian(float(fA2));

  if ((A1.IsEqualSimple(A2, WAngle::MakeFromDegree(float(fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, WAngle::MakeFromDegree(float(fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}
