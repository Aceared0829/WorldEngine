#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomPointInSphere(WRandom& inout_rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = inout_rng.DoubleMinMax(-1, 1);
    py = inout_rng.DoubleMinMax(-1, 1);
    pz = inout_rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return WVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomDirection(WRandom& inout_rng)
{
  WVec3Template<Type> vec = MakeRandomPointInSphere(inout_rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomDeviationX(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation)
{
  const double twoPi = 2.0 * WMath::Pi<double>();

  const double cosAngle = WMath::Cos(maxDeviation);

  const double x = inout_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const WAngle phi = WAngle::MakeFromRadian((float)(inout_rng.DoubleZeroToOneInclusive() * twoPi));
  const double invSqrt = WMath::Sqrt(1 - (x * x));
  const double y = invSqrt * WMath::Cos(phi);
  const double z = invSqrt * WMath::Sin(phi);

  return WVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomDeviationY(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation)
{
  WVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  WMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomDeviationZ(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation)
{
  WVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  WMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeRandomDeviation(WRandom& inout_rng, const WAngleTemplate<Type>& maxDeviation, const WVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  WQuatTemplate<Type> qRotXtoDir = WQuatTemplate<Type>::MakeShortestRotation(WVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  WVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
