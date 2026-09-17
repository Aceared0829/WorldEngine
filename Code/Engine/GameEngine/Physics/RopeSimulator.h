#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <GameEngine/GameEngineDLL.h>

/// A simple simulator for swinging and hanging ropes.
///
/// Can be used both for interactive rope simulation, as well as to just pre-compute the shape of hanging wires, cables, etc.
/// Uses Verlet Integration to update the rope positions from velocities, and the "Jakobsen method" to enforce
/// rope distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class W_GAMEENGINE_DLL WRopeSimulator
{
public:
  struct Node
  {
    WSimdVec4f m_vPosition = WSimdVec4f::MakeZero();
    WSimdVec4f m_vPreviousPosition = WSimdVec4f::MakeZero();

    // could add per node acceleration
    // could add per node mass
  };

public:
  WRopeSimulator();
  ~WRopeSimulator();

  /// External acceleration, typically gravity or a combination of gravity and wind.
  /// Applied to all rope nodes equally.
  WVec3 m_vAcceleration = WVec3(0, 0, -10);

  /// All the nodes in the rope
  WDynamicArray<Node, WAlignedAllocatorWrapper> m_Nodes;

  /// A factor to dampen velocities to make the rope stop swinging.
  /// Should be between 0.97 (strong damping) and 1.0 (no damping).
  float m_fDampingFactor = 0.995f;

  /// How long each rope segment (between two nodes) should be.
  float m_fSegmentLength = 0.1f;

  bool m_bFirstNodeIsFixed = true;
  bool m_bLastNodeIsFixed = true;

  void SimulateRope(const WTime& diff);
  void SimulateStep(const WSimdFloat fDiffSqr, WUInt32 uiMaxIterations, WSimdFloat fAllowedError);
  void SimulateTillEquilibrium(WSimdFloat fAllowedMovement = 0.005f, WUInt32 uiMaxIterations = 1000);
  bool HasEquilibrium(WSimdFloat fAllowedMovement) const;
  float GetTotalLength() const;
  WSimdVec4f GetPositionAtLength(float fLength) const;

private:
  WSimdFloat EnforceDistanceConstraint();
  void UpdateNodePositions(const WSimdFloat tDiffSqr);
  WSimdVec4f MoveTowards(const WSimdVec4f posThis, const WSimdVec4f posNext, WSimdFloat factor, const WSimdVec4f fallbackDir, WSimdFloat& inout_fError);

  WTime m_LeftOverTimeStep;
};
