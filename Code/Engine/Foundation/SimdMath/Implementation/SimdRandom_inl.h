#pragma once

// static
W_FORCE_INLINE WSimdVec4u WSimdRandom::UInt(const WSimdVec4i& vPosition, const WSimdVec4u& vSeed /*= WSimdVec4u::MakeZero()*/)
{
  // Based on Squirrel3 which was introduced by Squirrel Eiserloh at 'Math for Game Programmers: Noise-Based RNG', GDC17.
  const WSimdVec4u BIT_NOISE1 = WSimdVec4u(0xb5297a4d);
  const WSimdVec4u BIT_NOISE2 = WSimdVec4u(0x68e31da4);
  const WSimdVec4u BIT_NOISE3 = WSimdVec4u(0x1b56c4e9);

  WSimdVec4u mangled = WSimdVec4u(vPosition);
  mangled = mangled.CompMul(BIT_NOISE1);
  mangled += vSeed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled = mangled.CompMul(BIT_NOISE3);
  mangled ^= (mangled >> 8);

  return mangled;
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdRandom::FloatZeroToOne(const WSimdVec4i& vPosition, const WSimdVec4u& vSeed /*= WSimdVec4u::MakeZero()*/)
{
  return UInt(vPosition, vSeed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdRandom::FloatMinMax(const WSimdVec4i& vPosition, const WSimdVec4f& vMinValue, const WSimdVec4f& vMaxValue, const WSimdVec4u& vSeed /*= WSimdVec4u::MakeZero()*/)
{
  return WSimdVec4f::Lerp(vMinValue, vMaxValue, FloatZeroToOne(vPosition, vSeed));
}
