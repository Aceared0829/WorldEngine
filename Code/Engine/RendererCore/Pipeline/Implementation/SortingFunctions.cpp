#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WRenderSortingFunctions, 1)
  W_ENUM_CONSTANTS(WRenderSortingFunctions::ByRenderDataThenFrontToBack, WRenderSortingFunctions::BackToFrontThenByRenderData)
  W_ENUM_CONSTANTS(WRenderSortingFunctions::ByDepthOffsetOnly, WRenderSortingFunctions::BySortingKeyOnly)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  W_ALWAYS_INLINE WUInt32 CalculateTypeHash(const WRenderData* pRenderData)
  {
    WUInt32 uiTypeHash = WHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  template <WUInt32 Bits>
  W_ALWAYS_INLINE WUInt64 NormalizeDistance(float fDistance, float fMaxDistance)
  {
    const float fNormalizedDistance = WMath::Saturate(fDistance / fMaxDistance);
    return static_cast<WUInt64>(fNormalizedDistance * static_cast<float>(W_BIT(Bits) - 1));
  }

  template <WUInt32 Bits>
  W_FORCE_INLINE WUInt64 CalculateDistance(const WRenderData* pRenderData, const WCamera& camera)
  {
    ///\todo far-plane is not enough to normalize distance
    const float fDistance = (camera.GetPosition() - pRenderData->m_vGlobalPosition).GetLength() + pRenderData->m_fSortingDepthOffset;
    return NormalizeDistance<Bits>(fDistance, camera.GetFarPlane());
  }
} // namespace

// static
WUInt64 WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc(const WRenderData* pRenderData, const WCamera& camera)
{
  const WUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const WUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const WUInt64 uiDistance = CalculateDistance<16>(pRenderData, camera);

  const WUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
WUInt64 WRenderSortingFunctions::BackToFrontThenByRenderDataFunc(const WRenderData* pRenderData, const WCamera& camera)
{
  const WUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const WUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const WUInt64 uiInvDistance = 0xFFFF - CalculateDistance<16>(pRenderData, camera);

  const WUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}

// static
WUInt64 WRenderSortingFunctions::ByDepthOffsetOnlyFunc(const WRenderData* pRenderData, const WCamera& camera)
{
  const float fMidDistance = camera.GetFarPlane() * 0.5f;
  const float fDistance = fMidDistance + pRenderData->m_fSortingDepthOffset;
  const WUInt64 uiInvDistance = 0xFFFFFFFF - NormalizeDistance<32>(fDistance, camera.GetFarPlane());

  return uiInvDistance;
}

// static
WUInt64 WRenderSortingFunctions::BySortingKeyOnlyFunc(const WRenderData* pRenderData, const WCamera& camera)
{
  return pRenderData->m_uiSortingKey;
}

// static
WRenderSortingFunctions::Func WRenderSortingFunctions::GetFunction(Enum sortingFunction)
{
  switch (sortingFunction)
  {
    case ByRenderDataThenFrontToBack:
      return &ByRenderDataThenFrontToBackFunc;
    case BackToFrontThenByRenderData:
      return &BackToFrontThenByRenderDataFunc;
    case ByDepthOffsetOnly:
      return &ByDepthOffsetOnlyFunc;
    case BySortingKeyOnly:
      return &BySortingKeyOnlyFunc;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return nullptr;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);
