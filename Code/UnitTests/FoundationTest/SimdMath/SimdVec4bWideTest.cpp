#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4b_Wide.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdVec4bWide)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_DISABLED(W_COMPILER_GCC) && W_DISABLED(W_COMPILE_FOR_DEBUG)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) double testBlock[4] = {1, 2, 3, 4};
    WSimdVec4bWide* pDefCtor = ::new ((void*)&testBlock[0]) WSimdVec4bWide;
    W_TEST_BOOL(testBlock[0] == 1.0 && testBlock[1] == 2.0 && testBlock[2] == 3.0 && testBlock[3] == 4.0);
#endif

    // Make sure the class didn't accidentally change in size.
#if W_SSE_LEVEL >= W_SSE_AVX
    static_assert(sizeof(WSimdVec4bWide) == 32);
    static_assert(alignof(WSimdVec4bWide) == 32);
#else

    static_assert(sizeof(WSimdVec4bWide) == 32);

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    static_assert(alignof(WSimdVec4bWide) == 16);
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
    static_assert(alignof(WSimdVec4bWide) == 8);
#endif

    #endif

    WSimdVec4bWide vInit1B(true);
    W_TEST_BOOL(vInit1B.x() == true && vInit1B.y() == true && vInit1B.z() == true && vInit1B.w() == true);

    // Make sure all components have the correct value
#if W_SSE_LEVEL >= W_SSE_AVX
    alignas(32) WUInt64 vals[4];
    _mm256_store_si256((__m256i*)vals, _mm256_castpd_si256(vInit1B.m_v));

    W_TEST_BOOL(vals[0] == 0xFFFFFFFFFFFFFFFFULL
      && vals[1] == 0xFFFFFFFFFFFFFFFFULL
      && vals[2] == 0xFFFFFFFFFFFFFFFFULL
      && vals[3] == 0xFFFFFFFFFFFFFFFFULL);
#else

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    alignas(16) WUInt64 valsXY[2];
    alignas(16) WUInt64 valsZW[2];
    _mm_store_si128((__m128i*)valsXY, _mm_castpd_si128(vInit1B.m_v.xy));
    _mm_store_si128((__m128i*)valsZW, _mm_castpd_si128(vInit1B.m_v.zw));

    W_TEST_BOOL(valsXY[0] == 0xFFFFFFFFFFFFFFFFULL
      && valsXY[1] == 0xFFFFFFFFFFFFFFFFULL
      && valsZW[0] == 0xFFFFFFFFFFFFFFFFULL
      && valsZW[1] == 0xFFFFFFFFFFFFFFFFULL);
#endif

#endif

    WSimdVec4bWide vInit4B(false, true, false, true);
    W_TEST_BOOL(vInit4B.x() == false && vInit4B.y() == true && vInit4B.z() == false && vInit4B.w() == true);

    // Make sure all components have the correct value
#if W_SSE_LEVEL >= W_SSE_AVX
    _mm256_store_si256((__m256i*)vals, _mm256_castpd_si256(vInit4B.m_v));
    W_TEST_BOOL(vals[0] == 0
      && vals[1] == 0xFFFFFFFFFFFFFFFFULL
      && vals[2] == 0
      && vals[3] == 0xFFFFFFFFFFFFFFFFULL);
#else

#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    _mm_store_si128((__m128i*)valsXY, _mm_castpd_si128(vInit4B.m_v.xy));
    _mm_store_si128((__m128i*)valsZW, _mm_castpd_si128(vInit4B.m_v.zw));
    
    W_TEST_BOOL(valsXY[0] == 0
      && valsXY[1] == 0xFFFFFFFFFFFFFFFFULL
      && valsZW[0] == 0
      && valsZW[1] == 0xFFFFFFFFFFFFFFFFULL);
#endif
#endif

    WSimdVec4bWide vCopy(vInit4B);
    W_TEST_BOOL(vCopy.x() == false && vCopy.y() == true && vCopy.z() == false && vCopy.w() == true);

    W_TEST_BOOL(
      vCopy.GetComponent<0>() == false && vCopy.GetComponent<1>() == true && vCopy.GetComponent<2>() == false && vCopy.GetComponent<3>() == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swizzle")
  {
    WSimdVec4bWide a(true, false, true, false);

    WSimdVec4bWide b = a.Get<WSwizzle::XXXX>();
    W_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<WSwizzle::YYYX>();
    W_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());

    b = a.Get<WSwizzle::ZZZX>();
    W_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<WSwizzle::WWWX>();
    W_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    WSimdVec4bWide a(true, false, true, false);
    WSimdVec4bWide b(false, true, true, false);

    WSimdVec4bWide c = a && b;
    W_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());

    c = a || b;
    W_TEST_BOOL(c.x() && c.y() && c.z() && !c.w());

    c = !a;
    W_TEST_BOOL(!c.x() && c.y() && !c.z() && c.w());
    W_TEST_BOOL(c.AnySet<2>());
    W_TEST_BOOL(!c.AllSet<4>());
    W_TEST_BOOL(!c.NoneSet<4>());

    c = c || a;
    W_TEST_BOOL(c.AnySet<4>());
    W_TEST_BOOL(c.AllSet<4>());
    W_TEST_BOOL(!c.NoneSet<4>());

    c = !c;
    W_TEST_BOOL(!c.AnySet<4>());
    W_TEST_BOOL(!c.AllSet<4>());
    W_TEST_BOOL(c.NoneSet<4>());

    c = a == b;
    W_TEST_BOOL(!c.x() && !c.y() && c.z() && c.w());

    c = a != b;
    W_TEST_BOOL(c.x() && c.y() && !c.z() && !c.w());

    W_TEST_BOOL(a.AllSet<1>());
    W_TEST_BOOL(b.NoneSet<1>());

    WSimdVec4bWide cmp(false, true, false, true);
    c = WSimdVec4bWide::Select(cmp, a, b);
    W_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());
  }
}
