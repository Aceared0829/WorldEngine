#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdDouble.h>


W_CREATE_SIMPLE_TEST(SimdMath, SimdDouble)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    WSimdDouble vDefCtor;
    W_TEST_BOOL(WMath::IsNaN((double)vDefCtor));
#else

// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if W_DISABLED(W_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    WSimdDouble* pDefCtor = ::new ((void*)&testBlock[0]) WSimdDouble;
    W_TEST_BOOL_MSG((double)(*pDefCtor) == 1.0f, "Default constructed value is %lf", (double)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if (W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE) && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    static_assert(sizeof(WSimdDouble) == 32);
    static_assert(alignof(WSimdDouble) == 32);
#  else
    static_assert(sizeof(WSimdDouble) == 32);
    static_assert(alignof(WSimdDouble) == 16);
#  endif
#endif

    WSimdDouble vInit1F(2.0f);
    W_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if (W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE) && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    W_TEST_BOOL(vInit1F.m_v.m256d_f64[0] == 2.0 && vInit1F.m_v.m256d_f64[1] == 2.0 && vInit1F.m_v.m256d_f64[2] == 2.0 && vInit1F.m_v.m256d_f64[3] == 2.0);
#  else
    W_TEST_BOOL(vInit1F.m_v.xy.m128d_f64[0] == 2.0 && vInit1F.m_v.xy.m128d_f64[1] == 2.0 && vInit1F.m_v.zw.m128d_f64[0] == 2.0 && vInit1F.m_v.zw.m128d_f64[1] == 2.0);
#  endif
#endif

    WSimdDouble vInit1I(1);
    W_TEST_BOOL(vInit1I == 1.0);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    W_TEST_BOOL(vInit1I.m_v.m256d_f64[0] == 1.0 && vInit1I.m_v.m256d_f64[1] == 1.0 && vInit1I.m_v.m256d_f64[2] == 1.0 && vInit1I.m_v.m256d_f64[3] == 1.0);
#  else
    W_TEST_BOOL(vInit1I.m_v.xy.m128d_f64[0] == 1.0 && vInit1I.m_v.xy.m128d_f64[1] == 1.0 && vInit1I.m_v.zw.m128d_f64[0] == 1.0 && vInit1I.m_v.zw.m128d_f64[1] == 1.0);
#  endif
#endif

    WSimdDouble vInit1U(4553u);
    W_TEST_BOOL(vInit1U == 4553.0);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    W_TEST_BOOL(vInit1U.m_v.m256d_f64[0] == 4553.0 && vInit1U.m_v.m256d_f64[1] == 4553.0 && vInit1U.m_v.m256d_f64[2] == 4553.0 && vInit1U.m_v.m256d_f64[3] == 4553.0);
#  else
    W_TEST_BOOL(vInit1U.m_v.xy.m128d_f64[0] == 4553.0 && vInit1U.m_v.xy.m128d_f64[1] == 4553.0 && vInit1U.m_v.zw.m128d_f64[0] == 4553.0 && vInit1U.m_v.zw.m128d_f64[1] == 4553.0);
#  endif
#endif

    WSimdDouble z = WSimdDouble::MakeZero();
    W_TEST_BOOL(z == 0.0);

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    W_TEST_BOOL(z.m_v.m256d_f64[0] == 0.0 && z.m_v.m256d_f64[1] == 0.0 && z.m_v.m256d_f64[2] == 0.0 && z.m_v.m256d_f64[3] == 0.0);
#  else
    W_TEST_BOOL(z.m_v.xy.m128d_f64[0] == 0.0 && z.m_v.xy.m128d_f64[1] == 0.0 && z.m_v.zw.m128d_f64[0] == 0.0 && z.m_v.zw.m128d_f64[1] == 0.0);
#  endif
#endif
  }

  {
    WSimdDouble z = WSimdDouble::MakeNaN();

    // Make sure all components are set to the same value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if (W_SSE_LEVEL >= W_SSE_AVX)
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m256d_f64[0]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m256d_f64[1]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m256d_f64[2]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.m256d_f64[3]));
#  else
    W_TEST_BOOL(WMath::IsNaN(z.m_v.xy.m128d_f64[0]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.xy.m128d_f64[1]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.zw.m128d_f64[0]));
    W_TEST_BOOL(WMath::IsNaN(z.m_v.zw.m128d_f64[1]));
#  endif
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    WSimdDouble a = 5.0;
    WSimdDouble b = 2.0;

    W_TEST_DOUBLE(a + b, 7.0, WMath::SmallEpsilon<double>());
    W_TEST_DOUBLE(a - b, 3.0, WMath::SmallEpsilon<double>());
    W_TEST_DOUBLE(a * b, 10.0, WMath::SmallEpsilon<double>());
    W_TEST_DOUBLE(a / b, 2.5, WMath::SmallEpsilon<double>());

    WSimdDouble c = 1.0;
    c += a;
    W_TEST_DOUBLE(c, 6.0, WMath::SmallEpsilon<double>());

    c = 1.0;
    c -= b;
    W_TEST_DOUBLE(c, -1.0, WMath::SmallEpsilon<double>());

    c = 1.0;
    c *= a;
    W_TEST_DOUBLE(c, 5.0, WMath::SmallEpsilon<double>());

    c = 1.0;
    c /= a;
    W_TEST_DOUBLE(c, 0.2, WMath::SmallEpsilon<double>());

    W_TEST_BOOL(c.IsEqual(0.201, 0.001));
    W_TEST_BOOL(c.IsEqual(0.199, 0.001));
    W_TEST_BOOL(!c.IsEqual(0.202, 0.001));
    W_TEST_BOOL(!c.IsEqual(0.198, 0.001));

    c = b;
    W_TEST_BOOL(c == b);
    W_TEST_BOOL(c != a);
    W_TEST_BOOL(a > b);
    W_TEST_BOOL(c >= b);
    W_TEST_BOOL(b < a);
    W_TEST_BOOL(b <= c);

    W_TEST_BOOL(c == 2.0);
    W_TEST_BOOL(c != 5.0);
    W_TEST_BOOL(a > 2.0);
    W_TEST_BOOL(c >= 2.0);
    W_TEST_BOOL(b < 5.0);
    W_TEST_BOOL(b <= 2.0);

    // Test float overloads explicitly
    W_TEST_BOOL(c == 2.0f);
    W_TEST_BOOL(c != 5.0f);
    W_TEST_BOOL(a > 2.0f);
    W_TEST_BOOL(c >= 2.0f);
    W_TEST_BOOL(b < 5.0f);
    W_TEST_BOOL(b <= 2.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Misc")
  {
    WSimdDouble a = 2.0;

    W_TEST_DOUBLE(a.GetReciprocal(), 0.5, WMath::SmallEpsilon<double>());
    
    W_TEST_DOUBLE(a.GetSqrt(), 1.41421356237309504880, WMath::SmallEpsilon<double>());
    
    W_TEST_DOUBLE(a.GetInvSqrt(), 0.70710678118654752440, WMath::SmallEpsilon<double>());
    
    WSimdDouble b = 5.0;
    W_TEST_BOOL(a.Max(b) == b);
    W_TEST_BOOL(a.Min(b) == a);

    WSimdDouble c = -4.0;
    W_TEST_DOUBLE(c.Abs(), 4.0, WMath::SmallEpsilon<double>());
  }
}
