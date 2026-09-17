#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// A simple simulator for swinging and hanging cloth.
///
/// Uses Verlet Integration to update the cloth positions from velocities, and the "Jakobsen method" to enforce distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class W_GAMEENGINE_DLL WClothSimulator
{
public:
  struct Node
  {
    /// Whether this node can swing freely or will remain fixed in place.
    bool m_bFixed = false;
    WSimdVec4f m_vPosition = WSimdVec4f::MakeZero();
    WSimdVec4f m_vPreviousPosition = WSimdVec4f::MakeZero();
  };

  /// Resolution of the cloth along X
  WUInt8 m_uiWidth = 32;

  /// Resolution of the cloth along Y
  WUInt8 m_uiHeight = 32;

  /// Overall force acting equally upon all cloth nodes.
  WVec3 m_vAcceleration;

  /// Factor with which all node velocities are damped to reduce swinging.
  float m_fDampingFactor = 0.995f;

  /// The distance along x and y between each neighboring node.
  WVec2 m_vSegmentLength = WVec2(0.1f);

  /// All cloth nodes.
  WDynamicArray<Node, WAlignedAllocatorWrapper> m_Nodes;

  void SimulateCloth(const WTime& diff);
  void SimulateStep(const WSimdFloat fDiffSqr, WUInt32 uiMaxIterations, WSimdFloat fAllowedError);
  bool HasEquilibrium(WSimdFloat fAllowedMovement) const;

private:
  WSimdFloat EnforceDistanceConstraint();
  void UpdateNodePositions(const WSimdFloat tDiffSqr);
  WSimdVec4f MoveTowards(const WSimdVec4f posThis, const WSimdVec4f posNext, WSimdFloat factor, const WSimdVec4f fallbackDir, WSimdFloat& inout_fError, WSimdFloat fSegLen);

  WTime m_LeftOverTimeStep;
};
