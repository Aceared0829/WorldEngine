#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/Curve1D.h>

WCurve1D::ControlPoint::ControlPoint()
{
  m_Position.SetZero();
  m_LeftTangent.SetZero();
  m_RightTangent.SetZero();
  m_uiOriginalIndex = 0;
}

WCurve1D::WCurve1D()
{
  Clear();
}

void WCurve1D::Clear()
{
  m_fMinX = 0;
  m_fMaxX = 0;
  m_fMinY = 0;
  m_fMaxY = 0;

  m_ControlPoints.Clear();
}

bool WCurve1D::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

WCurve1D::ControlPoint& WCurve1D::AddControlPoint(double x)
{
  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_uiOriginalIndex = static_cast<WUInt16>(m_ControlPoints.GetCount() - 1);
  cp.m_Position.x = x;
  cp.m_Position.y = 0;
  cp.m_LeftTangent.x = -0.1f;
  cp.m_LeftTangent.y = 0.0f;
  cp.m_RightTangent.x = +0.1f;
  cp.m_RightTangent.y = 0.0f;

  return cp;
}

void WCurve1D::QueryExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  ref_fMinx = m_fMinX;
  ref_fMaxx = m_fMaxX;
}

void WCurve1D::QueryExtremeValues(double& ref_fMinVal, double& ref_fMaxVal) const
{
  ref_fMinVal = m_fMinY;
  ref_fMaxVal = m_fMaxY;
}

WUInt32 WCurve1D::GetNumControlPoints() const
{
  return m_ControlPoints.GetCount();
}

void WCurve1D::SortControlPoints()
{
  m_ControlPoints.Sort();

  RecomputeExtents();
}

WInt32 WCurve1D::FindApproxControlPoint(double x) const
{
  WUInt32 uiLowIdx = 0;
  WUInt32 uiHighIdx = m_LinearApproximation.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const WUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    if (m_LinearApproximation[uiMidIdx].x >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (WUInt32 idx = uiLowIdx; idx < uiHighIdx; ++idx)
  {
    if (m_LinearApproximation[idx].x >= x)
    {
      // when m_LinearApproximation[0].x >= x, we want to return -1
      return ((WInt32)idx) - 1;
    }
  }

  // return last index
  return (WInt32)uiHighIdx - 1;
}

double WCurve1D::Evaluate(double x) const
{
  W_ASSERT_DEBUG(!m_LinearApproximation.IsEmpty(), "Cannot evaluate curve without precomputing curve approximation data first. Call CreateLinearApproximation() on curve before calling Evaluate().");

  if (m_LinearApproximation.GetCount() >= 2)
  {
    const WUInt32 numCPs = m_LinearApproximation.GetCount();
    const WInt32 iControlPoint = FindApproxControlPoint(x);

    if (iControlPoint < 0)
    {
      // clamp to left value
      return m_LinearApproximation[0].y;
    }
    else if (WUInt32(iControlPoint) == numCPs - 1)
    {
      // clamp to right value
      return m_LinearApproximation[numCPs - 1].y;
    }
    else
    {
      const double v1 = m_LinearApproximation[iControlPoint].y;
      const double v2 = m_LinearApproximation[iControlPoint + 1].y;

      // interpolate
      double lerpX = x - m_LinearApproximation[iControlPoint].x;
      const double len = (m_LinearApproximation[iControlPoint + 1].x - m_LinearApproximation[iControlPoint].x);

      if (len <= 0)
        lerpX = 0;
      else
        lerpX /= len; // TODO remove division ?

      return WMath::Lerp(v1, v2, lerpX);
    }
  }
  else if (m_LinearApproximation.GetCount() == 1)
  {
    return m_LinearApproximation[0].y;
  }

  return 0;
}

double WCurve1D::ConvertNormalizedPos(double fPos) const
{
  double fMin, fMax;
  QueryExtents(fMin, fMax);

  return WMath::Lerp(fMin, fMax, fPos);
}


double WCurve1D::NormalizeValue(double value) const
{
  double fMin, fMax;
  QueryExtremeValues(fMin, fMax);

  if (fMin >= fMax)
    return 0;

  return (value - fMin) / (fMax - fMin);
}

WUInt64 WCurve1D::GetHeapMemoryUsage() const
{
  return m_ControlPoints.GetHeapMemoryUsage();
}

void WCurve1D::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 4;

  inout_stream << uiVersion;

  const WUInt32 numCp = m_ControlPoints.GetCount();

  inout_stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    inout_stream << cp.m_Position;
    inout_stream << cp.m_LeftTangent;
    inout_stream << cp.m_RightTangent;
    inout_stream << cp.m_TangentModeRight;
    inout_stream << cp.m_TangentModeLeft;
  }
}

void WCurve1D::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  W_ASSERT_DEV(uiVersion <= 4, "Incorrect version '{0}' for WCurve1D", uiVersion);

  WUInt32 numCp = 0;

  inout_stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  if (uiVersion <= 2)
  {
    for (auto& cp : m_ControlPoints)
    {
      WVec2 pos;
      inout_stream >> pos;
      cp.m_Position.Set(pos.x, pos.y);

      if (uiVersion >= 2)
      {
        inout_stream >> cp.m_LeftTangent;
        inout_stream >> cp.m_RightTangent;
      }
    }
  }
  else
  {
    for (auto& cp : m_ControlPoints)
    {
      inout_stream >> cp.m_Position;
      inout_stream >> cp.m_LeftTangent;
      inout_stream >> cp.m_RightTangent;

      if (uiVersion >= 4)
      {
        inout_stream >> cp.m_TangentModeRight;
        inout_stream >> cp.m_TangentModeLeft;
      }
    }
  }
}

void WCurve1D::CreateLinearApproximation(double fMaxError /*= 0.01f*/, WUInt8 uiMaxSubDivs /*= 8*/)
{
  m_LinearApproximation.Clear();

  /// \todo Since we do this, we actually don't need the linear approximation anymore and could just evaluate the full curve
  ApplyTangentModes();

  ClampTangents();

  if (m_ControlPoints.IsEmpty())
  {
    m_LinearApproximation.PushBack(WVec2d::MakeZero());
    return;
  }

  for (WUInt32 i = 1; i < m_ControlPoints.GetCount(); ++i)
  {
    W_ASSERT_DEBUG(m_ControlPoints[i - 1].m_Position.x <= m_ControlPoints[i].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    double fMinY, fMaxY;
    ApproximateMinMaxValues(m_ControlPoints[i - 1], m_ControlPoints[i], fMinY, fMaxY);

    const double rangeY = WMath::Max(0.1, fMaxY - fMinY);
    const double fMaxErrorY = fMaxError * rangeY;
    const double fMaxErrorX = (m_ControlPoints[i].m_Position.x - m_ControlPoints[i - 1].m_Position.x) * fMaxError;


    m_LinearApproximation.PushBack(m_ControlPoints[i - 1].m_Position);

    ApproximateCurve(m_ControlPoints[i - 1].m_Position,
      m_ControlPoints[i - 1].m_Position + WVec2d(m_ControlPoints[i - 1].m_RightTangent.x, m_ControlPoints[i - 1].m_RightTangent.y),
      m_ControlPoints[i].m_Position + WVec2d(m_ControlPoints[i].m_LeftTangent.x, m_ControlPoints[i].m_LeftTangent.y), m_ControlPoints[i].m_Position,
      fMaxErrorX, fMaxErrorY, uiMaxSubDivs);
  }

  m_LinearApproximation.PushBack(m_ControlPoints.PeekBack().m_Position);

  RecomputeLinearApproxExtremes();
}

void WCurve1D::RecomputeExtents()
{
  m_fMinX = WMath::MaxValue<float>();
  m_fMaxX = -WMath::MaxValue<float>();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = WMath::Min(m_fMinX, cp.m_Position.x);
    m_fMaxX = WMath::Max(m_fMaxX, cp.m_Position.x);

    // ignore X values that could go outside the control point range due to Bezier curve interpolation
    // we just assume the curve is always restricted along X by the CPs

    // m_fMinX = WMath::Min(m_fMinX, cp.m_Position.x + cp.m_LeftTangent.x);
    // m_fMaxX = WMath::Max(m_fMaxX, cp.m_Position.x + cp.m_LeftTangent.x);

    // m_fMinX = WMath::Min(m_fMinX, cp.m_Position.x + cp.m_RightTangent.x);
    // m_fMaxX = WMath::Max(m_fMaxX, cp.m_Position.x + cp.m_RightTangent.x);
  }
}


void WCurve1D::RecomputeLinearApproxExtremes()
{
  m_fMinY = WMath::MaxValue<float>();
  m_fMaxY = -WMath::MaxValue<float>();

  for (const auto& cp : m_LinearApproximation)
  {
    m_fMinY = WMath::Min(m_fMinY, cp.y);
    m_fMaxY = WMath::Max(m_fMaxY, cp.y);
  }
}

void WCurve1D::ApproximateMinMaxValues(const ControlPoint& lhs, const ControlPoint& rhs, double& fMinY, double& fMaxY)
{
  fMinY = WMath::Min(lhs.m_Position.y, rhs.m_Position.y);
  fMaxY = WMath::Max(lhs.m_Position.y, rhs.m_Position.y);

  fMinY = WMath::Min(fMinY, lhs.m_Position.y + lhs.m_RightTangent.y);
  fMaxY = WMath::Max(fMaxY, lhs.m_Position.y + lhs.m_RightTangent.y);

  fMinY = WMath::Min(fMinY, rhs.m_Position.y + rhs.m_LeftTangent.y);
  fMaxY = WMath::Max(fMaxY, rhs.m_Position.y + rhs.m_LeftTangent.y);
}

void WCurve1D::ApproximateCurve(
  const WVec2d& p0, const WVec2d& p1, const WVec2d& p2, const WVec2d& p3, double fMaxErrorX, double fMaxErrorY, WInt32 iSubDivLeft)
{
  const WVec2d cubicCenter = WMath::EvaluateBezierCurve(0.5, p0, p1, p2, p3);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.0f, p0, 0.5, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft);

  // always insert the center point
  // with an S curve the cubicCenter and the linearCenter can be identical even though the rest of the curve is absolutely not linear
  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.5, cubicCenter, 1.0, p3, fMaxErrorX, fMaxErrorY, iSubDivLeft);
}

void WCurve1D::ApproximateCurvePiece(const WVec2d& p0, const WVec2d& p1, const WVec2d& p2, const WVec2d& p3, double tLeft, const WVec2d& pLeft,
  double tRight, const WVec2d& pRight, double fMaxErrorX, double fMaxErrorY, WInt32 iSubDivLeft)
{
  // this is a safe guard
  if (iSubDivLeft <= 0)
    return;

  const double tCenter = WMath::Lerp(tLeft, tRight, 0.5);

  const WVec2d cubicCenter = WMath::EvaluateBezierCurve(tCenter, p0, p1, p2, p3);
  const WVec2d linearCenter = WMath::Lerp(pLeft, pRight, 0.5);

  // check whether the linear interpolation between pLeft and pRight would already result in a good enough approximation
  // if not, subdivide the curve further

  const double fThisErrorX = WMath::Abs(cubicCenter.x - linearCenter.x);
  const double fThisErrorY = WMath::Abs(cubicCenter.y - linearCenter.y);

  if (fThisErrorX < fMaxErrorX && fThisErrorY < fMaxErrorY)
    return;

  ApproximateCurvePiece(p0, p1, p2, p3, tLeft, pLeft, tCenter, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);

  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, tCenter, cubicCenter, tRight, pRight, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);
}

void WCurve1D::ClampTangents()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (WUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    auto& tCP = m_ControlPoints[i];
    const auto& pCP = m_ControlPoints[i - 1];
    const auto& nCP = m_ControlPoints[i + 1];

    WVec2d lpt = tCP.m_Position + WVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    WVec2d rpt = tCP.m_Position + WVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);

    lpt.x = WMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);
    rpt.x = WMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const WVec2d tangentL = lpt - tCP.m_Position;
    const WVec2d tangentR = rpt - tCP.m_Position;

    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // first CP
  {
    auto& tCP = m_ControlPoints[0];
    const auto& nCP = m_ControlPoints[1];

    WVec2d rpt = tCP.m_Position + WVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);
    rpt.x = WMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const WVec2d tangentR = rpt - tCP.m_Position;
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // last CP
  {
    auto& tCP = m_ControlPoints[m_ControlPoints.GetCount() - 1];
    const auto& pCP = m_ControlPoints[m_ControlPoints.GetCount() - 2];

    WVec2d lpt = tCP.m_Position + WVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    lpt.x = WMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);

    const WVec2d tangentL = lpt - tCP.m_Position;
    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
  }
}

void WCurve1D::ApplyTangentModes()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (WUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    const auto& cp = m_ControlPoints[i];

    W_ASSERT_DEBUG(cp.m_Position.x >= m_ControlPoints[i - 1].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");
    W_ASSERT_DEBUG(m_ControlPoints[i + 1].m_Position.x >= cp.m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    if (cp.m_TangentModeLeft == WCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == WCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == WCurveTangentMode::Auto)
      MakeAutoTangentLeft(i);

    if (cp.m_TangentModeRight == WCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == WCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == WCurveTangentMode::Auto)
      MakeAutoTangentRight(i);
  }

  // first CP
  {
    const WUInt32 i = 0;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeRight == WCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == WCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == WCurveTangentMode::Auto)
      MakeLinearTangentRight(i); // note: first point will always be linear in auto mode
  }

  // last CP
  {
    const WUInt32 i = m_ControlPoints.GetCount() - 1;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeLeft == WCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == WCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == WCurveTangentMode::Auto)
      MakeLinearTangentLeft(i); // note: last point will always be linear in auto mode
  }
}

void WCurve1D::MakeFixedLengthTangentLeft(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const double lengthL = (pCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthL >= -0.0000001)
  {
    tCP.m_LeftTangent.SetZero();
  }
  else
  {
    const double tLen = WMath::Min((double)tCP.m_LeftTangent.x, -0.001);

    const double fNormL = lengthL / tLen;
    tCP.m_LeftTangent.x = (float)lengthL;
    tCP.m_LeftTangent.y *= (float)fNormL;
  }
}

void WCurve1D::MakeFixedLengthTangentRight(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double lengthR = (nCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthR <= 0.0000001)
  {
    tCP.m_RightTangent.SetZero();
  }
  else
  {
    const double tLen = WMath::Max((double)tCP.m_RightTangent.x, 0.001);

    const double fNormR = lengthR / tLen;
    tCP.m_RightTangent.x = (float)lengthR;
    tCP.m_RightTangent.y *= (float)fNormR;
  }
}

void WCurve1D::MakeLinearTangentLeft(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const WVec2d tangent = (pCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_LeftTangent.Set((float)tangent.x, (float)tangent.y);
}

void WCurve1D::MakeLinearTangentRight(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const WVec2d tangent = (nCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

void WCurve1D::MakeAutoTangentLeft(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const WVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const WVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const WVec2d tangent = WMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_LeftTangent.Set(-(float)tangent.x, -(float)tangent.y);
}

void WCurve1D::MakeAutoTangentRight(WUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const WVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const WVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const WVec2d tangent = WMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

WResult WCurve1D::GenerateSampledCurve(WUInt32 uiNumSamples, WSampledCurve1D& out_sampledCurve)
{
  if (uiNumSamples < 2)
    return W_FAILURE;

  SortControlPoints();
  CreateLinearApproximation();

  double fMinX, fMaxX;
  QueryExtents(fMinX, fMaxX);
  fMinX = WMath::Min(fMinX, 0.0);
  fMaxX = WMath::Max(fMaxX, 1.0);
  const float fStep = static_cast<float>(fMaxX - fMinX) / static_cast<float>(uiNumSamples - 1);

  WDynamicArray<float> samples;
  samples.SetCount(uiNumSamples);
  for (WUInt32 i = 0; i < uiNumSamples; ++i)
  {
    float x = static_cast<float>(fMinX + fStep * i);
    samples[i] = static_cast<float>(Evaluate(x));
  }

  out_sampledCurve.m_Samples = std::move(samples);
  out_sampledCurve.m_fMinX = static_cast<float>(fMinX);
  out_sampledCurve.m_fMaxX = static_cast<float>(fMaxX);
  return W_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSampledCurve1D, 1, WRTTIDefaultAllocator<WSampledCurve1D>)
W_END_DYNAMIC_REFLECTED_TYPE;

bool WSampledCurve1D::operator==(const WSampledCurve1D& rhs) const
{
  return m_fMinX == rhs.m_fMinX && m_fMaxX == rhs.m_fMaxX && m_Samples == rhs.m_Samples;
}

void WSampledCurve1D::Save(WStreamWriter& inout_stream) const
{
  inout_stream.WriteArray(m_Samples).IgnoreResult();
  inout_stream << m_fMinX;
  inout_stream << m_fMaxX;
}

WResult WSampledCurve1D::Load(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Samples));
  inout_stream >> m_fMinX;
  inout_stream >> m_fMaxX;

  return W_SUCCESS;
}


W_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_Curve1D);
