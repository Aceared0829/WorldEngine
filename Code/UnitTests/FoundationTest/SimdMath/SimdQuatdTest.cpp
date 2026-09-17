#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

W_CREATE_SIMPLE_TEST(SimdMath, WSimdQuatd)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    WSimdQuatd vDefCtor;
    W_TEST_BOOL(vDefCtor.IsNaN());
#else

#  if W_DISABLED(W_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) double testBlock[4] = {1, 2, 3, 4};
    WSimdQuatd* pDefCtor = ::new ((void*)&testBlock[0]) WSimdQuatd;
    W_TEST_BOOL(pDefCtor->m_v.x() == 1.0f && pDefCtor->m_v.y() == 2.0f && pDefCtor->m_v.z() == 3.0f && pDefCtor->m_v.w() == 4.0f);
#  endif

#endif

    // Make sure the class didn't accidentally change in size.
#if W_SSE_LEVEL >= W_SSE_AVX
    static_assert(sizeof(WSimdQuatd) == 32);
    static_assert(alignof(WSimdQuatd) == 32);
#else

    static_assert(sizeof(WSimdQuatd) == 32);
#if W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
    static_assert(alignof(WSimdQuatd) == 16);
#elif W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_FPU
    static_assert(alignof(WSimdQuatd) == 8);
#endif

#endif

  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IdentityQuaternion")
  {
    WSimdQuatd q = WSimdQuatd::MakeIdentity();

    W_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetIdentity")
  {
    WSimdQuatd q(WSimdVec4d(1, 2, 3, 4));

    q = WSimdQuatd::MakeIdentity();

    W_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(1, 0, 0), WAngle::MakeFromDegree(90));

      W_TEST_BOOL((q * WSimdVec4d(0, 1, 0)).IsEqual(WSimdVec4d(0, 0, 1), 0.0001f).AllSet());
    }

    {
      WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngle::MakeFromDegree(90));

      W_TEST_BOOL((q * WSimdVec4d(1, 0, 0)).IsEqual(WSimdVec4d(0, 0, -1), 0.0001f).AllSet());
    }

    {
      WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(90));

      W_TEST_BOOL((q * WSimdVec4d(0, 1, 0)).IsEqual(WSimdVec4d(-1, 0, 0), 0.0001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    WSimdQuatd q1, q2, q3;
    q1 = WSimdQuatd::MakeShortestRotation(WSimdVec4d(0, 1, 0), WSimdVec4d(1, 0, 0));
    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, -1), WAngle::MakeFromDegree(90));
    q3 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(-90));

    W_TEST_BOOL(q1.IsEqualRotation(q2, WMath::LargeEpsilon<float>()));
    W_TEST_BOOL(q1.IsEqualRotation(q3, WMath::LargeEpsilon<float>()));

    W_TEST_BOOL(WSimdQuatd::MakeIdentity().IsEqualRotation(WSimdQuatd::MakeIdentity(), WMath::LargeEpsilon<float>()));
    W_TEST_BOOL(WSimdQuatd::MakeIdentity().IsEqualRotation(WSimdQuatd(WSimdVec4d(0, 0, 0, -1)), WMath::LargeEpsilon<float>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSlerp")
  {
    WSimdQuatd q1, q2, q3, qr;
    q1 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(45));
    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(0));
    q3 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(90));

    qr = WSimdQuatd::MakeSlerp(q2, q3, 0.5f);

    W_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    WSimdQuatd q1, q2, q3;
    q1 = WSimdQuatd::MakeShortestRotation(WSimdVec4d(0, 1, 0), WSimdVec4d(1, 0, 0));
    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, -1), WAngle::MakeFromDegree(90));
    q3 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(-90));

    WSimdVec4d axis;
    WSimdDouble angle;

    W_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == W_SUCCESS);
    W_TEST_BOOL(axis.IsEqual(WSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    W_TEST_FLOAT(WAngle::RadToDeg((float)angle), 90, WMath::LargeEpsilon<float>());

    W_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == W_SUCCESS);
    W_TEST_BOOL(axis.IsEqual(WSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    W_TEST_FLOAT(WAngle::RadToDeg((float)angle), 90, WMath::LargeEpsilon<float>());

    W_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == W_SUCCESS);
    W_TEST_BOOL(axis.IsEqual(WSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    W_TEST_FLOAT(WAngle::RadToDeg((float)angle), 90, WMath::LargeEpsilon<float>());

    W_TEST_BOOL(WSimdQuatd::MakeIdentity().GetRotationAxisAndAngle(axis, angle) == W_SUCCESS);
    W_TEST_BOOL(axis.IsEqual(WSimdVec4d(1, 0, 0), 0.001f).AllSet<3>());
    W_TEST_FLOAT(WAngle::RadToDeg((float)angle), 0, WMath::LargeEpsilon<float>());

    WSimdQuatd otherIdentity(WSimdVec4d(0, 0, 0, -1));
    W_TEST_BOOL(otherIdentity.GetRotationAxisAndAngle(axis, angle) == W_SUCCESS);
    W_TEST_BOOL(axis.IsEqual(WSimdVec4d(1, 0, 0), 0.001f).AllSet<3>());
    W_TEST_FLOAT(WAngle::RadToDeg((float)angle), 360, WMath::LargeEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid / Normalize")
  {
    WSimdQuatd q(WSimdVec4d(1, 2, 3, 4));
    W_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    W_TEST_BOOL(q.IsValid(0.001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator-")
  {
    WSimdQuatd q, q1;
    q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(90));
    q1 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(-90));

    WSimdQuatd q2 = -q;
    W_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(quat, quat)")
  {
    WSimdQuatd q1, q2, qr, q3;
    q1 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(60));
    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(30));
    q3 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(90));

    qr = q1 * q2;

    W_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator==/!=")
  {
    WSimdQuatd q1, q2;
    q1 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(60));
    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(30));
    W_TEST_BOOL(q1 != q2);

    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(1, 0, 0), WAngle::MakeFromDegree(60));
    W_TEST_BOOL(q1 != q2);

    q2 = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngle::MakeFromDegree(60));
    W_TEST_BOOL(q1 == q2);
  }
}
