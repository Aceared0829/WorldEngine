#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using WCollectionResourceHandle = WTypedResourceHandle<class WCollectionResource>;

#define MaxPlayers 4
#define MaxAsteroids 30
#define MaxPlayerActions 7

class Level
{
public:
  Level();

  void SetupLevel(WUniquePtr<WWorld> pWorld);
  void UpdatePlayerInput(WInt32 iPlayer);

  WWorld* GetWorld() const { return m_pWorld.Borrow(); }
private:
  void CreatePlayerShip(WInt32 iPlayer);
  void CreateAsteroid();

  WCollectionResourceHandle m_hAssetCollection;
  WUniquePtr<WWorld> m_pWorld;
  WGameObjectHandle m_hPlayerShips[MaxPlayers];
};
