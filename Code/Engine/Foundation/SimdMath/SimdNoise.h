#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

class W_FOUNDATION_DLL WSimdPerlinNoise
{
public:
  WSimdPerlinNoise();
  WSimdPerlinNoise(WUInt32 uiSeed);

  void Initialize(WRandom& ref_rng);

  WSimdVec4f NoiseZeroToOne(const WSimdVec4f& x, const WSimdVec4f& y, const WSimdVec4f& z, WUInt32 uiNumOctaves = 1);

private:
  WSimdVec4f Noise(const WSimdVec4f& x, const WSimdVec4f& y, const WSimdVec4f& z);

  W_FORCE_INLINE WSimdVec4i Permute(const WSimdVec4i& v)
  {
#if 0
    WArrayPtr<WUInt8> p = WMakeArrayPtr(m_Permutations);
#else
    WUInt8* p = m_Permutations;
#endif

    WSimdVec4i i = v & WSimdVec4i(W_ARRAY_SIZE(m_Permutations) - 1);
    return WSimdVec4i(p[i.x()], p[i.y()], p[i.z()], p[i.w()]);
  }

  WUInt8 m_Permutations[256];
};
