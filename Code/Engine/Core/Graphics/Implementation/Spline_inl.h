
W_ALWAYS_INLINE void WSpline::ControlPoint::SetPosition(const WSimdVec4f& vPos)
{
  m_vPos = vPos;
}

W_ALWAYS_INLINE WSplineTangentMode::Enum WSpline::ControlPoint::GetTangentModeIn() const
{
  float w = m_vPosTangentIn.w();
  return static_cast<WSplineTangentMode::Enum>(w);
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetTangentModeIn(WSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentIn.SetW(w);
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetTangentIn(const WSimdVec4f& vTangent, WSplineTangentMode::Enum mode /*= WSplineTangentMode::Custom*/)
{
  m_vPosTangentIn = vTangent;
  SetTangentModeIn(mode);
}

W_ALWAYS_INLINE WSplineTangentMode::Enum WSpline::ControlPoint::GetTangentModeOut() const
{
  float w = m_vPosTangentOut.w();
  return static_cast<WSplineTangentMode::Enum>(w);
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetTangentModeOut(WSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentOut.SetW(w);
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetTangentOut(const WSimdVec4f& vTangent, WSplineTangentMode::Enum mode /*= WSplineTangentMode::Custom*/)
{
  m_vPosTangentOut = vTangent;
  SetTangentModeOut(mode);
}

W_ALWAYS_INLINE WAngle WSpline::ControlPoint::GetRoll() const
{
  return WAngle::MakeFromRadian(m_vUpDirAndRoll.w());
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetRoll(WAngle roll)
{
  m_vUpDirAndRoll.SetW(roll);
}

W_ALWAYS_INLINE void WSpline::ControlPoint::SetScale(const WSimdVec4f& vScale)
{
  m_vScale = vScale;
  m_vScaleTangentIn.SetZero();
  m_vScaleTangentOut.SetZero();
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WUInt32 WSpline::GetNumControlPoints() const
{
  return m_ControlPoints.GetCount();
}

W_ALWAYS_INLINE WUInt32 WSpline::GetNumSegments() const
{
  const WUInt32 uiNumPoints = m_ControlPoints.GetCount();
  return m_bClosed ? uiNumPoints : (WMath::Max(uiNumPoints, 1u) - 1);
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluatePosition(float fT) const
{
  WUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluatePosition(uiCp0, fT);
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluatePosition(WUInt32 uiCp0, const WSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return WSimdVec4f::MakeZero();

  const WUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluatePosition(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluateDerivative(float fT) const
{
  WUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluateDerivative(uiCp0, fT);
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluateDerivative(WUInt32 uiCp0, const WSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return WSimdVec4f::MakeZero();

  const WUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateDerivative(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluateUpDirection(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return WSimdVec4f::MakeZero();

  WUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const WUInt32 uiCp1 = GetCp1Index(uiCp0);

  WSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT, forwardDir, rightDir, upDir);

  return upDir;
}

W_FORCE_INLINE WSimdVec4f WSpline::EvaluateScale(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return WSimdVec4f::MakeZero();

  WUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const WUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateScale(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

W_ALWAYS_INLINE float WSpline::ClampAndSplitT(float fT, WUInt32& out_uiIndex) const
{
  float fNumPoints = static_cast<float>(m_ControlPoints.GetCount());
  if (m_bClosed)
  {
    fT = (fT < 0.0f || fT >= fNumPoints) ? 0.0f : fT;
  }
  else
  {
    fT = WMath::Clamp(fT, 0.0f, fNumPoints - 1.0f);
  }

  const float fIndex = WMath::Floor(fT);
  out_uiIndex = static_cast<WUInt32>(fIndex);
  return fT - fIndex;
}

W_ALWAYS_INLINE WUInt32 WSpline::GetCp1Index(WUInt32 uiCp0) const
{
  return (uiCp0 + 1 < m_ControlPoints.GetCount()) ? uiCp0 + 1 : 0;
}

W_ALWAYS_INLINE WSimdVec4f WSpline::EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const
{
  return WMath::EvaluateBezierCurve(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

W_ALWAYS_INLINE WSimdVec4f WSpline::EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const
{
  return WMath::EvaluateBezierCurveDerivative(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

W_ALWAYS_INLINE void WSpline::EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT, WSimdVec4f& out_forwardDir, WSimdVec4f& out_rightDir, WSimdVec4f& out_upDir) const
{
  WSimdVec4f upDir = WMath::EvaluateBezierCurve(fT, cp0.m_vUpDirAndRoll, cp0.m_vUpDirAndRoll + cp0.m_vUpDirTangentOut, cp1.m_vUpDirAndRoll + cp1.m_vUpDirTangentIn, cp1.m_vUpDirAndRoll);

  out_forwardDir = EvaluateDerivative(cp0, cp1, fT);
  out_forwardDir.NormalizeIfNotZero<3>(WSimdVec4f(1, 0, 0));

  out_rightDir = upDir.CrossRH(out_forwardDir).GetNormalized<3>();
  out_upDir = out_forwardDir.CrossRH(out_rightDir).GetNormalized<3>();
}

W_ALWAYS_INLINE WSimdVec4f WSpline::EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const
{
  return WMath::EvaluateBezierCurve(fT, cp0.m_vScale, cp0.m_vScale + cp0.m_vScaleTangentOut, cp1.m_vScale + cp1.m_vScaleTangentIn, cp1.m_vScale);
}
