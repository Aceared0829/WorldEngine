#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdFloat.h>

W_CREATE_SIMPLE_TEST_GROUP(SimdMath);

W_CREATE_SIMPLE_TEST(SimdMath, SimdFloat)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    WSimdFloat vDefCtor;
    W_TEST_BOOL(WMath::IsNaN((float)vDefCtor));
#else
// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if W_DISABLED(W_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    WSimdFloat* pDefCtor = ::new ((void*)&testBlock[0]) WSimdFloat;
    W_TEST_BOOL_MSG((float)(*pDefCtor) == 1.0f, "Default constructed value is %f", (float)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    static_assert(sizeof(WSimdFloat) == 16);
    static_assert(alignof(WSimdFloat) == 16);
#endif

    WSimdFloat vInit1F(2.0f);
    W_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if (W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE) && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(
      vInit1F.m_v.m128_f32[0] == 2.0f && vInit1F.m_v.m128_f32[1] == 2.0f && vInit1F.m_v.m128_f32[2] == 2.0f && vInit1F.m_v.m128_f32[3] == 2.0f);
#endif

    WSimdFloat vInit1I(1);
    W_TEST_BOOL(vInit1I == 1.0f);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(
      vInit1I.m_v.m128_f32[0] == 1.0f && vInit1I.m_v.m128_f32[1] == 1.0f && vInit1I.m_v.m128_f32[2] == 1.0f && vInit1I.m_v.m128_f32[3] == 1.0f);
#endif

    WSimdFloat vInit1U(4553u);
    W_TEST_BOOL(vInit1U == 4553.0f);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(vInit1U.m_v.m128_f32[0] == 4553.0f && vInit1U.m_v.m128_f32[1] == 4553.0f && vInit1U.m_v.m128_f32[2] == 4553.0f &&
                 vInit1U.m_v.m128_f32[3] == 4553.0f);
#endif

    WSimdFloat z = WSimdFloat::MakeZero();
    W_TEST_BOOL(z == 0.0f);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(z.m_v.m128_f32[0] == 0.0f && z.m_v.m128_f32[1] == 0.0f && z.m_v.m128_f32[2] == 0.0f && z.m_v.m128_f32[3] == 0.0f);
#endif
  }

  {
    WSimdFloat z = WSimdFloat::MakeNaN();

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m128_f32[0]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m128_f32[1]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m128_f32[2]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m128_f32[3]));
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    WSimdFloat a = 5.0f;
    WSimdFloat b = 2.0f;

    W_TEST_FLOAT(a + b, 7.0f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a - b, 3.0f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a * b, 10.0f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a / b, 2.5f, WMath::SmallEpsilon<float>());

    WSimdFloat c = 1.0f;
    c += a;
    W_TEST_FLOAT(c, 6.0f, WMath::SmallEpsilon<float>());

    c = 1.0f;
    c -= b;
    W_TEST_FLOAT(c, -1.0f, WMath::SmallEpsilon<float>());

    c = 1.0f;
    c *= a;
    W_TEST_FLOAT(c, 5.0f, WMath::SmallEpsilon<float>());

    c = 1.0f;
    c /= a;
    W_TEST_FLOAT(c, 0.2f, WMath::SmallEpsilon<float>());

    W_TEST_BOOL(c.IsEqual(0.201f, WMath::HugeEpsilon<float>()));
    W_TEST_BOOL(c.IsEqual(0.199f, WMath::HugeEpsilon<float>()));
    W_TEST_BOOL(!c.IsEqual(0.202f, WMath::HugeEpsilon<float>()));
    W_TEST_BOOL(!c.IsEqual(0.198f, WMath::HugeEpsilon<float>()));

    c = b;
    W_TEST_BOOL(c == b);
    W_TEST_BOOL(c != a);
    W_TEST_BOOL(a > b);
    W_TEST_BOOL(c >= b);
    W_TEST_BOOL(b < a);
    W_TEST_BOOL(b <= c);

    W_TEST_BOOL(c == 2.0f);
    W_TEST_BOOL(c != 5.0f);
    W_TEST_BOOL(a > 2.0f);
    W_TEST_BOOL(c >= 2.0f);
    W_TEST_BOOL(b < 5.0f);
    W_TEST_BOOL(b <= 2.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Misc")
  {
    WSimdFloat a = 2.0f;

    W_TEST_FLOAT(a.GetReciprocal(), 0.5f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetReciprocal<WMathAcc::FULL>(), 0.5f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetReciprocal<WMathAcc::BITS_23>(), 0.5f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(a.GetReciprocal<WMathAcc::BITS_12>(), 0.5f, WMath::HugeEpsilon<float>());

    W_TEST_FLOAT(a.GetSqrt(), 1.41421356f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetSqrt<WMathAcc::FULL>(), 1.41421356f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetSqrt<WMathAcc::BITS_23>(), 1.41421356f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(a.GetSqrt<WMathAcc::BITS_12>(), 1.41421356f, WMath::HugeEpsilon<float>());

    W_TEST_FLOAT(a.GetInvSqrt(), 0.70710678f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetInvSqrt<WMathAcc::FULL>(), 0.70710678f, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(a.GetInvSqrt<WMathAcc::BITS_23>(), 0.70710678f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(a.GetInvSqrt<WMathAcc::BITS_12>(), 0.70710678f, WMath::HugeEpsilon<float>());

    WSimdFloat b = 5.0f;
    W_TEST_BOOL(a.Max(b) == b);
    W_TEST_BOOL(a.Min(b) == a);

    WSimdFloat c = -4.0f;
    W_TEST_FLOAT(c.Abs(), 4.0f, WMath::SmallEpsilon<float>());
  }
}
