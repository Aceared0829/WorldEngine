#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Spline.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/SimdMath/SimdConversion.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSplineTangentMode, 1)
  W_ENUM_CONSTANTS(WSplineTangentMode::Auto, WSplineTangentMode::Custom, WSplineTangentMode::Linear)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WResult WSpline::ControlPoint::Serialize(WStreamWriter& s) const
{
  s << WSimdConversion::ToVec3(m_vPos);
  s << WSimdConversion::ToVec4(m_vPosTangentIn);  // Contains the tangent mode in w
  s << WSimdConversion::ToVec4(m_vPosTangentOut); // Contains the tangent mode in w

  s << WSimdConversion::ToVec4(m_vUpDirAndRoll);  // Roll in w
  s << WSimdConversion::ToVec3(m_vUpDirTangentIn);
  s << WSimdConversion::ToVec3(m_vUpDirTangentOut);

  s << WSimdConversion::ToVec3(m_vScale);
  s << WSimdConversion::ToVec3(m_vScaleTangentIn);
  s << WSimdConversion::ToVec3(m_vScaleTangentOut);

  return W_SUCCESS;
}

WResult WSpline::ControlPoint::Deserialize(WStreamReader& s)
{
  {
    WVec3 vPos;
    s >> vPos;

    WVec4 vPosTangentIn, vPosTangentOut; // Contains the tangent mode in w
    s >> vPosTangentIn;
    s >> vPosTangentOut;

    m_vPos = WSimdConversion::ToVec3(vPos);
    m_vPosTangentIn = WSimdConversion::ToVec4(vPosTangentIn);
    m_vPosTangentOut = WSimdConversion::ToVec4(vPosTangentOut);
  }

  {
    WVec4 vUpDirAndRoll; // Roll in w
    s >> vUpDirAndRoll;

    WVec3 vUpDirTangentIn, vUpDirTangentOut;
    s >> vUpDirTangentIn;
    s >> vUpDirTangentOut;

    m_vUpDirAndRoll = WSimdConversion::ToVec4(vUpDirAndRoll);
    m_vUpDirTangentIn = WSimdConversion::ToVec3(vUpDirTangentIn);
    m_vUpDirTangentOut = WSimdConversion::ToVec3(vUpDirTangentOut);
  }

  {
    WVec3 vScale, vScaleTangentIn, vScaleTangentOut;
    s >> vScale;
    s >> vScaleTangentIn;
    s >> vScaleTangentOut;

    m_vScale = WSimdConversion::ToVec3(vScale);
    m_vScaleTangentIn = WSimdConversion::ToVec3(vScaleTangentIn);
    m_vScaleTangentOut = WSimdConversion::ToVec3(vScaleTangentOut);
  }

  return W_SUCCESS;
}

void WSpline::ControlPoint::SetAutoTangents(const WSimdVec4f& vDirIn, const WSimdVec4f& vDirOut)
{
  const WSimdVec4f autoPosTangent = (vDirIn + vDirOut) * 0.5f;
  const WSimdFloat eps = WMath::LargeEpsilon<float>();

  {
    auto tangentModeIn = GetTangentModeIn();
    if (tangentModeIn == WSplineTangentMode::Auto)
    {
      m_vPosTangentIn = -autoPosTangent;
    }
    else if (tangentModeIn == WSplineTangentMode::Linear)
    {
      m_vPosTangentIn = -vDirIn;
    }
    else
    {
      W_ASSERT_DEV(tangentModeIn == WSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentIn.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentIn = vDirIn;
      m_vPosTangentIn.NormalizeIfNotZero<3>(WSimdVec4f(-1, 0, 0));
      m_vPosTangentIn *= eps;
    }

    SetTangentModeIn(WSplineTangentMode::Custom);
  }

  {
    auto tangentModeOut = GetTangentModeOut();
    if (tangentModeOut == WSplineTangentMode::Auto)
    {
      m_vPosTangentOut = autoPosTangent;
    }
    else if (tangentModeOut == WSplineTangentMode::Linear)
    {
      m_vPosTangentOut = vDirOut;
    }
    else
    {
      W_ASSERT_DEV(tangentModeOut == WSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentOut.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentOut = vDirOut;
      m_vPosTangentOut.NormalizeIfNotZero<3>(WSimdVec4f(1, 0, 0));
      m_vPosTangentOut *= eps;
    }

    SetTangentModeOut(WSplineTangentMode::Custom);
  }
}

//////////////////////////////////////////////////////////////////////////

constexpr WTypeVersion s_SplineVersion = 1;

WResult WSpline::Serialize(WStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_SplineVersion);

  W_SUCCEED_OR_RETURN(ref_writer.WriteArray(m_ControlPoints));
  ref_writer << m_bClosed;

  return W_SUCCESS;
}

WResult WSpline::Deserialize(WStreamReader& ref_reader)
{
  /*const WTypeVersion version =*/ref_reader.ReadVersion(s_SplineVersion);

  W_SUCCEED_OR_RETURN(ref_reader.ReadArray(m_ControlPoints));
  ref_reader >> m_bClosed;

  return W_SUCCESS;
}

void WSpline::CalculateUpDirAndAutoTangents(const WSimdVec4f& vGlobalUpDir, const WSimdVec4f& vGlobalForwardDir)
{
  const WUInt32 uiNumPoints = m_ControlPoints.GetCount();
  if (uiNumPoints < 2)
    return;

  const WUInt32 uiLastIdx = uiNumPoints - 1;
  const WSimdFloat oneThird(1.0f / 3.0f);

  // Position tangents
  {
    WUInt32 uiNumTangentsToUpdate = uiNumPoints;
    WUInt32 uiPrevIdx = uiLastIdx - 1;
    WUInt32 uiCurIdx = uiLastIdx;
    WUInt32 uiNextIdx = 0;

    if (!m_bClosed)
    {
      const WSimdVec4f vStartTangent = (m_ControlPoints[1].m_vPos - m_ControlPoints[0].m_vPos) * oneThird;
      const WSimdVec4f vEndTangent = (m_ControlPoints[uiLastIdx].m_vPos - m_ControlPoints[uiLastIdx - 1].m_vPos) * oneThird;

      m_ControlPoints[0].SetAutoTangents(vStartTangent, vStartTangent);
      m_ControlPoints[uiLastIdx].SetAutoTangents(vEndTangent, vEndTangent);

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (WUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      const WSimdVec4f dirIn = (cCp.m_vPos - pCP.m_vPos) * oneThird;
      const WSimdVec4f dirOut = (nCP.m_vPos - cCp.m_vPos) * oneThird;

      cCp.SetAutoTangents(dirIn, dirOut);

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }

  // Up dir
  {
    for (WUInt32 i = 0; i < uiNumPoints; ++i)
    {
      auto& cp = m_ControlPoints[i];
      if (cp.m_vUpDirAndRoll.IsZero<3>() == false)
        continue;

      WSimdVec4f forwardDir = EvaluateDerivative(i, 0.0f);
      forwardDir.NormalizeIfNotZero<3>(vGlobalForwardDir);

      const WSimdVec4f upDir = [&]()
      {
        if (forwardDir.Dot<3>(vGlobalUpDir).Abs() < 0.99f)
          return vGlobalUpDir;

        if (i > 0)
        {
          auto& prevCp = m_ControlPoints[i - 1];
          if (forwardDir.Dot<3>(prevCp.m_vUpDirAndRoll).Abs() < 0.99f)
          {
            return prevCp.m_vUpDirAndRoll;
          }
        }

        return vGlobalForwardDir;
      }();

      const WSimdVec4f rightDir = upDir.CrossRH(forwardDir).GetNormalized<3>();
      const WSimdVec4f upDir2 = forwardDir.CrossRH(rightDir).GetNormalized<3>();
      const WSimdFloat roll = cp.GetRoll();
      const WSimdQuat rotation = WSimdQuat::MakeFromAxisAndAngle(forwardDir, roll);

      cp.m_vUpDirAndRoll = rotation * upDir2;
      cp.m_vUpDirAndRoll.SetW(roll);
      cp.m_vUpDirTangentIn.SetZero();
      cp.m_vUpDirTangentOut.SetZero();

      W_ASSERT_DEBUG(cp.m_vUpDirAndRoll.IsValid<4>(), "Invalid up dir");
    }
  }

  // up dir and scale tangents
  {
    WUInt32 uiNumTangentsToUpdate = uiNumPoints;
    WUInt32 uiPrevIdx = uiLastIdx - 1;
    WUInt32 uiCurIdx = uiLastIdx;
    WUInt32 uiNextIdx = 0;


    if (!m_bClosed)
    {
      {
        auto& cp0 = m_ControlPoints[0];
        auto& cp1 = m_ControlPoints[1];

        const WSimdVec4f vUpDirTangent = (cp1.m_vUpDirAndRoll - cp0.m_vUpDirAndRoll) * oneThird;
        const WSimdVec4f vScaleTangent = (cp1.m_vScale - cp0.m_vScale) * oneThird;

        cp0.m_vUpDirTangentIn = -vUpDirTangent;
        cp0.m_vUpDirTangentOut = vUpDirTangent;
        cp0.m_vScaleTangentIn = -vScaleTangent;
        cp0.m_vScaleTangentOut = vScaleTangent;
      }

      {
        auto& cpLast = m_ControlPoints[uiLastIdx];
        auto& cpPrev = m_ControlPoints[uiLastIdx - 1];

        const WSimdVec4f vUpDirTangent = (cpLast.m_vUpDirAndRoll - cpPrev.m_vUpDirAndRoll) * oneThird;
        const WSimdVec4f vScaleTangent = (cpLast.m_vScale - cpPrev.m_vScale) * oneThird;

        cpLast.m_vUpDirTangentIn = -vUpDirTangent;
        cpLast.m_vUpDirTangentOut = vUpDirTangent;
        cpLast.m_vScaleTangentIn = -vScaleTangent;
        cpLast.m_vScaleTangentOut = vScaleTangent;
      }

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (WUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      // Do not use classic auto tangents here, since we don't want overshooting for the up direction and scale.
      const WSimdVec4f vUpDirTangent = (cCp.m_vUpDirAndRoll - pCP.m_vUpDirAndRoll).CompMin(nCP.m_vUpDirAndRoll - cCp.m_vUpDirAndRoll) * oneThird;
      const WSimdVec4f vScaleTangent = (cCp.m_vScale - pCP.m_vScale).CompMin(nCP.m_vScale - cCp.m_vScale) * oneThird;

      cCp.m_vUpDirTangentIn = -vUpDirTangent;
      cCp.m_vUpDirTangentOut = vUpDirTangent;
      cCp.m_vScaleTangentIn = -vScaleTangent;
      cCp.m_vScaleTangentOut = vScaleTangent;

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }
}

WSimdTransform WSpline::EvaluateTransform(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return WSimdTransform::MakeIdentity();

  WUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const WUInt32 uiCp1 = GetCp1Index(uiCp0);
  const auto& cp0 = m_ControlPoints[uiCp0];
  const auto& cp1 = m_ControlPoints[uiCp1];

  WSimdTransform transform;
  transform.m_Position = EvaluatePosition(cp0, cp1, fT);

  WSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(cp0, cp1, fT, forwardDir, rightDir, upDir);

  WMat3 mRot;
  mRot.SetColumn(0, WSimdConversion::ToVec3(forwardDir));
  mRot.SetColumn(1, WSimdConversion::ToVec3(rightDir));
  mRot.SetColumn(2, WSimdConversion::ToVec3(upDir));
  transform.m_Rotation = WSimdConversion::ToQuat(WQuat::MakeFromMat3(mRot));

  transform.m_Scale = EvaluateScale(cp0, cp1, fT);

  return transform;
}

WResult WSpline::CalculateSegmentBounds(WUInt32 uiSegmentIndex, WSimdBBoxSphere& out_bounds) const
{
  W_ASSERT_DEBUG(uiSegmentIndex < m_ControlPoints.GetCount(), "Invalid segment index");

  auto& cp0 = m_ControlPoints[uiSegmentIndex];
  auto& cp1 = m_ControlPoints[GetCp1Index(uiSegmentIndex)];

  const WSimdVec4f points[] = {
    cp0.m_vPos,
    cp0.m_vPos + cp0.m_vPosTangentOut,
    cp1.m_vPos + cp1.m_vPosTangentIn,
    cp1.m_vPos,
  };

  out_bounds = WSimdBBoxSphere::MakeFromPoints(points, W_ARRAY_SIZE(points));
  return W_SUCCESS;
}

WResult WSpline::CalculateBounds(WSimdBBoxSphere& out_bounds) const
{
  if (m_ControlPoints.GetCount() < 2)
  {
    out_bounds = WSimdBBoxSphere::MakeInvalid();
    return W_FAILURE;
  }

  const WUInt32 uiNumSegments = GetNumSegments();

  out_bounds = WSimdBBoxSphere::MakeInvalid();
  for (WUInt32 i = 0; i < uiNumSegments; ++i)
  {
    WSimdBBoxSphere segmentBounds;
    W_SUCCEED_OR_RETURN(CalculateSegmentBounds(i, segmentBounds));
    out_bounds.ExpandToInclude(segmentBounds);
  }

  return W_SUCCESS;
}

W_ALWAYS_INLINE WSimdVec4f FindIteration(const WSimdVec4f& vP0, const WSimdVec4f& vP1, const WSimdVec4f& vP2, const WSimdVec4f& vP3, const WSimdFloat& fMinT, const WSimdFloat& fMaxT, const WSimdVec4f& vPoint, WSimdVec4f& out_vClosestDistSqr, WSimdVec4f& out_vClosestT, WSimdFloat& out_fStep)
{
  WSimdVec4f vClosestPoint = WMath::EvaluateBezierCurve(fMinT, vP0, vP1, vP2, vP3);
  WSimdVec4f vClosestDistSqr = WSimdVec4f((vClosestPoint - vPoint).GetLengthSquared<3>());
  WSimdVec4f vClosestT = WSimdVec4f(fMinT);

  const WUInt32 numSteps = 8;
  WSimdFloat fStep = (fMaxT - fMinT) / WSimdFloat(static_cast<float>(numSteps));
  for (WSimdFloat fT = fStep; fT <= fMaxT; fT += fStep)
  {
    const WSimdVec4f vCandidate = WMath::EvaluateBezierCurve(fT, vP0, vP1, vP2, vP3);
    const WSimdVec4f vDistSqr = WSimdVec4f((vCandidate - vPoint).GetLengthSquared<3>());
    const WSimdVec4b bIsCloser = (vDistSqr < vClosestDistSqr);

    vClosestPoint = WSimdVec4f::Select(bIsCloser, vCandidate, vClosestPoint);
    vClosestDistSqr = WSimdVec4f::Select(bIsCloser, vDistSqr, vClosestDistSqr);
    vClosestT = WSimdVec4f::Select(bIsCloser, WSimdVec4f(fT), vClosestT);
  }

  out_vClosestDistSqr = vClosestDistSqr;
  out_vClosestT = vClosestT;
  out_fStep = fStep;
  return vClosestPoint;
}

WSimdVec4f WSpline::FindClosestPointOnSegment(WUInt32 uiSegmentIndex, const WSimdVec4f& vPoint, float& out_fT, float& out_fDistanceSquared, float fMaxError /*= 0.1f*/) const
{
  W_ASSERT_DEBUG(uiSegmentIndex < m_ControlPoints.GetCount(), "Invalid segment index");

  auto& cp0 = m_ControlPoints[uiSegmentIndex];
  auto& cp1 = m_ControlPoints[GetCp1Index(uiSegmentIndex)];
  const WSimdVec4f p0 = cp0.m_vPos;
  const WSimdVec4f p1 = cp0.m_vPos + cp0.m_vPosTangentOut;
  const WSimdVec4f p2 = cp1.m_vPos + cp1.m_vPosTangentIn;
  const WSimdVec4f p3 = cp1.m_vPos;
  const WSimdFloat one(1.0f);
  const WSimdFloat maxErrorSqr(fMaxError * fMaxError);

  WSimdVec4f vClosestDistSqr;
  WSimdVec4f vClosestT;
  WSimdFloat fStep;
  WSimdVec4f vClosestPoint = FindIteration(p0, p1, p2, p2, WSimdFloat::MakeZero(), one, vPoint, vClosestDistSqr, vClosestT, fStep);

  constexpr WUInt32 maxIterations = 4;
  for (WUInt32 i = 0; i < maxIterations; ++i)
  {
    const WSimdFloat fClosestT = vClosestT.x();
    const WSimdFloat fMinT = (fClosestT - fStep).Max(WSimdFloat::MakeZero());
    const WSimdFloat fMaxT = (fClosestT + fStep).Min(one);

    const WSimdFloat fClosestTToMinT = (fClosestT - fMinT).Abs();
    const WSimdFloat fClosestTToMaxT = (fClosestT - fMaxT).Abs();
    const WSimdFloat fTestT = fClosestTToMaxT > fClosestTToMinT ? fMaxT : fMinT;
    const WSimdVec4f vTestP = WMath::EvaluateBezierCurve(fTestT, p0, p1, p2, p3);
    const WSimdFloat vTestDistSqr = (vTestP - vClosestPoint).GetLengthSquared<3>();
    if (vTestDistSqr < maxErrorSqr)
      break;

    vClosestPoint = FindIteration(p0, p1, p2, p3, fMinT, fMaxT, vPoint, vClosestDistSqr, vClosestT, fStep);
  }

  out_fT = vClosestT.x();
  out_fDistanceSquared = vClosestDistSqr.x();
  return vClosestPoint;
}

WSimdVec4f WSpline::FindClosestPoint(const WSimdVec4f& vPoint, float& out_fT, float& out_fDistanceSquared, float fMaxError /*= 0.1f*/) const
{
  if (m_ControlPoints.GetCount() < 2)
  {
    out_fT = -1.0f;
    return WSimdVec4f::MakeNaN();
  }

  const WUInt32 uiNumSegments = GetNumSegments();
  WTempHybridArray<WSimdBBox, 32> segmentBounds;
  segmentBounds.SetCountUninitialized(uiNumSegments);

  WUInt32 uiClosestSegment = 0;
  float fClosestDistSqr = WMath::MaxValue<float>();
  for (WUInt32 i = 0; i < uiNumSegments; ++i)
  {
    auto& cp0 = m_ControlPoints[i];
    auto& cp1 = m_ControlPoints[GetCp1Index(i)];

    auto& bounds = segmentBounds[i];
    bounds.m_Min = cp0.m_vPos;
    bounds.m_Max = cp0.m_vPos;
    bounds.ExpandToInclude(cp0.m_vPos + cp0.m_vPosTangentOut);
    bounds.ExpandToInclude(cp1.m_vPos + cp1.m_vPosTangentIn);
    bounds.ExpandToInclude(cp1.m_vPos);

    const float fDistSqr = bounds.GetDistanceSquaredTo(vPoint);
    if (fDistSqr < fClosestDistSqr)
    {
      fClosestDistSqr = fDistSqr;
      uiClosestSegment = i;
    }
  }

  fClosestDistSqr = WMath::MaxValue<float>();
  float fClosestT = 0.0f;
  WSimdVec4f vClosestPoint;

  for (WUInt32 i = 0; i < uiNumSegments; ++i)
  {
    WUInt32 uiSegment = (uiClosestSegment + i);
    if (uiSegment >= uiNumSegments)
      uiSegment -= uiNumSegments;

    const float fDistToBoundsSqr = segmentBounds[uiSegment].GetDistanceSquaredTo(vPoint);
    if (fDistToBoundsSqr > fClosestDistSqr)
      continue;

    float fCandidateT = 0.0f;
    float fCandidateDistSqr = 0.0f;
    const WSimdVec4f vCandidate = FindClosestPointOnSegment(uiSegment, vPoint, fCandidateT, fCandidateDistSqr, fMaxError);
    if (fCandidateDistSqr < fClosestDistSqr)
    {
      vClosestPoint = vCandidate;
      fClosestDistSqr = fCandidateDistSqr;
      fClosestT = static_cast<float>(uiSegment) + fCandidateT;
    }
  }

  out_fT = fClosestT;
  out_fDistanceSquared = fClosestDistSqr;
  return vClosestPoint;
}


W_STATICLINK_FILE(Core, Core_Graphics_Implementation_Spline);
