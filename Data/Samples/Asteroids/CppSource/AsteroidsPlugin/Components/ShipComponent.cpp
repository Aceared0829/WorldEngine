#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Messages/SetColorMessage.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(ShipComponent, 1, WComponentMode::Dynamic);
W_END_COMPONENT_TYPE
// clang-format on

WCVarFloat CVar_MaxAmmo("g_MaxAmmo", 20.0f, WCVarFlags::Default, "How much ammo a ship can store");
WCVarFloat CVar_MaxHealth("g_MaxHealth", 30.0f, WCVarFlags::Default, "How much health a ship can have");
WCVarFloat CVar_ProjectileSpeed("g_ProjectileSpeed", 100.0f, WCVarFlags::Default, "Projectile fly speed");
WCVarFloat CVar_ProjectileAmmoPerShot("g_AmmoPerShot", 0.2f, WCVarFlags::Default, "Ammo used up per shot");
WCVarFloat CVar_ShotDelay("g_ShotDelay", 1.0f / 20.0f, WCVarFlags::Default, "Delay between each shot");

ShipComponent::ShipComponent()
{
  m_fAmmunition = CVar_MaxAmmo / 2.0f;
  m_fHealth = CVar_MaxHealth;
}

void ShipComponent::AddExternalForce(const WVec3& vForce)
{
  m_vExternalForce += vForce;
}

void ShipComponent::SetIsShooting(bool b)
{
  m_bIsShooting = b;
}

void ShipComponent::Explode()
{
  const WUInt32 uiNumSparks = 100;
  const float fSparksSpeed = 25.0f;

  const float fSteps = 360.0f / uiNumSparks;

  for (WInt32 i = 0; i < uiNumSparks; ++i)
  {
    WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(i * fSteps));

    {
      WGameObjectDesc desc;
      desc.m_bDynamic = true;
      desc.m_LocalPosition = GetOwner()->GetLocalPosition();
      desc.m_LocalRotation = qRot * GetOwner()->GetLocalRotation();

      WGameObject* pProjectile = nullptr;
      GetWorld()->CreateObject(desc, pProjectile);

      ProjectileComponent* pProjectileComponent = nullptr;
      WComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(pProjectile, pProjectileComponent);

      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_fSpeed = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(1.0, 2.0) * fSparksSpeed;
      pProjectileComponent->m_fDoesDamage = 0.75f;

      // ProjectileMesh
      {
        WMeshComponent* pMeshComponent = nullptr;
        WMeshComponent::CreateComponent(pProjectile, pMeshComponent);

        pMeshComponent->SetMesh(WResourceManager::LoadResource<WMeshResource>("ProjectileMesh"));

        // this only works because the materials are part of the Asset Collection and get a name like this from there
        // otherwise we would need to have the GUIDs of the 4 different material assets available
        WStringBuilder sMaterialName;
        sMaterialName.SetFormat("MaterialPlayer{0}", m_iPlayerIndex + 1);
        pMeshComponent->SetMaterial(0, WResourceManager::LoadResource<WMaterialResource>(sMaterialName));
      }
    }
  }
}

void ShipComponent::Update()
{
  if (!IsAlive())
  {
    Explode();

    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    return;
  }


  const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  // slow down the ship over time
  m_vVelocity *= WMath::Pow(0.5f, tDiff.AsFloatInSeconds());
  // apply the external force to the ship's velocity
  m_vVelocity += tDiff.AsFloatInSeconds() * m_vExternalForce;
  // reset the forces, they will be re-added during the next update
  m_vExternalForce.SetZero();

  WVec3 vTravelDist = m_vVelocity * tDiff.AsFloatInSeconds();

  // if the ship is at least slightly moving, do collision checks
  if (!m_vVelocity.IsZero(0.001f))
  {
    CollidableComponentManager* pCollidableManager = GetWorld()->GetOrCreateComponentManager<CollidableComponentManager>();

    for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
    {
      if (!it->IsActiveAndSimulating())
        continue;

      CollidableComponent& Collider = *it;
      WGameObject* pColliderObject = Collider.GetOwner();
      ShipComponent* pShipComponent = nullptr;

      if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
      {
        // don't collide with yourself
        if (pShipComponent->m_iPlayerIndex == m_iPlayerIndex)
          continue;
      }

      WBoundingSphere bs = WBoundingSphere::MakeFromCenterAndRadius(pColliderObject->GetLocalPosition(), Collider.m_fCollisionRadius);

      const WVec3 vPos = GetOwner()->GetLocalPosition();

      if (!bs.Contains(vPos) && bs.GetLineSegmentIntersection(vPos, vPos + vTravelDist))
      {
        // bounce the ship into the opposite direction and lose a lot of velocity, when there is a collision
        m_vVelocity *= -0.5f;
        vTravelDist.SetZero();
        break;
      }
    }
  }

  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + vTravelDist);

  if (m_CurShootCooldown > WTime::MakeFromSeconds(0))
  {
    m_CurShootCooldown -= tDiff;
  }
  else if (m_bIsShooting && m_fAmmunition >= CVar_ProjectileAmmoPerShot)
  {
    m_CurShootCooldown = WTime::MakeFromSeconds(CVar_ShotDelay);

    WGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_LocalPosition = GetOwner()->GetLocalPosition();
    desc.m_LocalRotation = GetOwner()->GetGlobalRotation();

    WGameObject* pProjectile = nullptr;
    GetWorld()->CreateObject(desc, pProjectile);

    {
      ProjectileComponent* pProjectileComponent = nullptr;
      WComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(pProjectile, pProjectileComponent);

      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_fSpeed = CVar_ProjectileSpeed;
      pProjectileComponent->m_fDoesDamage = 1.0f;
    }

    // ProjectileMesh
    {
      WMeshComponent* pMeshComponent = nullptr;
      WMeshComponent::CreateComponent(pProjectile, pMeshComponent);

      pMeshComponent->SetMesh(WResourceManager::LoadResource<WMeshResource>("ProjectileMesh"));

      // this only works because the materials are part of the Asset Collection and get a name like this from there
      // otherwise we would need to have the GUIDs of the 4 different material assets available
      WStringBuilder sMaterialName;
      sMaterialName.SetFormat("MaterialPlayer{0}", m_iPlayerIndex + 1);
      pMeshComponent->SetMaterial(0, WResourceManager::LoadResource<WMaterialResource>(sMaterialName));
    }

    m_fAmmunition -= CVar_ProjectileAmmoPerShot;

    float ShootTrack[20] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    if (auto pController = WInputManager::GetInputDeviceOfType<WInputDeviceController>())
    {
      pController->AddVibrationTrack(static_cast<WUInt8>(m_iPlayerIndex), WInputDeviceController::Motor::RightMotor, ShootTrack, 20);
    }
  }

  m_fAmmunition = WMath::Clamp<float>(m_fAmmunition + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxAmmo);
  m_fHealth = WMath::Clamp<float>(m_fHealth + (float)tDiff.GetSeconds(), 0.0f, CVar_MaxHealth);

  // clamp the player position to the playing field
  WVec3 vCurPos = GetOwner()->GetLocalPosition();
  vCurPos = vCurPos.CompMax(WVec3(-20.0f));
  vCurPos = vCurPos.CompMin(WVec3(20.0f));

  GetOwner()->SetLocalPosition(vCurPos);
}
