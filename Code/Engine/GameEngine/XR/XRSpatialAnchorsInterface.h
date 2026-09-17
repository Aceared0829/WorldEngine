#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/Types/Id.h>
#include <GameEngine/GameEngineDLL.h>

using WXRSpatialAnchorID = WGenericId<32, 16>;

/// XR spatial anchors interface.
///
/// Aquire interface via WSingletonRegistry::GetSingletonInstance<WXRSpatialAnchorsInterface>().
class WXRSpatialAnchorsInterface
{
public:
  /// Creates a spatial anchor at the given world space position.
  /// Returns an invalid handle if anchors can't be created right now. Retry next frame.
  virtual WXRSpatialAnchorID CreateAnchor(const WTransform& globalTransform) = 0;

  /// Destroys a previously created anchor.
  virtual WResult DestroyAnchor(WXRSpatialAnchorID id) = 0;

  /// Tries to resolve the anchor position. Can fail of the anchor is invalid or tracking is
  /// currently lost.
  virtual WResult TryGetAnchorTransform(WXRSpatialAnchorID id, WTransform& out_globalTransform) = 0;
};
