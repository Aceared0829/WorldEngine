#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>

void WClothSimulator::SimulateCloth(const WTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr WTime tStep = WTime::MakeFromSeconds(1.0 / 60.0);
  const WSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_vSegmentLength.x);

    m_LeftOverTimeStep -= tStep;
  }
}

void WClothSimulator::SimulateStep(const WSimdFloat fDiffSqr, WUInt32 uiMaxIterations, WSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 4)
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

WSimdFloat WClothSimulator::EnforceDistanceConstraint()
{
  WSimdFloat fError = WSimdFloat::MakeZero();

  for (WUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (WUInt32 x = 0; x < m_uiWidth; ++x)
    {
      const WUInt32 idx = (y * m_uiWidth) + x;

      auto& n = m_Nodes[idx];

      if (n.m_bFixed)
        continue;

      const WSimdVec4f posThis = n.m_vPosition;

      if (x > 0)
      {
        const WSimdVec4f pos = m_Nodes[idx - 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, WSimdVec4f(-1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (x + 1 < m_uiWidth)
      {
        const WSimdVec4f pos = m_Nodes[idx + 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, WSimdVec4f(1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (y > 0)
      {
        const WSimdVec4f pos = m_Nodes[idx - m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, WSimdVec4f(0, -1, 0), fError, m_vSegmentLength.y);
      }

      if (y + 1 < m_uiHeight)
      {
        const WSimdVec4f pos = m_Nodes[idx + m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, WSimdVec4f(0, 1, 0), fError, m_vSegmentLength.y);
      }
    }
  }

  return fError;
}

WSimdVec4f WClothSimulator::MoveTowards(const WSimdVec4f posThis, const WSimdVec4f posNext, WSimdFloat factor, const WSimdVec4f fallbackDir, WSimdFloat& inout_fError, WSimdFloat fSegLen)
{
  WSimdVec4f vDir = (posNext - posThis);
  WSimdFloat fLen = vDir.GetLength<3>();

  if (fLen.IsEqual(WSimdFloat::MakeZero(), 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= fSegLen;

  const WSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

void WClothSimulator::UpdateNodePositions(const WSimdFloat tDiffSqr)
{
  const WSimdFloat damping = m_fDampingFactor;
  const WSimdVec4f acceleration = WSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (auto& n : m_Nodes)
  {
    if (n.m_bFixed)
    {
      n.m_vPreviousPosition = n.m_vPosition;
    }
    else
    {
      // this (simple) logic is the so called 'Verlet integration' (+ damping)

      const WSimdVec4f previousPos = n.m_vPosition;

      const WSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

      // instead of using a single global acceleration, this could also use individual accelerations per node
      // this would be needed to affect the rope more localized
      n.m_vPosition += vel + acceleration;
      n.m_vPreviousPosition = previousPos;
    }
  }
}

bool WClothSimulator::HasEquilibrium(WSimdFloat fAllowedMovement) const
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
