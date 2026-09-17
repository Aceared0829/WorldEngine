#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

template <typename T>
T* WRenderDataManager::CreateRenderDataForThisFrame(const WGameObject* pOwner) const
{
  static_assert(W_IS_DERIVED_FROM_STATIC(WRenderData, T));

  T* pRenderData = W_NEW(WFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_Flags.AddOrRemove(WRenderData::Flags::Dynamic, pOwner->IsDynamic());
    pRenderData->m_Flags.AddOrRemove(WRenderData::Flags::FlipWinding, pOwner->GetGlobalTransformSimd().HasMirrorScaling());

    pRenderData->m_vGlobalPosition = pOwner->GetGlobalPosition();

    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

// static
W_FORCE_INLINE void WRenderDataManager::FillPerInstanceData(WPerInstanceData& out_perInstanceData, const WGameObject* pObject, const WTransform& globalTransform, WUInt32 uiUniqueID /*= 0*/, const WColor& color /*= WColor::White*/, const WVec4& vCustomData /*= WVec4(0, 1, 0, 1)*/, float fBoundingSphereRadius /*= 1.0f*/, WUInt32 uiRandomSeed /*= 0*/)
{
  WMat4 objectToWorld = globalTransform.GetAsMat4();
  out_perInstanceData.ObjectToWorld = objectToWorld;

  if (globalTransform.HasOnlyUniformScaling())
  {
    out_perInstanceData.ObjectToWorldNormal = objectToWorld;
  }
  else
  {
    WMat3 mInverse = objectToWorld.GetRotationalPart();
    mInverse.Invert(0.0f).IgnoreResult();
    // we explicitly ignore the return value here (success / failure)
    // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

    out_perInstanceData.ObjectToWorldNormal = mInverse.GetTranspose();
  }

  if (pObject != nullptr)
  {
    out_perInstanceData.BoundingSphereRadius = pObject->GetGlobalBounds().m_fSphereRadius;
    out_perInstanceData.RandomSeed = pObject->GetStableRandomSeed();
  }
  else
  {
    out_perInstanceData.BoundingSphereRadius = fBoundingSphereRadius;
    out_perInstanceData.RandomSeed = uiRandomSeed;
  }

  out_perInstanceData.GameObjectID = uiUniqueID;
  out_perInstanceData.Reserved = 0;

  out_perInstanceData.Color = color;
  out_perInstanceData.CustomData = vCustomData;
}

W_FORCE_INLINE WGALDynamicBufferHandle WRenderDataManager::GetOrCreateInstanceDataAndFill(const WComponent& ownerComponent, bool bDynamic, const WTransform& globalTransform, WInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiUniqueID /*= 0*/, const WColor& color /*= WColor::White*/, const WVec4& vCustomData /*= WVec4(0, 1, 0, 1)*/) const
{
  WGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = GetOrCreateInstanceData(&ownerComponent, bDynamic, hInstanceDataBuffer, inout_instanceDataOffset);
  FillPerInstanceData(instanceData[0], ownerComponent.GetOwner(), globalTransform, uiUniqueID, color, vCustomData);

  return hInstanceDataBuffer;
}

template <typename T>
W_ALWAYS_INLINE WArrayPtr<T> WRenderDataManager::GetOrCreateCustomInstanceData(WUInt32 uiCustomDataIndex, const WComponent* pOwnerComponent, WGALDynamicBufferHandle& out_hBuffer, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount /*= 1*/) const
{
  WByteArrayPtr data = GetOrCreateCustomInstanceData(uiCustomDataIndex, sizeof(T), pOwnerComponent, out_hBuffer, inout_instanceDataOffset, uiCount);
  return WArrayPtr<T>(reinterpret_cast<T*>(data.GetPtr()), data.GetCount() / sizeof(T));
}

template <typename T>
W_FORCE_INLINE WGALDynamicBufferHandle WRenderDataManager::GetOrCreateCustomInstanceDataAndFill(WUInt32 uiCustomDataIndex, const WComponent& ownerComponent, WCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const
{
  WGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = GetOrCreateCustomInstanceData<T>(uiCustomDataIndex, &ownerComponent, hInstanceDataBuffer, inout_instanceDataOffset);
  instanceData[0] = data;

  return hInstanceDataBuffer;
}

W_ALWAYS_INLINE WGALDynamicBufferHandle WRenderDataManager::GetCustomInstanceDataBuffer(WUInt32 uiCustomDataIndex) const
{
  return m_Buffers[uiCustomDataIndex];
}
