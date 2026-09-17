#include <AsteroidsPlugin/Components/AsteroidComponent.h>
#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Collection/CollectionResource.h>
#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

extern const char* szPlayerActions[MaxPlayerActions];

Level::Level()
{
  m_pWorld = nullptr;
}

void Level::SetupLevel(WUniquePtr<WWorld> pWorld)
{
  m_pWorld = std::move(pWorld);
  W_LOCK(m_pWorld->GetWriteMarker());

  // Load the collection that holds all assets and allows us to access them with nice names
  {
    m_hAssetCollection = WResourceManager::LoadResource<WCollectionResource>("{ c475e948-2e1d-4af0-b69b-d7c0bbad9130 }");

    WResourceLock<WCollectionResource> pCollection(m_hAssetCollection, WResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }

  // Lights
  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation = WQuat::MakeFromAxisAndAngle(WVec3(0.0f, 1.0f, 0.0f), -WAngle::MakeFromDegree(120.0f));

    WGameObject* pObj;
    m_pWorld->CreateObject(obj, pObj);

    // point and spot lights won't work with the orthographic camera
    WDirectionalLightComponent* pDirLight;
    WDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    WAmbientLightComponent* pAmbLight;
    WAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }

  for (WInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);

  for (WInt32 iAsteroid = 0; iAsteroid < MaxAsteroids; ++iAsteroid)
    CreateAsteroid();
}

void Level::UpdatePlayerInput(WInt32 iPlayer)
{
  float fVal = 0.0f;

  WGameObject* pShip = nullptr;
  if (!m_pWorld->TryGetObject(m_hPlayerShips[iPlayer], pShip))
    return;

  ShipComponent* pShipComponent = nullptr;
  if (!pShip->TryGetComponentOfBaseType(pShipComponent))
    return;

  WVec3 vVelocity(0.0f);

  WStringBuilder sControls[MaxPlayerActions];

  for (WInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    sControls[iAction].SetFormat("Player{0}_{1}", iPlayer, szPlayerActions[iAction]);


  if (WInputManager::GetInputActionState("Game", sControls[0].GetData(), &fVal) != WKeyState::Up)
  {
    vVelocity += WVec3(0, 1, 0) * fVal;
  }

  if (WInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != WKeyState::Up)
  {
    vVelocity += WVec3(0, -1, 0) * fVal;
  }

  if (WInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != WKeyState::Up)
  {
    vVelocity += WVec3(-1, 0, 0) * fVal;
  }

  if (WInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != WKeyState::Up)
  {
    vVelocity += WVec3(1, 0, 0) * fVal;
  }

  if (WInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != WKeyState::Up)
  {
    WQuat qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(3.0f * fVal * 60.0f));

    WQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (WInputManager::GetInputActionState("Game", sControls[5].GetData(), &fVal) != WKeyState::Up)
  {
    WQuat qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(-3.0f * fVal * 60.0f));

    WQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (!vVelocity.IsZero())
    pShipComponent->AddExternalForce(vVelocity * 15000.0f);

  if (WInputManager::GetInputActionState("Game", sControls[6].GetData(), &fVal) != WKeyState::Up)
    pShipComponent->SetIsShooting(true);
  else
    pShipComponent->SetIsShooting(false);
}

void Level::CreatePlayerShip(WInt32 iPlayer)
{
  // create one game object for the ship
  // then attach a ship component to that object

  WGameObject* pShipObject = nullptr;

  {
    WGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_LocalPosition.x = -15 + iPlayer * 5.0f;
    m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc, pShipObject);
  }

  {
    // add a sub-object to place the ship mesh at the proper position
    WGameObject* pShipMeshObj = nullptr;

    {
      WGameObjectDesc desc;
      desc.m_hParent = pShipObject->GetHandle();
      desc.m_LocalPosition.x = -0.5f;
      m_pWorld->CreateObject(desc, pShipMeshObj);
    }

    WMeshComponent* pMeshComponent = nullptr;
    WMeshComponent::CreateComponent(pShipMeshObj, pMeshComponent);

    pMeshComponent->SetMesh(WResourceManager::LoadResource<WMeshResource>("ShipMesh"));

    // this only works because the materials are part of the Asset Collection and get a name like this from there
    // otherwise we would need to have the GUIDs of the 4 different material assets available
    WStringBuilder sMaterialName;
    sMaterialName.SetFormat("MaterialPlayer{0}", iPlayer + 1);
    pMeshComponent->SetMaterial(0, WResourceManager::LoadResource<WMaterialResource>(sMaterialName));
  }
  {
    ShipComponent* pShipComponent = nullptr;
    WComponentHandle hShipComponent = ShipComponent::CreateComponent(pShipObject, pShipComponent);

    pShipComponent->m_iPlayerIndex = iPlayer;
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    CollidableComponent::CreateComponent(pShipObject, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = 1.0f;
  }
}

void Level::CreateAsteroid()
{
  WGameObjectDesc desc;
  desc.m_bDynamic = true;
  desc.m_LocalPosition.x = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;
  desc.m_LocalPosition.y = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;

  desc.m_LocalScaling = WVec3(1.0f + ((rand() % 1000) / 999.0f));

  WGameObject* pGameObject = nullptr;
  m_pWorld->CreateObject(desc, pGameObject);

  {
    WMeshComponent* pMeshComponent = nullptr;
    WMeshComponent::CreateComponent(pGameObject, pMeshComponent);

    pMeshComponent->SetMesh(WResourceManager::LoadResource<WMeshResource>("AsteroidMesh"));
  }
  {
    AsteroidComponent* pAsteroidComponent = nullptr;
    AsteroidComponent::CreateComponent(pGameObject, pAsteroidComponent);
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    CollidableComponent::CreateComponent(pGameObject, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = desc.m_LocalScaling.x;
  }
}
