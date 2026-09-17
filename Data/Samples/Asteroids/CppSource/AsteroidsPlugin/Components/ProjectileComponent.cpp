#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererCore/Meshes/MeshComponent.h>

#include <Core/Input/DeviceTypes/Controller.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(ProjectileComponent, 1, WComponentMode::Dynamic);
W_END_COMPONENT_TYPE
// clang-format on

WCVarFloat CVar_ProjectileTimeToLive("g_ProjectileTimeToLive", 0.5f, WCVarFlags::Default, "Projectile time to Live");
WCVarFloat CVar_SparksTimeToLive("g_SparksTimeToLive", 3.0f, WCVarFlags::Default, "Projectile time to fade out");
WCVarInt CVar_SparksPerHit("g_SparksPerHit", 50, WCVarFlags::Default, "Number of particles spawned when projectile hits a ship");
WCVarFloat CVar_SparksSpeed("g_SparksSpeed", 50.0f, WCVarFlags::Default, "Projectile fly speed");

ProjectileComponent::ProjectileComponent()
{
  m_TimeToLive = WTime::MakeFromSeconds(CVar_ProjectileTimeToLive);
}

void ProjectileComponent::Update()
{
  const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  m_TimeToLive -= tDiff;

  if (m_TimeToLive.IsZeroOrNegative())
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    return;
  }

  if (m_fSpeed <= 0.0f)
    return;

  const WVec3 vVelocity = GetOwner()->GetLocalRotation() * WVec3(m_fSpeed, 0, 0);
  const WVec3 vDistance = (float)tDiff.GetSeconds() * vVelocity;
  GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + vDistance);

  CollidableComponentManager* pCollidableManager = GetWorld()->GetOrCreateComponentManager<CollidableComponentManager>();

  for (auto it = pCollidableManager->GetComponents(); it.IsValid(); ++it)
  {
    CollidableComponent& collider = *it;

    if (!collider.IsActiveAndSimulating())
      continue;

    WGameObject* pColliderObject = collider.GetOwner();
    ShipComponent* pShipComponent = nullptr;

    if (pColliderObject->TryGetComponentOfBaseType(pShipComponent))
    {
      if (pShipComponent->m_iPlayerIndex == m_iBelongsToPlayer)
        continue;

      if (!pShipComponent->IsAlive())
        continue;
    }

    WBoundingSphere bs = WBoundingSphere::MakeFromCenterAndRadius(pColliderObject->GetLocalPosition(), collider.m_fCollisionRadius);

    const WVec3 vPos = GetOwner()->GetLocalPosition();

    if (!vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + vDistance))
    {
      if (pShipComponent && m_fDoesDamage > 0.0f)
      {
        pShipComponent->m_fHealth = WMath::Max(pShipComponent->m_fHealth - m_fDoesDamage, 0.0f);

        {
          float HitTrack[20] = {
            1.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

          if (auto pController = WInputManager::GetInputDeviceOfType<WInputDeviceController>())
          {
            pController->AddVibrationTrack(static_cast<WUInt8>(pShipComponent->m_iPlayerIndex), WInputDeviceController::Motor::LeftMotor, HitTrack, 20);
          }
        }

        const float fAngle = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(10.0, 100.0);
        const float fSteps = fAngle / CVar_SparksPerHit;

        for (WInt32 i = 0; i < CVar_SparksPerHit; ++i)
        {
          WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree((i - (CVar_SparksPerHit / 2)) * fSteps));

          {
            WGameObjectDesc desc;
            desc.m_bDynamic = true;
            desc.m_LocalPosition = GetOwner()->GetLocalPosition();
            desc.m_LocalRotation = qRot * GetOwner()->GetLocalRotation();

            WGameObject* pProjectile = nullptr;
            GetWorld()->CreateObject(desc, pProjectile);

            ProjectileComponent* pProjectileComponent = nullptr;
            WComponentHandle hProjectileComponent = ProjectileComponent::CreateComponent(pProjectile, pProjectileComponent);

            pProjectileComponent->m_iBelongsToPlayer = pShipComponent->m_iPlayerIndex;
            pProjectileComponent->m_fSpeed = (float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(1.0, 2.0) * CVar_SparksSpeed;
            pProjectileComponent->m_fDoesDamage = 0.0f;

            // ProjectileMesh
            {
              WMeshComponent* pMeshComponent = nullptr;
              WMeshComponent::CreateComponent(pProjectile, pMeshComponent);

              pMeshComponent->SetMesh(WResourceManager::LoadResource<WMeshResource>("ProjectileMesh"));

              // this only works because the materials are part of the Asset Collection and get a name like this from there
              // otherwise we would need to have the GUIDs of the 4 different material assets available
              WStringBuilder sMaterialName;
              sMaterialName.SetFormat("MaterialPlayer{0}", pShipComponent->m_iPlayerIndex + 1);
              pMeshComponent->SetMaterial(0, WResourceManager::LoadResource<WMaterialResource>(sMaterialName));
            }
          }
        }
      }

      if (pShipComponent)
      {
        m_TimeToLive = WTime::MakeFromSeconds(0);
      }
      else
      {
        m_fSpeed = 0.0f;
        m_TimeToLive = WTime::MakeFromSeconds(CVar_SparksTimeToLive);
      }
    }
  }
}
