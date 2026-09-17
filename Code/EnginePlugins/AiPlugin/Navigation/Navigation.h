#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

class WDebugRendererContext;

/// Aggregated data by WAiNavigation that should be sufficient to implement a steering behavior.
struct WAiSteeringInfo
{
  WVec3 m_vNextWaypoint;
  float m_fDistanceToWaypoint = 0;
  float m_fArrivalDistance = WMath::HighValue<float>();
  WVec2 m_vDirectionTowardsWaypoint = WVec2::MakeZero();
  WAngle m_AbsRotationTowardsWaypoint = WAngle::MakeZero();
  WAngle m_MaxAbsRotationAfterWaypoint = WAngle::MakeZero();
  // float m_fWaypointCorridorWidth = WMath::HighValue<float>();
};

/// Computes a path through a navigation mesh.
///
/// First call SetNavmesh() and SetQueryFilter().
///
/// When you need a path, call SetCurrentPosition() and SetTargetPosition() to inform the
/// system of the current position and desired target location.
/// Then call Update() once per frame to have it compute the path.
/// Call GetState() to figure out whether a path exists.
/// Use ComputeAllWaypoints() to get an entire path, e.g. for visualization.
/// For steering this is not necessary. Instead use ComputeSteeringInfo() to plan the next step.
/// Apply your steering behavior to your character as desired.
/// Keep calling SetCurrentPosition() and SetTargetPosition() to inform the WAiNavigation of the
/// new state, and keep calling ComputeSteeringInfo() every frame for the updated path.
///
/// If the destination was reached, a completely different path should be computed, or the current
/// path should be canceled, call CancelNavigation().
/// To start a new path search, call SetTargetPosition() again (and Update() every frame).
class W_AIPLUGIN_DLL WAiNavigation final
{
public:
  WAiNavigation();
  ~WAiNavigation();

  enum class State
  {
    Idle,
    StartNewSearch,
    InvalidCurrentPosition,
    InvalidTargetPosition,
    NoPathFound,
    PartialPathSearchLimited, ///< partial: the A* search hit its node/buffer budget. A repath from closer to the target may complete it. The corridor is usable meanwhile, and Update() will auto-repath as the agent nears the partial end (as long as it keeps making progress).
    PartialPathUnreachable,   ///< partial: the search finished but the target is genuinely not reachable. The corridor's end is the closest reachable point - this is final and will not be retried.
    FullPathFound,
    Searching,
  };

  static constexpr WUInt32 MaxPathNodes = 64;
  static constexpr WUInt32 MaxSearchNodes = MaxPathNodes * 8;

  State GetState() const { return m_State; }

  void Update();

  void CancelNavigation();

  void SetCurrentPosition(const WVec3& vPosition);

  /// Sets the desired target location and starts a path search.
  ///
  /// If \a bOptimizeWhenFound is true, the corridor is optimized (topology + visibility) once,
  /// as soon as the path search finishes. Use this to avoid the weird corridor shapes that the
  /// incremental (counter-based) optimization produces right after a repath. The path search is
  /// asynchronous, so the optimization cannot happen inside this call - it happens in a later
  /// Update() when the search completes.
  void SetTargetPosition(const WVec3& vPosition, bool bOptimizeWhenFound = false);
  const WVec3& GetTargetPosition() const;

  /// Immediately optimizes the current path corridor (topology + visibility).
  ///
  /// Only has an effect when a path exists (GetState() == FullPathFound or one of the PartialPath* states).
  /// This is the on-demand counterpart to SetTargetPosition()'s bOptimizeWhenFound flag.
  void OptimizeCurrentPath();

  /// Checks whether \a vPosition lies inside the current path corridor.
  ///
  /// Returns true only if the point is directly above/below one of the corridor polygons and
  /// within \a fHeightTolerance of that polygon's surface. This is different from testing whether
  /// a point is on the navmesh at all, and different from a raycast: it answers "would moving to
  /// this position leave the planned corridor?". Returns false if no path exists.
  bool IsPointInPathCorridor(const WVec3& vPosition, float fHeightTolerance = 0.5f) const;
  void SetNavmesh(WAiNavMesh* pNavmesh);
  void SetQueryFilter(const dtQueryFilter& filter);

  void ComputeAllWaypoints(WDynamicArray<WVec3>& out_waypoints) const;

  void DebugDrawPathCorridor(const WDebugRendererContext& context, WColor tilesColor, float fPolyRenderOffsetZ = 0.1f);
  void DebugDrawPathLine(const WDebugRendererContext& context, WColor straightLineColor, float fLineRenderOffsetZ = 0.2f);
  void DebugDrawState(const WDebugRendererContext& context, const WVec3& vPosition) const;


  /// Returns the height of the navmesh at the current position.
  float GetCurrentElevation() const;

  void ComputeSteeringInfo(WAiSteeringInfo& out_info, const WVec2& vForwardDir, float fMaxLookAhead = 5.0f);

  // in what radius / up / down distance navigation mesh polygons should be searched around a given position
  // this should relate to the character size, ie at least the character radius
  // otherwise a character that barely left the navmesh area may not know where it is, anymore
  float m_fPolySearchRadius = 0.5f;
  float m_fPolySearchUp = 1.5f;
  float m_fPolySearchDown = 1.5f;

  // when a path search is started, all tiles in a rectangle around the start and end point are loaded first
  // this is the amount to increase that rectangle size, to overestimate which sectors may be needed during the path search
  constexpr static float c_fPathSearchBoundary = 10.0f;


private:
  State m_State = State::Idle;

  WVec3 m_vCurrentPosition = WVec3::MakeZero();
  WVec3 m_vTargetPosition = WVec3::MakeZero();

  WUInt8 m_uiCurrentPositionChangedBit : 1;
  WUInt8 m_uiTargetPositionChangedBit : 1;
  WUInt8 m_uiReinitQueryBit : 1;
  WUInt8 m_uiOptimizeWhenFoundBit : 1;

  WAiNavMesh* m_pNavmesh = nullptr;
  dtNavMeshQuery m_Query;
  const dtQueryFilter* m_pFilter = nullptr;
  dtPathCorridor m_PathCorridor;

  dtPolyRef m_PathSearchTargetPoly;
  WVec3 m_vPathSearchTargetPos;

  WUInt8 m_uiOptimizeTopologyCounter = 0;
  WUInt8 m_uiOptimizeVisibilityCounter = 0;

  // Straight-line distance to the target at the last time a budget-limited partial path triggered an
  // automatic repath. Used as an anti-oscillation guard: we only repath again if we have gotten strictly
  // closer to the target since then. Reset to HighValue whenever a fresh search is requested.
  float m_fLastRepathStartDistToTarget = WMath::HighValue<float>();

  // Number of corridor polygons at the moment a budget-limited partial path was established. As the agent
  // advances, the corridor shrinks from the front; comparing the current length against this tells us how
  // much of the partial path has been used up. 0 means "not a budget-limited partial".
  WUInt32 m_uiPartialCorridorInitialLength = 0;

  // Repath a budget-limited partial path once so few corridor polygons remain, regardless of how much of the
  // original corridor that is - this keeps enough runway for the async search to finish before the agent stalls.
  constexpr static WUInt32 c_uiRepathMinPolysRemaining = 10;
  // ...or once at least this fraction of the original corridor has been consumed, so long corridors repath early
  // rather than travelling almost to their (partial) end first. Expressed as the remaining-length divisor.
  constexpr static WUInt32 c_uiRepathConsumedFractionDivisor = 2; // remaining <= initial / 2  ==  half consumed
  // Minimum progress (straight-line, towards the target) required since the last repath to allow another one.
  constexpr static float c_fRepathMinProgress = 1.0f;

  bool UpdatePathSearch();
};
