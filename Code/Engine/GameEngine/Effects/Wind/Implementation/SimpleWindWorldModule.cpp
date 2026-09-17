#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WSimpleWindWorldModule);

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimpleWindWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSimpleWindWorldModule::WSimpleWindWorldModule(WWorld* pWorld)
  : WWindWorldModuleInterface(pWorld)
{
  m_vFallbackWind.SetZero();
}

WSimpleWindWorldModule::~WSimpleWindWorldModule() = default;

WVec3 WSimpleWindWorldModule::GetWindAt(const WVec3& vPosition) const
{
  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    WTempHybridArray<WGameObject*, 16> volumes;

    WSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = WWindVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(WBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const WSimdVec4f pos = WSimdConversion::ToVec3(vPosition);
    WSimdVec4f force = WSimdVec4f::MakeZero();

    for (WGameObject* pObj : volumes)
    {
      WWindVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force += pVol->ComputeForceAtGlobalPosition(pos);
      }
    }

    return m_vFallbackWind + WSimdConversion::ToVec3(force);
  }

  return m_vFallbackWind;
}

void WSimpleWindWorldModule::SetFallbackWind(const WVec3& vWind)
{
  m_vFallbackWind = vWind;
}



W_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindWorldModule);
