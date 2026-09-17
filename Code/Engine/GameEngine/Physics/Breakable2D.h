#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec2.h>
#include <GameEngine/GameEngineDLL.h>

class WRandom;

/// The pattern with which to break the shard
enum class WBreakablePattern
{
  None = 0,
  Radial = W_BIT(0),   ///< A radial pattern, like breaking glass.
  Cellular = W_BIT(1), ///< Voronoi cells, for wood / stone or smaller glass shards.
  All = 0xFF,
};

/// State of a single broken shard.
struct W_GAMEENGINE_DLL WBreakableShard2D
{
  /// Index for shard edges that are unsupported (need to fall off).
  static constexpr WUInt32 LooseEdge = WInvalidIndex;
  /// Index for shard edges that are supported (don't fall off), e.g. because they touch a fixed border.
  static constexpr WUInt32 FixedEdge = WInvalidIndex - 1;

  struct Edge
  {
    WVec2 m_vStartPosition;                  ///< Local position where the edge starts. End point is defined by the next edge.
    WUInt32 m_uiOutsideShardIdx = LooseEdge; ///< Which other shard connects to this edge. Used to determine whether a shard is still supported by other shards.
  };

  /// Whether this shard is already destroyed, and should not be used/displayed any further.
  bool m_bShattered = false;

  /// Whether this shard should be physically simulated.
  bool m_bDynamic = false;

  /// If not yet broken, this mask of WBreakablePattern bits defines which patterns can be used to break it further.
  /// Small pieces may not be broken up further, medium sized ones can only use the Cellular pattern.
  WUInt8 m_uiBreakablePatterns = 0;

  /// Center position and radius, used for culling / proximity detection.
  WVec2 m_vCenterPosition;
  float m_fBoundingRadius = 0.0f;

  /// The edges that make up the convex (!) shape of the shard.
  WHybridArray<Edge, 6> m_Edges;
};

/// A 2-dimensional shape that can be broken up into many pieces (shards) using different break patterns.
///
/// This class handles breaking 2D shapes into convex shards using various patterns.
/// It only manages the geometry of the break patterns and does not perform physics simulation.
/// The resulting shards can be used by physics systems to simulate the broken pieces.
///
/// Key features:
/// - Supports radial and cellular (Voronoi) break patterns
/// - Tracks connections between shards to determine which pieces should fall
/// - Allows progressive breaking with different patterns based on shard size
/// - Provides culling information via bounding circles
///
/// Usage:
/// 1. Create instance and call Initialize()
/// 2. Configure initial unbroken shape via first shard,
/// 3. Call ShatterShard() to break pieces at impact points
/// 4. Use RecalculateDynamic() to determine which pieces should fall
/// 5. Feed shard geometry to physics system for simulation
class W_GAMEENGINE_DLL WBreakable2D
{
public:
  WBreakable2D();
  ~WBreakable2D();

  /// Resets all state back to the default.
  void Clear();

  /// Clears the state and sets up a single shard to begin with.
  /// Afterwards the calling code has to configure that shard's shape.
  void Initialize();

  /// Sets the given shard to 'shattered' (destroyed).
  void RemoveShard(WUInt32 uiShardIdx);

  /// Breaks the given shard further apart, using one of the allowed break patterns.
  ///
  /// \param vShatterPosition The world position where the impact occurred that causes the shard to break
  /// \param fImpactRadius The radius of the impact area - affects how far the break pattern spreads
  /// \param fCellSize For cellular break patterns, defines the approximate size of the resulting shards
  /// \param uiAllowedBreakPatterns A mask of WBreakablePattern bits that defines which patterns may be used
  ///
  /// When a shard is shattered:
  /// * For Radial patterns: Creates a radial break pattern originating from vShatterPosition
  /// * For Cellular patterns: Breaks the shard into roughly equally sized pieces using Voronoi cells
  /// * The shard must have the respective pattern enabled in m_uiBreakablePatterns
  /// * The resulting pieces inherit allowed break patterns based on their size
  /// * Small pieces will have no break patterns enabled (can't break further)
  /// * Medium pieces will only allow cellular patterns
  /// * Large pieces allow all patterns
  void ShatterShard(WUInt32 uiShardIdx, const WVec2& vShatterPosition, WRandom& ref_rng, float fImpactRadius, float fCellSize, WUInt8 uiAllowedBreakPatterns = (WUInt8)WBreakablePattern::All);

  /// Shatters all shards using the cellular pattern and optionally sets them all to dynamic.
  void ShatterAll(float fShardSize, WRandom& ref_rng, bool bMakeAllDynamic);

  /// Calculates which shards are unsupported (not directly or indirectly connected to a fixed edge) and sets them to dynamic.
  void RecalculateDymamic();

  /// The maximum radius of all active shards.
  float m_fMaxRadius = 0.0f;

  /// List of all shards that make up this breakable 2D object
  ///
  /// Each shard represents a convex piece of the overall shape.
  /// - When unbroken, contains just a single shard representing the whole shape
  /// - After breaking, contains multiple shards representing the broken pieces
  /// - Shattered/destroyed shards remain in the list but are marked with m_bShattered=true
  /// - Dynamic shards (m_bDynamic=true) are intended to be physically simulated, but this must be handled by other code
  /// - Each shard's edges define its shape and connections to neighboring shards
  WDeque<WBreakableShard2D> m_Shards;

private:
  struct ClipPlane
  {
    WPlane m_Plane;
    WUInt32 m_uiOutsideShardIdx = WInvalidIndex;
  };

  void ShatterWithRadialPattern(WArrayPtr<const ClipPlane> clipPlanes, const WVec2& vShatterPosition, WRandom& ref_rng, float fImpactRadius);
  bool ShatterWithCellularPattern(WUInt32 uiShardIdx, WArrayPtr<const ClipPlane> clipPlanes, WRandom& ref_rng, float fShardSize);
  WUInt32 AddShard(WArrayPtr<const ClipPlane> clipPlanes, WArrayPtr<WBreakableShard2D::Edge> shape);
  void GenerateRingVertices(WDynamicArray<WVec2>& vertices, const WVec2& vCenter, WArrayPtr<float> radii, const WArrayPtr<WQuat> qRotations);
  void GenerateRingShards(WArrayPtr<const ClipPlane> clipPlanes, WArrayPtr<WVec2> innerVertices, WArrayPtr<WVec2> outerVertices, WArrayPtr<WUInt32> prevShardIDs, WDynamicArray<WUInt32>& out_ShardIDs);
};
