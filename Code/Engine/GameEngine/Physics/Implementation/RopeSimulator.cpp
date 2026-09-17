#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/RopeSimulator.h>

WRopeSimulator::WRopeSimulator() = default;
WRopeSimulator::~WRopeSimulator() = default;

void WRopeSimulator::SimulateRope(const WTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr WTime tStep = WTime::MakeFromSeconds(1.0 / 60.0);
  const WSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());
  const WSimdFloat fAllowedError = m_fSegmentLength;

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, fAllowedError);

    m_LeftOverTimeStep -= tStep;
  }
}

void WRopeSimulator::SimulateStep(const WSimdFloat fDiffSqr, WUInt32 uiMaxIterations, WSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 2)
    return;

  UpdateNodePositions(fDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (WUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const WSimdFloat fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

void WRopeSimulator::SimulateTillEquilibrium(WSimdFloat fAllowedMovement, WUInt32 uiMaxIterations)
{
  constexpr WTime tStep = WTime::MakeFromSeconds(1.0 / 60.0);
  WSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  WUInt8 uiInEquilibrium = 0;

  while (uiInEquilibrium < 100 && uiMaxIterations > 0)
  {
    --uiMaxIterations;

    SimulateStep(tStepSqr, 32, m_fSegmentLength);
    uiInEquilibrium++;

    if (!HasEquilibrium(fAllowedMovement))
    {
      uiInEquilibrium = 0;
    }
  }
}

bool WRopeSimulator::HasEquilibrium(WSimdFloat fAllowedMovement) const
{
  const WSimdFloat fErrorSqr = fAllowedMovement * fAllowedMovement;

  for (const auto& n : m_Nodes)
  {
    if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared<3>() > fErrorSqr)
    {
      return false;
    }
  }

  return true;
}

float WRopeSimulator::GetTotalLength() const
{
  if (m_Nodes.GetCount() <= 1)
    return 0.0f;

  float len = 0;

  WSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (WUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const WSimdVec4f cur = m_Nodes[i].m_vPosition;

    len += (cur - prev).GetLength<3>();

    prev = cur;
  }

  return len;
}

WSimdVec4f WRopeSimulator::GetPositionAtLength(float fLength) const
{
  if (m_Nodes.IsEmpty())
    return WSimdVec4f::MakeZero();

  WSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (WUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const WSimdVec4f cur = m_Nodes[i].m_vPosition;

    const WSimdVec4f dir = cur - prev;
    const float dist = dir.GetLength<3>();

    if (fLength <= dist)
    {
      const float interpolate = fLength / dist;
      return prev + dir * interpolate;
    }

    fLength -= dist;
    prev = cur;
  }

  return m_Nodes.PeekBack().m_vPosition;
}

WSimdVec4f WRopeSimulator::MoveTowards(const WSimdVec4f posThis, const WSimdVec4f posNext, WSimdFloat factor, const WSimdVec4f fallbackDir, WSimdFloat& inout_fError)
{
  WSimdVec4f vDir = (posNext - posThis);
  WSimdFloat fLen = vDir.GetLength<3>();

  if (fLen < m_fSegmentLength)
  {
    return WSimdVec4f::MakeZero();
  }

  vDir /= fLen;
  fLen -= m_fSegmentLength;

  const WSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

WSimdFloat WRopeSimulator::EnforceDistanceConstraint()
{
  // this is the "Jakobsen method" to enforce the distance constraints in each rope node
  // just move each node half the error amount towards the left and right neighboring nodes
  // the ends are either not moved at all (when they are 'attached' to something)
  // or they are moved most of the way
  // this is applied iteratively until the overall error is pretty low

  auto& firstNode = m_Nodes[0];
  auto& lastNode = m_Nodes.PeekBack();

  WSimdFloat fError = WSimdFloat::MakeZero();

  if (!m_bFirstNodeIsFixed)
  {
    const WSimdVec4f posThis = m_Nodes[0].m_vPosition;
    const WSimdVec4f posNext = m_Nodes[1].m_vPosition;

    m_Nodes[0].m_vPosition += MoveTowards(posThis, posNext, 0.75f, WSimdVec4f(0, 0, 1), fError);
  }

  for (WUInt32 i = 1; i < m_Nodes.GetCount() - 1; ++i)
  {
    const WSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const WSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;
    const WSimdVec4f posNext = m_Nodes[i + 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.5f, WSimdVec4f(0, 0, 1), fError);
    m_Nodes[i].m_vPosition += MoveTowards(posThis, posNext, 0.5f, WSimdVec4f(0, 0, -1), fError);
  }

  if (!m_bLastNodeIsFixed)
  {
    const WUInt32 i = m_Nodes.GetCount() - 1;
    const WSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const WSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.75f, WSimdVec4f(0, 0, 1), fError);
  }

  return fError;
}

void WRopeSimulator::UpdateNodePositions(const WSimdFloat tDiffSqr)
{
  const WUInt32 uiFirstNode = m_bFirstNodeIsFixed ? 1 : 0;
  const WUInt32 uiNumNodes = m_bLastNodeIsFixed ? m_Nodes.GetCount() - 1 : m_Nodes.GetCount();

  const WSimdFloat damping = m_fDampingFactor;

  const WSimdVec4f acceleration = WSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (WUInt32 i = uiFirstNode; i < uiNumNodes; ++i)
  {
    // this (simple) logic is the so called 'Verlet integration' (+ damping)

    auto& n = m_Nodes[i];

    const WSimdVec4f previousPos = n.m_vPosition;

    const WSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

    // instead of using a single global acceleration, this could also use individual accelerations per node
    // this would be needed to affect the rope more localized
    n.m_vPosition += vel + acceleration;
    n.m_vPreviousPosition = previousPos;
  }

  if (m_bFirstNodeIsFixed)
  {
    m_Nodes[0].m_vPreviousPosition = m_Nodes[0].m_vPosition;
  }
  if (m_bLastNodeIsFixed)
  {
    m_Nodes.PeekBack().m_vPreviousPosition = m_Nodes.PeekBack().m_vPosition;
  }
}
