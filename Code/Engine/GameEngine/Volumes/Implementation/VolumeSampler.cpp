#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <RendererCore/Utils/BlackboardHelper.h>

WVolumeSampler::WVolumeSampler() = default;
WVolumeSampler::~WVolumeSampler() = default;

void WVolumeSampler::RegisterValue(WHashedString sName, WVariant defaultValue, WTime interpolationDuration /*= WTime::MakeZero()*/)
{
  auto& value = m_Values[sName];
  value.m_DefaultValue = defaultValue;
  value.m_CurrentValue = defaultValue;

  if (interpolationDuration.IsPositive())
  {
    // Reach 90% of target value after interpolation duration:
    // Lerp factor for exponential moving average:
    // y = 1-f^t
    // solve for f with y = 0.9:
    // f = 10^(-1 / t)
    value.m_fInterpolationFactor = WMath::Pow(10.0, -1.0 / interpolationDuration.GetSeconds());
  }
  else
  {
    value.m_fInterpolationFactor = -1.0;
  }

  WStringBuilder sb = sName.GetView();
  sb.Append(W_STRENGTH_SUFFIX);
  value.m_sStrengthName.Assign(sb);
  value.m_fCurrentStrength = 0.0f;
}

void WVolumeSampler::DeregisterValue(WHashedString sName)
{
  m_Values.Remove(sName);
}

void WVolumeSampler::DeregisterAllValues()
{
  m_Values.Clear();
}

void WVolumeSampler::SampleAtPosition(const WWorld& world, WSpatialData::Category spatialCategory, const WVec3& vGlobalPosition, WTime deltaTime, WBlackboard* pTargetBlackboard /*= nullptr*/)
{
  struct ComponentInfo
  {
    const WVolumeComponent* m_pComponent = nullptr;
    WUInt32 m_uiSortingKey = 0;
    float m_fAlpha = 0.0f;

    bool operator<(const ComponentInfo& other) const
    {
      return m_uiSortingKey < other.m_uiSortingKey;
    }
  };

  auto vPos = WSimdConversion::ToVec3(vGlobalPosition);
  WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(vGlobalPosition, 0.01f);

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();

  WTempHybridArray<ComponentInfo, 16> componentInfos;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](WGameObject* pObject)
    {
      WVolumeComponent* pComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pComponent))
      {
        ComponentInfo info;
        info.m_pComponent = pComponent;

        WSimdTransform scaledTransform = pComponent->GetOwner()->GetGlobalTransformSimd();

        if (auto pBoxComponent = WDynamicCast<const WVolumeBoxComponent*>(pComponent))
        {
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(WSimdConversion::ToVec3(pBoxComponent->GetExtents())) * 0.5f;

          WSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const WSimdVec4f absLocalPos = globalToLocalTransform.TransformPosition(vPos).Abs();
          if ((absLocalPos <= WSimdVec4f(1.0f)).AllSet<3>())
          {
            WSimdVec4f vAlpha = (WSimdVec4f(1.0f) - absLocalPos).CompDiv(WSimdConversion::ToVec3(pBoxComponent->GetFalloff()));
            vAlpha = vAlpha.CompMin(WSimdVec4f(1.0f)).CompMax(WSimdVec4f::MakeZero());
            info.m_fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          }
        }
        else if (auto pSphereComponent = WDynamicCast<const WVolumeSphereComponent*>(pComponent))
        {
          scaledTransform.m_Scale *= pSphereComponent->GetRadius();

          WSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const WSimdVec4f localPos = globalToLocalTransform.TransformPosition(vPos);
          const float distSquared = localPos.GetLengthSquared<3>();
          if (distSquared <= 1.0f)
          {
            info.m_fAlpha = WMath::Saturate((1.0f - WMath::Sqrt(distSquared)) / pSphereComponent->GetFalloff());
          }
        }
        else
        {
          W_ASSERT_NOT_IMPLEMENTED;
        }

        if (info.m_fAlpha > 0.0f)
        {
          info.m_uiSortingKey = ComputeSortingKey(pComponent->GetSortOrder(), scaledTransform.GetMaxScale());

          componentInfos.PushBack(info);
        }
      }

      return WVisitorExecution::Continue; });

  // Sort
  {
    componentInfos.Sort();
  }

  for (auto& it : m_Values)
  {
    auto& sName = it.Key();
    auto& value = it.Value();

    WVariant targetValue = value.m_DefaultValue;
    float fTargetStrength = 0.0f;

    for (auto& info : componentInfos)
    {
      WVariant volumeValue = info.m_pComponent->GetValue(sName);
      if (volumeValue.IsValid() == false)
        continue;

      if (targetValue.IsValid())
      {
        WResult conversionStatus = W_SUCCESS;
        WEnum<WVariantType> targetType = targetValue.GetType();
        WVariant newTargetValue = volumeValue.ConvertTo(targetType, &conversionStatus);
        if (conversionStatus.Failed())
        {
          WLog::Error("VolumeSampler: Can't convert volume value '{}' to '{}'.", sName, targetType);
          continue;
        }

        targetValue = WMath::Lerp(targetValue, newTargetValue, double(info.m_fAlpha));
      }
      else
      {
        targetValue = volumeValue;
      }

      fTargetStrength = WMath::Lerp(fTargetStrength, 1.0f, info.m_fAlpha);
    }

    // When interpolation is disabled, use a factor of 1 so that Lerp simply returns the target value.
    const double f = value.m_fInterpolationFactor > 0.0
                       ? 1.0 - WMath::Pow(value.m_fInterpolationFactor, deltaTime.GetSeconds())
                       : 1.0;

    // Update current value only if the target value is valid. Otherwise, keep the current value as is.
    if (targetValue.IsValid())
    {
      value.m_CurrentValue = value.m_CurrentValue.IsValid() ? WMath::Lerp(value.m_CurrentValue, targetValue, f) : targetValue;
    }

    // Always update strength, even if the value is invalid
    value.m_fCurrentStrength = WMath::Lerp(value.m_fCurrentStrength, fTargetStrength, f);

    if (pTargetBlackboard != nullptr)
    {
      pTargetBlackboard->SetEntryValue(sName, value.m_CurrentValue);
      pTargetBlackboard->SetEntryValue(value.m_sStrengthName, value.m_fCurrentStrength);
    }
  }
}

// static
WUInt32 WVolumeSampler::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  WUInt32 uiSortingKey = (WUInt32)(WMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | (0xFFFF - ((WUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}
