#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/World.h>

struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;

struct W_GAMEENGINE_DLL WGrabbableItemGrabPoint
{
  WVec3 m_vLocalPosition;
  WQuat m_qLocalRotation;
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WGrabbableItemGrabPoint);

//////////////////////////////////////////////////////////////////////////

using WGrabbableItemComponentManager = WComponentManager<class WGrabbableItemComponent, WBlockStorageType::Compact>;

/// Used to define 'grab points' on an object where a player can pick up and hold the item
///
/// The grabbable item component is typically added to objects with a dynamic physics actor to mark it as an item that can be
/// picked up, and to define the anchor points at which the object can be held.
/// Of course a game can utilize this information without a physical actor and physically holding objects as well.
///
/// Each grab point defines how the object would be oriented when held.
///
/// The component only holds data, it doesn't add any custom behavior. It is the responsibility of other components to use this
/// data in a sensible way.
class W_GAMEENGINE_DLL WGrabbableItemComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WGrabbableItemComponent, WComponent, WGrabbableItemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WGrabbableItemComponent

public:
  WGrabbableItemComponent();
  ~WGrabbableItemComponent();

  void SetDebugShowPoints(bool bShow);                   // [ property ]
  bool GetDebugShowPoints() const;                       // [ property ]

  WDynamicArray<WGrabbableItemGrabPoint> m_GrabPoints; // [ property ]

  static void DebugDrawGrabPoint(const WWorld& world, const WTransform& globalGrabPointTransform);

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(WMsgExtractRenderData& msg) const;
};
