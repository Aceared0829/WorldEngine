#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const WCoordinateSystemConversion& atoB, const WCoordinateSystemConversion& btoA, float fSourceLength, float fTargetLength)
{
  W_TEST_FLOAT(atoB.ConvertSourceLength(fSourceLength), fTargetLength, WMath::DefaultEpsilon<float>());
  W_TEST_FLOAT(atoB.ConvertTargetLength(fTargetLength), fSourceLength, WMath::DefaultEpsilon<float>());

  W_TEST_FLOAT(btoA.ConvertTargetLength(fSourceLength), fTargetLength, WMath::DefaultEpsilon<float>());
  W_TEST_FLOAT(btoA.ConvertSourceLength(fTargetLength), fSourceLength, WMath::DefaultEpsilon<float>());
}

void TestPosition(
  const WCoordinateSystemConversion& atoB, const WCoordinateSystemConversion& btoA, const WVec3& vSourcePos, const WVec3& vTargetPos)
{
  TestLength(atoB, btoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  W_TEST_VEC3(atoB.ConvertSourcePosition(vSourcePos), vTargetPos, WMath::DefaultEpsilon<float>());
  W_TEST_VEC3(atoB.ConvertTargetPosition(vTargetPos), vSourcePos, WMath::DefaultEpsilon<float>());
  W_TEST_VEC3(btoA.ConvertSourcePosition(vTargetPos), vSourcePos, WMath::DefaultEpsilon<float>());
  W_TEST_VEC3(btoA.ConvertTargetPosition(vSourcePos), vTargetPos, WMath::DefaultEpsilon<float>());
}

void TestRotation(const WCoordinateSystemConversion& atoB, const WCoordinateSystemConversion& btoA, const WVec3& vSourceStartDir,
  const WVec3& vSourceEndDir, const WQuat& qSourceRot, const WVec3& vTargetStartDir, const WVec3& vTargetEndDir, const WQuat& qTargetRot)
{
  TestPosition(atoB, btoA, vSourceStartDir, vTargetStartDir);
  TestPosition(atoB, btoA, vSourceEndDir, vTargetEndDir);

  W_TEST_BOOL(atoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, WMath::DefaultEpsilon<float>()));
  W_TEST_BOOL(atoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, WMath::DefaultEpsilon<float>()));
  W_TEST_BOOL(btoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, WMath::DefaultEpsilon<float>()));
  W_TEST_BOOL(btoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, WMath::DefaultEpsilon<float>()));

  W_TEST_VEC3(qSourceRot * vSourceStartDir, vSourceEndDir, WMath::DefaultEpsilon<float>());
  W_TEST_VEC3(qTargetRot * vTargetStartDir, vTargetEndDir, WMath::DefaultEpsilon<float>());
}

WQuat FromAxisAndAngle(const WVec3& vAxis, WAngle angle)
{
  WQuat q = WQuat::MakeFromAxisAndAngle(vAxis.GetNormalized(), angle);
  return q;
}

bool IsRightHanded(const WCoordinateSystem& cs)
{
  WVec3 vF = cs.m_vUpDir.CrossRH(cs.m_vRightDir);

  return vF.Dot(cs.m_vForwardDir) > 0;
}

void TestCoordinateSystemConversion(const WCoordinateSystem& a, const WCoordinateSystem& b)
{
  const bool bAisRH = IsRightHanded(a);
  const bool bBisRH = IsRightHanded(b);
  const WAngle A_CWRot = bAisRH ? WAngle::MakeFromDegree(-90.0f) : WAngle::MakeFromDegree(90.0f);
  const WAngle B_CWRot = bBisRH ? WAngle::MakeFromDegree(-90.0f) : WAngle::MakeFromDegree(90.0f);

  WCoordinateSystemConversion AtoB;
  AtoB.SetConversion(a, b);

  WCoordinateSystemConversion BtoA;
  BtoA.SetConversion(b, a);

  TestPosition(AtoB, BtoA, a.m_vForwardDir, b.m_vForwardDir);
  TestPosition(AtoB, BtoA, a.m_vRightDir, b.m_vRightDir);
  TestPosition(AtoB, BtoA, a.m_vUpDir, b.m_vUpDir);

  TestRotation(AtoB, BtoA, a.m_vForwardDir, a.m_vRightDir, FromAxisAndAngle(a.m_vUpDir, A_CWRot), b.m_vForwardDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vForwardDir, FromAxisAndAngle(a.m_vRightDir, A_CWRot), b.m_vUpDir, b.m_vForwardDir,
    FromAxisAndAngle(b.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vRightDir, FromAxisAndAngle(a.m_vForwardDir, -A_CWRot), b.m_vUpDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vForwardDir, -B_CWRot));
}


W_CREATE_SIMPLE_TEST(World, CoordinateSystem)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "W / OpenXR")
  {
    WCoordinateSystem WCoordSysLH;
    WCoordSysLH.m_vForwardDir = WVec3(1.0f, 0.0f, 0.0f);
    WCoordSysLH.m_vRightDir = WVec3(0.0f, 1.0f, 0.0f);
    WCoordSysLH.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);

    WCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = WVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = WVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = WVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(WCoordSysLH, openXrRH);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Scaled W / Scaled OpenXR")
  {
    WCoordinateSystem WCoordSysLH;
    WCoordSysLH.m_vForwardDir = WVec3(0.1f, 0.0f, 0.0f);
    WCoordSysLH.m_vRightDir = WVec3(0.0f, 0.1f, 0.0f);
    WCoordSysLH.m_vUpDir = WVec3(0.0f, 0.0f, 0.1f);

    WCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = WVec3(0.0f, 0.0f, -20.0f);
    openXrRH.m_vRightDir = WVec3(20.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = WVec3(0.0f, 20.0f, 0.0f);

    TestCoordinateSystemConversion(WCoordSysLH, openXrRH);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "W / Flipped W")
  {
    WCoordinateSystem WCoordSysLH;
    WCoordSysLH.m_vForwardDir = WVec3(1.0f, 0.0f, 0.0f);
    WCoordSysLH.m_vRightDir = WVec3(0.0f, 1.0f, 0.0f);
    WCoordSysLH.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);

    WCoordinateSystem WCoordSysFlippedLH;
    WCoordSysFlippedLH.m_vForwardDir = WVec3(-1.0f, 0.0f, 0.0f);
    WCoordSysFlippedLH.m_vRightDir = WVec3(0.0f, -1.0f, 0.0f);
    WCoordSysFlippedLH.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(WCoordSysLH, WCoordSysFlippedLH);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "OpenXR / Flipped OpenXR")
  {
    WCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = WVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = WVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = WVec3(0.0f, 1.0f, 0.0f);

    WCoordinateSystem openXrFlippedRH;
    openXrFlippedRH.m_vForwardDir = WVec3(0.0f, 0.0f, 1.0f);
    openXrFlippedRH.m_vRightDir = WVec3(-1.0f, 0.0f, 0.0f);
    openXrFlippedRH.m_vUpDir = WVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(openXrRH, openXrFlippedRH);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Identity")
  {
    WCoordinateSystem WCoordSysLH;
    WCoordSysLH.m_vForwardDir = WVec3(1.0f, 0.0f, 0.0f);
    WCoordSysLH.m_vRightDir = WVec3(0.0f, 1.0f, 0.0f);
    WCoordSysLH.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(WCoordSysLH, WCoordSysLH);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Constructed")
  {
    WCoordinateSystem WCoordSysLH;
    WCoordSysLH.m_vForwardDir = WVec3(1.0f, 0.0f, 0.0f);
    WCoordSysLH.m_vRightDir = WVec3(0.0f, 1.0f, 0.0f);
    WCoordSysLH.m_vUpDir = WVec3(0.0f, 0.0f, 1.0f);

    const WAngle rot = WAngle::MakeFromDegree(90.0f);

    WCoordinateSystemConversion defaultConstucted;

    TestPosition(defaultConstucted, defaultConstucted, WCoordSysLH.m_vForwardDir, WCoordSysLH.m_vForwardDir);
    TestPosition(defaultConstucted, defaultConstucted, WCoordSysLH.m_vRightDir, WCoordSysLH.m_vRightDir);
    TestPosition(defaultConstucted, defaultConstucted, WCoordSysLH.m_vUpDir, WCoordSysLH.m_vUpDir);

    TestRotation(defaultConstucted, defaultConstucted, WCoordSysLH.m_vForwardDir, WCoordSysLH.m_vRightDir,
      FromAxisAndAngle(WCoordSysLH.m_vUpDir, rot), WCoordSysLH.m_vForwardDir, WCoordSysLH.m_vRightDir,
      FromAxisAndAngle(WCoordSysLH.m_vUpDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, WCoordSysLH.m_vUpDir, WCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(WCoordSysLH.m_vRightDir, rot), WCoordSysLH.m_vUpDir, WCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(WCoordSysLH.m_vRightDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, WCoordSysLH.m_vUpDir, WCoordSysLH.m_vRightDir,
      FromAxisAndAngle(WCoordSysLH.m_vForwardDir, -rot), WCoordSysLH.m_vUpDir, WCoordSysLH.m_vRightDir,
      FromAxisAndAngle(WCoordSysLH.m_vForwardDir, -rot));
  }
}
