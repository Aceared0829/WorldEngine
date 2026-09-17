#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4b.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdVec4b)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_DISABLED(W_COMPILER_GCC) && W_DISABLED(W_COMPILE_FOR_DEBUG)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    WSimdVec4b* pDefCtor = ::new ((void*)&testBlock[0]) WSimdVec4b;
    W_TEST_BOOL(testBlock[0] == 1.0f && testBlock[1] == 2.0f && testBlock[2] == 3.0f && testBlock[3] == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    static_assert(sizeof(WSimdVec4b) == 16);
    static_assert(alignof(WSimdVec4b) == 16);
#endif

    WSimdVec4b vInit1B(true);
    W_TEST_BOOL(vInit1B.x() == true && vInit1B.y() == true && vInit1B.z() == true && vInit1B.w() == true);

    // Make sure all components have the correct value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(vInit1B.m_v.m128_u32[0] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[2] == 0xFFFFFFFF &&
                 vInit1B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    WSimdVec4b vInit4B(false, true, false, true);
    W_TEST_BOOL(vInit4B.x() == false && vInit4B.y() == true && vInit4B.z() == false && vInit4B.w() == true);

    // Make sure all components have the correct value
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
    W_TEST_BOOL(
      vInit4B.m_v.m128_u32[0] == 0 && vInit4B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit4B.m_v.m128_u32[2] == 0 && vInit4B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    WSimdVec4b vCopy(vInit4B);
    W_TEST_BOOL(vCopy.x() == false && vCopy.y() == true && vCopy.z() == false && vCopy.w() == true);

    W_TEST_BOOL(
      vCopy.GetComponent<0>() == false && vCopy.GetComponent<1>() == true && vCopy.GetComponent<2>() == false && vCopy.GetComponent<3>() == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swizzle")
  {
    WSimdVec4b a(true, false, true, false);

    WSimdVec4b b = a.Get<WSwizzle::XXXX>();
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
    WSimdVec4b a(true, false, true, false);
    WSimdVec4b b(false, true, true, false);

    WSimdVec4b c = a && b;
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

    WSimdVec4b cmp(false, true, false, true);
    c = WSimdVec4b::Select(cmp, a, b);
    W_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());
  }
}
