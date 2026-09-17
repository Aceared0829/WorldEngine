#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;

using WPlayerStartPointComponentManager = WComponentManager<class WPlayerStartPointComponent, WBlockStorageType::Compact>;

/// Defines a location that the player may start from.
///
/// This component specifies which prefab to use as the player object and parameters to spawn the player object with.
///
/// The component itself has no functionality. It is the WGameState that decides how to utilize player start points.
/// The default game state searches for a start point component and spawns the prefab from there, assuming that the prefab
/// contains all the functionality to make the game playable (e.g. it should contain a main camera, input handling, movement and so on).
///
/// A game state may also ignore the prefab and on the player start point component and only use the location to spawn or relocate the player object.
///
/// It is not mandatory to use a spawn point component. A player object can also be instantiated in a scene just as a regular
/// prefab. However, the editor functionality "Play from here", that allows you to spawn the player object at any location in the
/// scene, relies on spawn points, as the spawn point defines which prefab to use for the player prefab, but the game state
/// can now override the location where to spawn the player.
class W_GAMEENGINE_DLL WPlayerStartPointComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WPlayerStartPointComponent, WComponent, WPlayerStartPointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WPlayerStartPointComponent

public:
  WPlayerStartPointComponent();
  ~WPlayerStartPointComponent();

  void SetPlayerPrefab(const WPrefabResourceHandle& hPrefab);      // [ property ]
  const WPrefabResourceHandle& GetPlayerPrefab() const;            // [ property ]

  const WRangeView<const char*, WUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const WVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, WVariant& out_value) const; // [ property ] (exposed parameter)

  WArrayMap<WHashedString, WVariant> m_Parameters;

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number

protected:
  WPrefabResourceHandle m_hPlayerPrefab;
};
