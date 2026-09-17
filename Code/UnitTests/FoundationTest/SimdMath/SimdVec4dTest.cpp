#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4d.h>

namespace
{
  static bool AllCompSame(const WSimdDouble& a)
  {
    // Make sure all components are the same
    WSimdVec4d test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }


  static void TestLength(const WSimdVec4d& a, double r[4], const WSimdDouble& fEps)
  {
    WSimdDouble l1 = a.GetLength<1>();
    WSimdDouble l2 = a.GetLength<2>();
    WSimdDouble l3 = a.GetLength<3>();
    WSimdDouble l4 = a.GetLength<4>();
    W_TEST_DOUBLE(l1, r[0], fEps);
    W_TEST_DOUBLE(l2, r[1], fEps);
    W_TEST_DOUBLE(l3, r[2], fEps);
    W_TEST_DOUBLE(l4, r[3], fEps);
    W_TEST_BOOL(AllCompSame(l1));
    W_TEST_BOOL(AllCompSame(l2));
    W_TEST_BOOL(AllCompSame(l3));
    W_TEST_BOOL(AllCompSame(l4));
  }


  static void TestInvLength(const WSimdVec4d& a, double r[4], const WSimdDouble& fEps)
  {
    WSimdDouble l1 = a.GetInvLength<1>();
    WSimdDouble l2 = a.GetInvLength<2>();
    WSimdDouble l3 = a.GetInvLength<3>();
    WSimdDouble l4 = a.GetInvLength<4>();
    W_TEST_DOUBLE(l1, r[0], fEps);
    W_TEST_DOUBLE(l2, r[1], fEps);
    W_TEST_DOUBLE(l3, r[2], fEps);
    W_TEST_DOUBLE(l4, r[3], fEps);
    W_TEST_BOOL(AllCompSame(l1));
    W_TEST_BOOL(AllCompSame(l2));
    W_TEST_BOOL(AllCompSame(l3));
    W_TEST_BOOL(AllCompSame(l4));
  }


  static void TestNormalize(const WSimdVec4d& a, WSimdVec4d n[4], WSimdDouble r[4], const WSimdDouble& fEps)
  {
    WSimdVec4d n1 = a.GetNormalized<1>();
    WSimdVec4d n2 = a.GetNormalized<2>();
    WSimdVec4d n3 = a.GetNormalized<3>();
    WSimdVec4d n4 = a.GetNormalized<4>();
    W_TEST_BOOL(n1.IsEqual(n[0], fEps).AllSet());
    W_TEST_BOOL(n2.IsEqual(n[1], fEps).AllSet());
    W_TEST_BOOL(n3.IsEqual(n[2], fEps).AllSet());
    W_TEST_BOOL(n4.IsEqual(n[3], fEps).AllSet());

    WSimdVec4d a1 = a;
    WSimdVec4d a2 = a;
    WSimdVec4d a3 = a;
    WSimdVec4d a4 = a;

    WSimdDouble l1 = a1.GetLengthAndNormalize<1>();
    WSimdDouble l2 = a2.GetLengthAndNormalize<2>();
    WSimdDouble l3 = a3.GetLengthAndNormalize<3>();
    WSimdDouble l4 = a4.GetLengthAndNormalize<4>();
    W_TEST_DOUBLE(l1, r[0], fEps);
    W_TEST_DOUBLE(l2, r[1], fEps);
    W_TEST_DOUBLE(l3, r[2], fEps);
    W_TEST_DOUBLE(l4, r[3], fEps);
    W_TEST_BOOL(AllCompSame(l1));
    W_TEST_BOOL(AllCompSame(l2));
    W_TEST_BOOL(AllCompSame(l3));
    W_TEST_BOOL(AllCompSame(l4));

    W_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    W_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    W_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    W_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    W_TEST_BOOL(a1.IsNormalized<1>(fEps));
    W_TEST_BOOL(a2.IsNormalized<2>(fEps));
    W_TEST_BOOL(a3.IsNormalized<3>(fEps));
    W_TEST_BOOL(a4.IsNormalized<4>(fEps));
    W_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    W_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    W_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    a1 = a;
    a1.Normalize<1>();
    a2 = a;
    a2.Normalize<2>();
    a3 = a;
    a3.Normalize<3>();
    a4 = a;
    a4.Normalize<4>();
    W_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    W_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    W_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    W_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());
  }


  static void TestNormalizeIfNotZero(const WSimdVec4d& a, WSimdVec4d n[4], const WSimdDouble& fEps)
  {
    WSimdVec4d a1 = a;
    a1.NormalizeIfNotZero<1>(fEps);
    WSimdVec4d a2 = a;
    a2.NormalizeIfNotZero<2>(fEps);
    WSimdVec4d a3 = a;
    a3.NormalizeIfNotZero<3>(fEps);
    WSimdVec4d a4 = a;
    a4.NormalizeIfNotZero<4>(fEps);
    W_TEST_BOOL(a1.IsEqual(n[0], fEps).AllSet());
    W_TEST_BOOL(a2.IsEqual(n[1], fEps).AllSet());
    W_TEST_BOOL(a3.IsEqual(n[2], fEps).AllSet());
    W_TEST_BOOL(a4.IsEqual(n[3], fEps).AllSet());

    W_TEST_BOOL(a1.IsNormalized<1>(fEps));
    W_TEST_BOOL(a2.IsNormalized<2>(fEps));
    W_TEST_BOOL(a3.IsNormalized<3>(fEps));
    W_TEST_BOOL(a4.IsNormalized<4>(fEps));
    W_TEST_BOOL(!a1.IsNormalized<2>(fEps));
    W_TEST_BOOL(!a2.IsNormalized<3>(fEps));
    W_TEST_BOOL(!a3.IsNormalized<4>(fEps));

    WSimdVec4d b(fEps);
    b.NormalizeIfNotZero<4>(fEps);
    W_TEST_BOOL(b.IsZero<4>());
  }

  static void TestNormalizeIfNotZeroWithFallback(const WSimdVec4d& a, const WSimdDouble& fEps)
  {

    WSimdVec4d vNorm = a;
    vNorm.Normalize<3>();


    WSimdVec4d vNormCond = vNorm * (fEps * WSimdDouble(0.1));
    vNormCond.NormalizeIfNotZero<3>(a, fEps);
    W_TEST_BOOL((vNormCond == a).AllSet());

    vNormCond = vNorm * fEps;
    vNormCond.NormalizeIfNotZero<3>(a, (fEps * WSimdDouble(0.1)));
    W_TEST_BOOL(vNormCond.IsEqual(vNorm, fEps).AllSet());
  }

} // namespace

W_CREATE_SIMPLE_TEST(SimdMath, SimdVec4d)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    WSimdVec4d vDefCtor;
    W_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
// GCC assumes that the contents of the memory prior to the placement constructor doesn't matter
// So it optimizes away the initialization.
#  if W_DISABLED(W_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(32) double testBlock[4] = {1, 2, 3, 4};
    WSimdVec4d* pDefCtor = ::new ((void*)&testBlock[0]) WSimdVec4d;
    W_TEST_BOOL(pDefCtor->x() == 1.0 && pDefCtor->y() == 2.0 && pDefCtor->z() == 3.0 && pDefCtor->w() == 4.0);
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
#  if W_SSE_LEVEL >= W_SSE_AVX
    static_assert(sizeof(WSimdVec4d) == 32);
    static_assert(alignof(WSimdVec4d) == 32);
#  else
    static_assert(sizeof(WSimdVec4d) == 32);
    static_assert(alignof(WSimdVec4d) == 16);
#  endif
#endif

    WSimdVec4d vInit1F(2.0);
    W_TEST_BOOL(vInit1F.x() == 2.0 && vInit1F.y() == 2.0 && vInit1F.z() == 2.0 && vInit1F.w() == 2.0);

    WSimdDouble a(3.0);
    WSimdVec4d vInit1SF(a);
    W_TEST_BOOL(vInit1SF.x() == 3.0 && vInit1SF.y() == 3.0 && vInit1SF.z() == 3.0 && vInit1SF.w() == 3.0);

    WSimdVec4d vInit4D(1.0, 2.0, 3.0, 4.0);
    W_TEST_BOOL(vInit4D.x() == 1.0 && vInit4D.y() == 2.0 && vInit4D.z() == 3.0 && vInit4D.w() == 4.0);

    // Make sure all components have the correct values
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if W_SSE_LEVEL >= W_SSE_AVX
    W_TEST_BOOL(
      vInit4D.m_v.m256d_f64[0] == 1.0 && vInit4D.m_v.m256d_f64[1] == 2.0 && vInit4D.m_v.m256d_f64[2] == 3.0 && vInit4D.m_v.m256d_f64[3] == 4.0);

#  else
    W_TEST_BOOL(
      vInit4D.m_v.xy.m128d_f64[0] == 1.0 && vInit4D.m_v.xy.m128d_f64[1] == 2.0 && vInit4D.m_v.zw.m128d_f64[0] == 3.0 && vInit4D.m_v.zw.m128d_f64[1] == 4.0);

#  endif
#endif

    WSimdVec4d vCopy(vInit4D);
    W_TEST_BOOL(vCopy.x() == 1.0 && vCopy.y() == 2.0 && vCopy.z() == 3.0 && vCopy.w() == 4.0);

    WSimdVec4d vZero = WSimdVec4d::MakeZero();
    W_TEST_BOOL(vZero.x() == 0.0 && vZero.y() == 0.0 && vZero.z() == 0.0 && vZero.w() == 0.0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Setter")
  {
    WSimdVec4d a;
    a.Set(2.0);
    W_TEST_BOOL(a.x() == 2.0 && a.y() == 2.0 && a.z() == 2.0 && a.w() == 2.0);

    WSimdVec4d b;
    b.Set(1.0, 2.0, 3.0, 4.0);
    W_TEST_BOOL(b.x() == 1.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetX(5.0);
    W_TEST_BOOL(b.x() == 5.0 && b.y() == 2.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetY(6.0);
    W_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 3.0 && b.w() == 4.0);

    b.SetZ(7.0);
    W_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 4.0);

    b.SetW(8.0);
    W_TEST_BOOL(b.x() == 5.0 && b.y() == 6.0 && b.z() == 7.0 && b.w() == 8.0);

    WSimdVec4d c;
    c.SetZero();
    W_TEST_BOOL(c.x() == 0.0 && c.y() == 0.0 && c.z() == 0.0 && c.w() == 0.0);

    {
      WSimdVec4d z = WSimdVec4d::MakeZero();
      W_TEST_BOOL(z.x() == 0.0 && z.y() == 0.0 && z.z() == 0.0 && z.w() == 0.0);
    }

    {
      WSimdVec4d z = WSimdVec4d::MakeNaN();
      W_TEST_BOOL(WMath::IsNaN((double)z.x()));
      W_TEST_BOOL(WMath::IsNaN((double)z.y()));
      W_TEST_BOOL(WMath::IsNaN((double)z.z()));
      W_TEST_BOOL(WMath::IsNaN((double)z.w()));
    }

    {
      double testBlock[4] = {1, 2, 3, 4};
      WSimdVec4d x;
      x.Load<1>(testBlock);
      W_TEST_BOOL(x.x() == 1.0 && x.y() == 0.0 && x.z() == 0.0 && x.w() == 0.0);

      WSimdVec4d xy;
      xy.Load<2>(testBlock);
      W_TEST_BOOL(xy.x() == 1.0 && xy.y() == 2.0 && xy.z() == 0.0 && xy.w() == 0.0);

      WSimdVec4d xyz;
      xyz.Load<3>(testBlock);
      W_TEST_BOOL(xyz.x() == 1.0 && xyz.y() == 2.0 && xyz.z() == 3.0 && xyz.w() == 0.0);

      WSimdVec4d xyzw;
      xyzw.Load<4>(testBlock);
      W_TEST_BOOL(xyzw.x() == 1.0 && xyzw.y() == 2.0 && xyzw.z() == 3.0 && xyzw.w() == 4.0);

      W_TEST_BOOL(xyzw.GetComponent(0) == 1.0);
      W_TEST_BOOL(xyzw.GetComponent(1) == 2.0);
      W_TEST_BOOL(xyzw.GetComponent(2) == 3.0);
      W_TEST_BOOL(xyzw.GetComponent(3) == 4.0);
      W_TEST_BOOL(xyzw.GetComponent(4) == 4.0);

      // Make sure all components have the correct values
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE && W_ENABLED(W_COMPILER_MSVC_PURE)
#  if W_SSE_LEVEL >= W_SSE_AVX
      W_TEST_BOOL(xyzw.m_v.m256d_f64[0] == 1.0 && xyzw.m_v.m256d_f64[1] == 2.0 && xyzw.m_v.m256d_f64[2] == 3.0 && xyzw.m_v.m256d_f64[3] == 4.0);
#  else
      W_TEST_BOOL(xyzw.m_v.xy.m128d_f64[0] == 1.0 && xyzw.m_v.xy.m128d_f64[1] == 2.0 && xyzw.m_v.zw.m128d_f64[0] == 3.0 && xyzw.m_v.zw.m128d_f64[1] == 4.0);
#  endif
#endif
    }

    // Test Load from float array - should zero unused components just like Load from double
    {
      float testBlockF[4] = {1.0f, 2.0f, 3.0f, 4.0f};

      WSimdVec4d x;
      x.Load<1>(testBlockF);
      W_TEST_BOOL(x.x() == 1.0 && x.y() == 0.0 && x.z() == 0.0 && x.w() == 0.0);

      WSimdVec4d xy;
      xy.Load<2>(testBlockF);
      W_TEST_BOOL(xy.x() == 1.0 && xy.y() == 2.0 && xy.z() == 0.0 && xy.w() == 0.0);

      WSimdVec4d xyz;
      xyz.Load<3>(testBlockF);
      W_TEST_BOOL(xyz.x() == 1.0 && xyz.y() == 2.0 && xyz.z() == 3.0 && xyz.w() == 0.0);

      WSimdVec4d xyzw;
      xyzw.Load<4>(testBlockF);
      W_TEST_BOOL(xyzw.x() == 1.0 && xyzw.y() == 2.0 && xyzw.z() == 3.0 && xyzw.w() == 4.0);
    }

    {
      double testBlock[4] = {7, 7, 7, 7};
      double mem[4] = {};

      WSimdVec4d b2(1.0, 2.0, 3.0, 4.0);

      memcpy(mem, testBlock, 32);
      b2.Store<1>(mem);
      W_TEST_BOOL(mem[0] == 1.0 && mem[1] == 7.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<2>(mem);
      W_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 7.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<3>(mem);
      W_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 7.0);

      memcpy(mem, testBlock, 32);
      b2.Store<4>(mem);
      W_TEST_BOOL(mem[0] == 1.0 && mem[1] == 2.0 && mem[2] == 3.0 && mem[3] == 4.0);
    }

    // Test Store to float array
    {
      float testBlockF[4] = {7.0f, 7.0f, 7.0f, 7.0f};
      float memF[4] = {};

      WSimdVec4d b2(1.0, 2.0, 3.0, 4.0);

      memcpy(memF, testBlockF, 16);
      b2.Store<1>(memF);
      W_TEST_BOOL(memF[0] == 1.0f && memF[1] == 7.0f && memF[2] == 7.0f && memF[3] == 7.0f);

      memcpy(memF, testBlockF, 16);
      b2.Store<2>(memF);
      W_TEST_BOOL(memF[0] == 1.0f && memF[1] == 2.0f && memF[2] == 7.0f && memF[3] == 7.0f);

      memcpy(memF, testBlockF, 16);
      b2.Store<3>(memF);
      W_TEST_BOOL(memF[0] == 1.0f && memF[1] == 2.0f && memF[2] == 3.0f && memF[3] == 7.0f);

      memcpy(memF, testBlockF, 16);
      b2.Store<4>(memF);
      W_TEST_BOOL(memF[0] == 1.0f && memF[1] == 2.0f && memF[2] == 3.0f && memF[3] == 4.0f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Functions")
  {
    {
      WSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      WSimdVec4d b(1.0, 0.5, 0.25, 0.125);

      W_TEST_BOOL(a.GetReciprocal().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
      W_TEST_BOOL(a.GetReciprocal().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
    }

    {
      WSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      WSimdVec4d b(1.0, WMath::Sqrt(2.0), WMath::Sqrt(4.0), WMath::Sqrt(8.0));

      W_TEST_BOOL(a.GetSqrt().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
      W_TEST_BOOL(a.GetSqrt().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
    }

    {
      WSimdVec4d a(1.0, 2.0, 4.0, 8.0);
      WSimdVec4d b(1.0, 1.0 / WMath::Sqrt(2.0), 1.0 / WMath::Sqrt(4.0), 1.0 / WMath::Sqrt(8.0));

      W_TEST_BOOL(a.GetInvSqrt().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
      W_TEST_BOOL(a.GetInvSqrt().IsEqual(b, WMath::SmallEpsilon<double>()).AllSet());
    }

    // IsEqual with epsilon edge cases
    {
      WSimdVec4d a(1.0, 2.0, 3.0, 4.0);
      WSimdVec4d b = a;
      // Equal with zero epsilon should work for identical values
      W_TEST_BOOL(a.IsEqual(b, 0.0).AllSet());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 2.0;
      r[1] = WVec2d(a.x(), a.y()).GetLength();
      r[2] = WVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = WVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();


      W_TEST_DOUBLE(a.GetLength<1>(), r[0], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLength<2>(), r[1], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLength<3>(), r[2], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLength<4>(), r[3], WMath::SmallEpsilon<double>());


      TestLength(a, r, WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 0.5;
      r[1] = 1.0 / WVec2d(a.x(), a.y()).GetLength();
      r[2] = 1.0 / WVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0 / WVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      W_TEST_DOUBLE(a.GetInvLength<1>(), r[0], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetInvLength<2>(), r[1], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetInvLength<3>(), r[2], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetInvLength<4>(), r[3], WMath::SmallEpsilon<double>());

      TestInvLength(a, r, WMath::SmallEpsilon<double>());
      // TestInvLength<WMathAcc::BITS_23>(a, r, WMath::DefaultEpsilon<double>());
      // TestInvLength<WMathAcc::BITS_12>(a, r, WMath::HugeEpsilon<double>());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      double r[4];
      r[0] = 2.0 * 2.0;
      r[1] = WVec2d(a.x(), a.y()).GetLengthSquared();
      r[2] = WVec3d(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = WVec4d(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      W_TEST_DOUBLE(a.GetLengthSquared<1>(), r[0], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLengthSquared<2>(), r[1], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLengthSquared<3>(), r[2], WMath::SmallEpsilon<double>());
      W_TEST_DOUBLE(a.GetLengthSquared<4>(), r[3], WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      WSimdDouble r[4];
      r[0] = 2.0;
      r[1] = WVec2d(a.x(), a.y()).GetLength();
      r[2] = WVec3d(a.x(), a.y(), a.z()).GetLength();
      r[3] = WVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      WSimdVec4d n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize(a, n, r, WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);
      WSimdVec4d n[4];
      n[0] = a / 2.0;
      n[1] = a / WVec2d(a.x(), a.y()).GetLength();
      n[2] = a / WVec3d(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / WVec4d(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero(a, n, WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a(2.0, -2.0, 4.0, -8.0);

      TestNormalizeIfNotZeroWithFallback(a, WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a;

      a.Set(0.0, 2.0, 0.0, 0.0);
      W_TEST_BOOL(a.IsZero<1>());
      W_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0, 0.0, 3.0, 0.0);
      W_TEST_BOOL(a.IsZero<2>());
      W_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0, 0.0, 0.0, 4.0);
      W_TEST_BOOL(a.IsZero<3>());
      W_TEST_BOOL(!a.IsZero<4>());

      double smallEps = WMath::SmallEpsilon<double>();
      a.Set(smallEps, 2.0, smallEps, smallEps);
      W_TEST_BOOL(a.IsZero<1>(WMath::DefaultEpsilon<double>()));
      W_TEST_BOOL(!a.IsZero<2>(WMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, 3.0, smallEps);
      W_TEST_BOOL(a.IsZero<2>(WMath::DefaultEpsilon<double>()));
      W_TEST_BOOL(!a.IsZero<3>(WMath::DefaultEpsilon<double>()));

      a.Set(smallEps, smallEps, smallEps, 4.0);
      W_TEST_BOOL(a.IsZero<3>(WMath::DefaultEpsilon<double>()));
      W_TEST_BOOL(!a.IsZero<4>(WMath::DefaultEpsilon<double>()));
    }

    {
      WSimdVec4d a;

      double NaN = WMath::NaN<double>();
      double Inf = WMath::Infinity<double>();

      a.Set(NaN, 1.0, NaN, NaN);
      W_TEST_BOOL(a.IsNaN<1>());
      W_TEST_BOOL(a.IsNaN<2>());
      W_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0, NaN, NaN);
      W_TEST_BOOL(!a.IsNaN<1>());
      W_TEST_BOOL(!a.IsNaN<2>());
      W_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0, 2.0, Inf, NaN);
      W_TEST_BOOL(a.IsNaN<4>());
      W_TEST_BOOL(!a.IsNaN<3>());
      W_TEST_BOOL(a.IsValid<2>());
      W_TEST_BOOL(!a.IsValid<3>());

      a.Set(-1.0, -2.0, -3.0, -4.0);
      W_TEST_BOOL(a.IsValid<1>());
      W_TEST_BOOL(a.IsValid<2>());
      W_TEST_BOOL(a.IsValid<3>());
      W_TEST_BOOL(a.IsValid<4>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swizzle")
  {
    WSimdVec4d a(3.0, 5.0, 7.0, 9.0);

    WSimdVec4d b = a.Get<WSwizzle::XXXX>();
    W_TEST_BOOL(b.x() == 3.0 && b.y() == 3.0 && b.z() == 3.0 && b.w() == 3.0);

    b = a.Get<WSwizzle::YYYX>();
    W_TEST_BOOL(b.x() == 5.0 && b.y() == 5.0 && b.z() == 5.0 && b.w() == 3.0);

    b = a.Get<WSwizzle::ZZZX>();
    W_TEST_BOOL(b.x() == 7.0 && b.y() == 7.0 && b.z() == 7.0 && b.w() == 3.0);

    b = a.Get<WSwizzle::WWWX>();
    W_TEST_BOOL(b.x() == 9.0 && b.y() == 9.0 && b.z() == 9.0 && b.w() == 3.0);

    b = a.Get<WSwizzle::WZYX>();
    W_TEST_BOOL(b.x() == 9.0 && b.y() == 7.0 && b.z() == 5.0 && b.w() == 3.0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCombined")
  {
    WSimdVec4d a(2.0, 4.0, 6.0, 8.0);
    WSimdVec4d b(3.0, 5.0, 7.0, 9.0);

    WSimdVec4d c = a.GetCombined<WSwizzle::XXXX>(b);
    W_TEST_BOOL(c.x() == a.x() && c.y() == a.x() && c.z() == b.x() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::YYYX>(b);
    W_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::ZZZX>(b);
    W_TEST_BOOL(c.x() == a.z() && c.y() == a.z() && c.z() == b.z() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::WWWX>(b);
    W_TEST_BOOL(c.x() == a.w() && c.y() == a.w() && c.z() == b.w() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::WZYX>(b);
    W_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::XYZW>(b);
    W_TEST_BOOL(c.x() == a.x() && c.y() == a.y() && c.z() == b.z() && c.w() == b.w());

    c = a.GetCombined<WSwizzle::WZYX>(b);
    W_TEST_BOOL(c.x() == a.w() && c.y() == a.z() && c.z() == b.y() && c.w() == b.x());

    c = a.GetCombined<WSwizzle::YYYY>(b);
    W_TEST_BOOL(c.x() == a.y() && c.y() == a.y() && c.z() == b.y() && c.w() == b.y());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      WSimdVec4d b = -a;
      W_TEST_BOOL(b.x() == 3.0 && b.y() == -5.0 && b.z() == 7.0 && b.w() == -9.0);

      b.Set(8.0, 6.0, 4.0, 2.0);
      WSimdVec4d c;
      c = a + b;
      W_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a - b;
      W_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a * WSimdDouble(3.0);
      W_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a / WSimdDouble(2.0);
      W_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);

      c = a.CompMul(b);
      W_TEST_BOOL(c.x() == -24.0 && c.y() == 30.0 && c.z() == -28.0 && c.w() == 18.0);

      WSimdVec4d divRes(-0.375, 5.0 / 6.0, -1.75, 4.5);
      WSimdVec4d d1 = a.CompDiv(b);


      W_TEST_BOOL(d1.IsEqual(divRes, WMath::SmallEpsilon<double>()).AllSet());
    }

    {
      WSimdVec4d a(-3.4, 5.4, -7.6, 9.6);
      WSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      WSimdVec4d c;

      c = a.CompMin(b);
      W_TEST_BOOL(c.x() == -3.4 && c.y() == 5.4 && c.z() == -7.6 && c.w() == 2.0);

      c = a.CompMax(b);
      W_TEST_BOOL(c.x() == 8.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.6);

      c = a.Abs();
      W_TEST_BOOL(c.x() == 3.4 && c.y() == 5.4 && c.z() == 7.6 && c.w() == 9.6);

      c = a.Round();
      W_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 10.0);

      c = a.Floor();
      W_TEST_BOOL(c.x() == -4.0 && c.y() == 5.0 && c.z() == -8.0 && c.w() == 9.0);

      c = a.Ceil();
      W_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == -7.0 && c.w() == 10.0);

      c = a.Trunc();
      W_TEST_BOOL(c.x() == -3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 9.0);

      c = a.Fraction();
      W_TEST_BOOL(c.IsEqual(WSimdVec4d(-0.4, 0.4, -0.6, 0.6), WMath::SmallEpsilon<double>()).AllSet());
    }

    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      WSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      WSimdVec4bWide cmp(true, false, false, true);
      WSimdVec4d c;

      c = a.FlipSign(cmp);
      W_TEST_BOOL(c.x() == 3.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == -9.0);

      c = WSimdVec4d::Select(cmp, b, a);
      W_TEST_BOOL(c.x() == 8.0 && c.y() == 5.0 && c.z() == -7.0 && c.w() == 2.0);

      c = WSimdVec4d::Select(cmp, a, b);
      W_TEST_BOOL(c.x() == -3.0 && c.y() == 6.0 && c.z() == 4.0 && c.w() == 9.0);

      WSimdVec4d a2(1.0, 2.0, 3.0, 4.0);
      WSimdVec4d b2(10.0, 20.0, 30.0, 40.0);
      WSimdVec4bWide alternating(true, false, true, false);
      WSimdVec4d result = WSimdVec4d::Select(alternating, a2, b2);
      W_TEST_BOOL(result.x() == 1.0 && result.y() == 20.0 && result.z() == 3.0 && result.w() == 40.0);
    }

    // FlipSign all components
    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      WSimdVec4bWide allTrue(true, true, true, true);
      WSimdVec4bWide allFalse(false, false, false, false);

      WSimdVec4d flippedAll = a.FlipSign(allTrue);
      W_TEST_BOOL(flippedAll.x() == 3.0 && flippedAll.y() == -5.0 && flippedAll.z() == 7.0 && flippedAll.w() == -9.0);

      WSimdVec4d flippedNone = a.FlipSign(allFalse);
      W_TEST_BOOL((flippedNone == a).AllSet());
    }

    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      WSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      WSimdVec4d c = a;
      c += b;
      W_TEST_BOOL(c.x() == 5.0 && c.y() == 11.0 && c.z() == -3.0 && c.w() == 11.0);

      c = a;
      c -= b;
      W_TEST_BOOL(c.x() == -11.0 && c.y() == -1.0 && c.z() == -11.0 && c.w() == 7.0);

      c = a;
      c *= WSimdDouble(3.0);
      W_TEST_BOOL(c.x() == -9.0 && c.y() == 15.0 && c.z() == -21.0 && c.w() == 27.0);

      c = a;
      c /= WSimdDouble(2.0);
      W_TEST_BOOL(c.x() == -1.5 && c.y() == 2.5 && c.z() == -3.5 && c.w() == 4.5);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdVec4d a(7.0, 5.0, 4.0, 3.0);
    WSimdVec4d b(8.0, 6.0, 4.0, 2.0);
    WSimdVec4bWide cmp;

    cmp = a == b;
    W_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    W_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    W_TEST_BOOL(cmp.x() && cmp.y() && cmp.z() && !cmp.w());

    cmp = a < b;
    W_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && !cmp.w());

    cmp = a >= b;
    W_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && cmp.w());

    cmp = a > b;
    W_TEST_BOOL(!cmp.x() && !cmp.y() && !cmp.z() && cmp.w());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Advanced Operators")
  {
    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);

      W_TEST_DOUBLE(a.HorizontalSum<1>(), -3.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalSum<2>(), 2.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalSum<3>(), -5.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalSum<4>(), 4.0, 0.0);
      W_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      W_TEST_DOUBLE(a.HorizontalMin<1>(), -3.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMin<2>(), -3.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMin<3>(), -7.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMin<4>(), -7.0, 0.0);
      W_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      W_TEST_DOUBLE(a.HorizontalMax<1>(), -3.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMax<2>(), 5.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMax<3>(), 5.0, 0.0);
      W_TEST_DOUBLE(a.HorizontalMax<4>(), 9.0, 0.0);
      W_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      W_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      WSimdVec4d b(8.0, 6.0, 4.0, 2.0);

      W_TEST_DOUBLE(a.Dot<1>(b), -24.0, 0.0);
      W_TEST_DOUBLE(a.Dot<2>(b), 6.0, 0.0);
      W_TEST_DOUBLE(a.Dot<3>(b), -22.0, 0.0);
      W_TEST_DOUBLE(a.Dot<4>(b), -4.0, 0.0);
      W_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      W_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      W_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      W_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      WSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      WSimdVec4d b(2.0, -4.0, 6.0, 8.0);

      WVec3d res = WVec3d(a.x(), a.y(), a.z()).CrossRH(WVec3d(b.x(), b.y(), b.z()));

      WSimdVec4d c = a.CrossRH(b);
      W_TEST_BOOL(c.x() == res.x);
      W_TEST_BOOL(c.y() == res.y);
      W_TEST_BOOL(c.z() == res.z);
    }

    {
      WSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      WSimdVec4d b(2.0, -4.0, 6.0, 0.0);

      WVec3d res = WVec3d(a.x(), a.y(), a.z()).CrossRH(WVec3d(b.x(), b.y(), b.z()));

      WSimdVec4d c = a.CrossRH(b);
      W_TEST_BOOL(c.x() == res.x);
      W_TEST_BOOL(c.y() == res.y);
      W_TEST_BOOL(c.z() == res.z);
    }

    // Cross product edge cases
    {
      // Parallel vectors (should produce zero)
      WSimdVec4d a(1.0, 2.0, 3.0, 0.0);
      WSimdVec4d b(2.0, 4.0, 6.0, 0.0); // parallel to a
      WSimdVec4d c = a.CrossRH(b);
      W_TEST_BOOL(c.IsZero<3>(WMath::SmallEpsilon<double>()));

      // Anti-parallel
      WSimdVec4d d(-1.0, -2.0, -3.0, 0.0);
      WSimdVec4d e = a.CrossRH(d);
      W_TEST_BOOL(e.IsZero<3>(WMath::SmallEpsilon<double>()));

      // Cross with zero
      WSimdVec4d zero = WSimdVec4d::MakeZero();
      WSimdVec4d f = a.CrossRH(zero);
      W_TEST_BOOL(f.IsZero<3>());
    }

    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 0.0);
      WSimdVec4d b = a.GetOrthogonalVector();

      W_TEST_BOOL(!b.IsZero<3>());
      W_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);

      a = WSimdVec4d(0.0, 1.0, 0.0, 0.0);
      b = a.GetOrthogonalVector();

      W_TEST_BOOL(!b.IsZero<3>());
      W_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);

      a = WSimdVec4d(0.0, 0.0, 1.0, 0.0);
      b = a.GetOrthogonalVector();

      W_TEST_BOOL(!b.IsZero<3>());
      W_TEST_DOUBLE(a.Dot<3>(b), 0.0, 0.0);
    }

    // GetOrthogonalVector edge cases
    {
      // Near-axis-aligned vectors
      WSimdVec4d a(1e-10, 0.9999999999, 1e-10, 0.0);
      a.Normalize<3>();
      WSimdVec4d b = a.GetOrthogonalVector();

      W_TEST_BOOL(!b.IsZero<3>());
      W_TEST_DOUBLE(a.Dot<3>(b), 0.0, WMath::SmallEpsilon<double>());
    }

    {
      WSimdVec4d a(-3.0, 5.0, -7.0, 9.0);
      WSimdVec4d b(8.0, 6.0, 4.0, 2.0);
      WSimdVec4d c(1.0, 2.0, 3.0, 4.0);
      WSimdVec4d d;

      d = WSimdVec4d::MulAdd(a, b, c);
      W_TEST_BOOL(d.x() == -23.0 && d.y() == 32.0 && d.z() == -25.0 && d.w() == 22.0);

      d = WSimdVec4d::MulAdd(a, WSimdDouble(3.0), c);
      W_TEST_BOOL(d.x() == -8.0 && d.y() == 17.0 && d.z() == -18.0 && d.w() == 31.0);

      d = WSimdVec4d::MulSub(a, b, c);
      W_TEST_BOOL(d.x() == -25.0 && d.y() == 28.0 && d.z() == -31.0 && d.w() == 14.0);

      d = WSimdVec4d::MulSub(a, WSimdDouble(3.0), c);
      W_TEST_BOOL(d.x() == -10.0 && d.y() == 13.0 && d.z() == -24.0 && d.w() == 23.0);

      d = WSimdVec4d::CopySign(b, a);
      W_TEST_BOOL(d.x() == -8.0 && d.y() == 6.0 && d.z() == -4.0 && d.w() == 2.0);

      // Negative zero handling with CopySign
      WSimdVec4d mag(1.0, 2.0, 3.0, 4.0);
      WSimdVec4d negZero(-0.0, 0.0, -0.0, 0.0);
      WSimdVec4d result = WSimdVec4d::CopySign(mag, negZero);
      W_TEST_BOOL(result.x() == -1.0 && result.y() == 2.0 && result.z() == -3.0 && result.w() == 4.0);
    }

    // Lerp edge cases
    {
      WSimdVec4d a(1.0, 2.0, 3.0, 4.0);
      WSimdVec4d b(5.0, 6.0, 7.0, 8.0);

      // t = 0 should give a
      WSimdVec4d t0 = WSimdVec4d::Lerp(a, b, WSimdVec4d(0.0));
      W_TEST_BOOL((t0 == a).AllSet());

      // t = 1 should give b
      WSimdVec4d t1 = WSimdVec4d::Lerp(a, b, WSimdVec4d(1.0));
      W_TEST_BOOL((t1 == b).AllSet());

      // t = 0.5 should give midpoint
      WSimdVec4d tHalf = WSimdVec4d::Lerp(a, b, WSimdVec4d(0.5));
      W_TEST_BOOL(tHalf.IsEqual(WSimdVec4d(3.0, 4.0, 5.0, 6.0), WMath::SmallEpsilon<double>()).AllSet());

      // t outside [0,1] - extrapolation
      WSimdVec4d tNeg = WSimdVec4d::Lerp(a, b, WSimdVec4d(-1.0));
      W_TEST_BOOL(tNeg.IsEqual(WSimdVec4d(-3.0, -2.0, -1.0, 0.0), WMath::SmallEpsilon<double>()).AllSet());
    }
  }
}
