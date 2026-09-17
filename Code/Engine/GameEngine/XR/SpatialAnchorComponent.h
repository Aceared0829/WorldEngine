#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

using WSpatialAnchorComponentManager = WComponentManagerSimple<class WSpatialAnchorComponent, WComponentUpdateType::WhenSimulating>;

class W_GAMEENGINE_DLL WSpatialAnchorComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WSpatialAnchorComponent, WComponent, WSpatialAnchorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WSpatialAnchorComponent

public:
  WSpatialAnchorComponent();
  ~WSpatialAnchorComponent();

  /// Attempts to create a new anchor at the given location.
  ///
  /// On failure, the existing anchor will continue to be used.
  /// On success, the new anchor will be used and the new location.
  WResult RecreateAnchorAt(const WTransform& position);

protected:
  void Update();

private:
  WXRSpatialAnchorID m_AnchorID;
};
