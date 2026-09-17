#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

/// Noise based random number generator that generates 4 pseudo random values at once.
///
/// Does not keep any internal state but relies on the user to provide different positions for each call.
/// The seed parameter can be used to further alter the noise function.
struct WSimdRandom
{
  /// Returns 4 random uint32 values at position, ie. ranging from 0 to (2 ^ 32) - 1
  static WSimdVec4u UInt(const WSimdVec4i& vPosition, const WSimdVec4u& vSeed = WSimdVec4u::MakeZero());

  /// Returns 4 random float values in range [0.0 ; 1.0], ie. including zero and one
  static WSimdVec4f FloatZeroToOne(const WSimdVec4i& vPosition, const WSimdVec4u& vSeed = WSimdVec4u::MakeZero());

  /// Returns 4 random float values in range [fMinValue ; fMaxValue]
  static WSimdVec4f FloatMinMax(const WSimdVec4i& vPosition, const WSimdVec4f& vMinValue, const WSimdVec4f& vMaxValue, const WSimdVec4u& vSeed = WSimdVec4u::MakeZero());
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>
