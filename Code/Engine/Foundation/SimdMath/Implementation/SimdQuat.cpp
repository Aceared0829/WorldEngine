#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

WSimdQuat WSimdQuat::MakeShortestRotation(const WSimdVec4f& vDirFrom, const WSimdVec4f& vDirTo)
{
  const WSimdVec4f v0 = vDirFrom.GetNormalized<3>();
  const WSimdVec4f v1 = vDirTo.GetNormalized<3>();

  const WSimdFloat fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return WSimdQuat::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return WSimdQuat::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), WAngle::MakeFromRadian(WMath::Pi<float>()));
  }

  const WSimdVec4f c = v0.CrossRH(v1);
  const WSimdFloat s = ((fDot + WSimdFloat(1.0f)) * WSimdFloat(2.0f)).GetSqrt();

  WSimdQuat res;
  res.m_v = c / s;
  res.m_v.SetW(s * WSimdFloat(0.5f));
  res.Normalize();
  return res;
}

WSimdQuat WSimdQuat::MakeSlerp(const WSimdQuat& qFrom, const WSimdQuat& qTo, const WSimdFloat& t)
{
  W_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const WSimdFloat one = 1.0f;
  const WSimdFloat qdelta = 1.0f - 0.001f;

  const WSimdFloat fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  WSimdFloat cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  WSimdFloat t0, t1;

  if (cosTheta < qdelta)
  {
    WAngle theta = WMath::ACos(float(cosTheta));

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const WSimdFloat iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const WAngle tTheta = (float)t * theta;

    WSimdFloat s0 = WMath::Sin(theta - tTheta);
    WSimdFloat s1 = WMath::Sin(tTheta);

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

  WSimdQuat res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool WSimdQuat::IsEqualRotation(const WSimdQuat& qOther, const WSimdFloat& fEpsilon) const
{
  WSimdVec4f vA1, vA2;
  WSimdFloat fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == W_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == W_FAILURE)
    return false;

  WAngle A1 = WAngle::MakeFromRadian(fA1);
  WAngle A2 = WAngle::MakeFromRadian(fA2);

  if ((A1.IsEqualSimple(A2, WAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, WAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}
