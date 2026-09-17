#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdNoise.h>

WSimdPerlinNoise::WSimdPerlinNoise()
{
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_Permutations); ++i)
  {
    m_Permutations[i] = static_cast<WUInt8>(i);
  }
}

WSimdPerlinNoise::WSimdPerlinNoise(WUInt32 uiSeed)
  : WSimdPerlinNoise()
{
  WRandom rnd;
  rnd.Initialize(uiSeed);

  Initialize(rnd);
}

void WSimdPerlinNoise::Initialize(WRandom& ref_rng)
{
  for (WUInt32 i = W_ARRAY_SIZE(m_Permutations) - 1; i > 0; --i)
  {
    WUInt32 uiRandomIndex = ref_rng.UIntInRange(W_ARRAY_SIZE(m_Permutations));
    WMath::Swap(m_Permutations[i], m_Permutations[uiRandomIndex]);
  }
}

WSimdVec4f WSimdPerlinNoise::NoiseZeroToOne(const WSimdVec4f& vX, const WSimdVec4f& vY, const WSimdVec4f& vZ, WUInt32 uiNumOctaves /*= 1*/)
{
  WSimdVec4f result = WSimdVec4f::MakeZero();
  WSimdFloat amplitude = 1.0f;
  WUInt32 uiOffset = 0;

  uiNumOctaves = WMath::Max(uiNumOctaves, 1u);
  for (WUInt32 i = 0; i < uiNumOctaves; ++i)
  {
    WSimdFloat scale = static_cast<float>(W_BIT(i));
    WSimdVec4f offset = Permute(WSimdVec4i(uiOffset) + WSimdVec4i(0, 1, 2, 3)).ToFloat();
    WSimdVec4f x = vX * scale + offset.Get<WSwizzle::XXXX>();
    WSimdVec4f y = vY * scale + offset.Get<WSwizzle::YYYY>();
    WSimdVec4f z = vZ * scale + offset.Get<WSwizzle::ZZZZ>();

    result += Noise(x, y, z) * amplitude;

    amplitude *= 0.5f;
    uiOffset += 23;
  }

  return result * 0.5f + WSimdVec4f(0.5f);
}

namespace
{
  W_FORCE_INLINE WSimdVec4f Fade(const WSimdVec4f& t)
  {
    return t.CompMul(t).CompMul(t).CompMul(t.CompMul(t * 6.0f - WSimdVec4f(15.0f)) + WSimdVec4f(10.0f));
  }

  W_FORCE_INLINE WSimdVec4f Grad(WSimdVec4i vHash, const WSimdVec4f& x, const WSimdVec4f& y, const WSimdVec4f& z)
  {
    // convert low 4 bits of hash code into 12 gradient directions.
    const WSimdVec4i h = vHash & WSimdVec4i(15);
    const WSimdVec4f u = WSimdVec4f::Select(h < WSimdVec4i(8), x, y);
    const WSimdVec4f v = WSimdVec4f::Select(h < WSimdVec4i(4), y, WSimdVec4f::Select(h == WSimdVec4i(12) || h == WSimdVec4i(14), x, z));
    return WSimdVec4f::Select((h & WSimdVec4i(1)) == WSimdVec4i::MakeZero(), u, -u) +
           WSimdVec4f::Select((h & WSimdVec4i(2)) == WSimdVec4i::MakeZero(), v, -v);
  }

  W_ALWAYS_INLINE WSimdVec4f Lerp(const WSimdVec4f& t, const WSimdVec4f& a, const WSimdVec4f& b)
  {
    return WSimdVec4f::Lerp(a, b, t);
  }

} // namespace

// reference: https://mrl.nyu.edu/~perlin/noise/
WSimdVec4f WSimdPerlinNoise::Noise(const WSimdVec4f& inX, const WSimdVec4f& inY, const WSimdVec4f& inZ)
{
  WSimdVec4f x = inX;
  WSimdVec4f y = inY;
  WSimdVec4f z = inZ;

  // find unit cube that contains point.
  const WSimdVec4f xFloored = x.Floor();
  const WSimdVec4f yFloored = y.Floor();
  const WSimdVec4f zFloored = z.Floor();

  const WSimdVec4i maxIndex = WSimdVec4i(255);
  const WSimdVec4i X = WSimdVec4i::Truncate(xFloored) & maxIndex;
  const WSimdVec4i Y = WSimdVec4i::Truncate(yFloored) & maxIndex;
  const WSimdVec4i Z = WSimdVec4i::Truncate(zFloored) & maxIndex;

  // find relative x,y,z of point in cube.
  x -= xFloored;
  y -= yFloored;
  z -= zFloored;

  // compute fade curves for each of x,y,z.
  const WSimdVec4f u = Fade(x);
  const WSimdVec4f v = Fade(y);
  const WSimdVec4f w = Fade(z);

  // hash coordinates of the 8 cube corners
  const WSimdVec4i i1 = WSimdVec4i(1);
  const WSimdVec4i A = Permute(X) + Y;
  const WSimdVec4i AA = Permute(A) + Z;
  const WSimdVec4i AB = Permute(A + i1) + Z;
  const WSimdVec4i B = Permute(X + i1) + Y;
  const WSimdVec4i BA = Permute(B) + Z;
  const WSimdVec4i BB = Permute(B + i1) + Z;

  const WSimdVec4f f1 = WSimdVec4f(1.0f);

  // and add blended results from 8 corners of cube.
  const WSimdVec4f c000 = Grad(Permute(AA), x, y, z);
  const WSimdVec4f c100 = Grad(Permute(BA), x - f1, y, z);
  const WSimdVec4f c010 = Grad(Permute(AB), x, y - f1, z);
  const WSimdVec4f c110 = Grad(Permute(BB), x - f1, y - f1, z);
  const WSimdVec4f c001 = Grad(Permute(AA + i1), x, y, z - f1);
  const WSimdVec4f c101 = Grad(Permute(BA + i1), x - f1, y, z - f1);
  const WSimdVec4f c011 = Grad(Permute(AB + i1), x, y - f1, z - f1);
  const WSimdVec4f c111 = Grad(Permute(BB + i1), x - f1, y - f1, z - f1);

  const WSimdVec4f c000_c100 = Lerp(u, c000, c100);
  const WSimdVec4f c010_c110 = Lerp(u, c010, c110);
  const WSimdVec4f c001_c101 = Lerp(u, c001, c101);
  const WSimdVec4f c011_c111 = Lerp(u, c011, c111);

  return Lerp(w, Lerp(v, c000_c100, c010_c110), Lerp(v, c001_c101, c011_c111));
}
