#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSurfaceResource, 1, WRTTIDefaultAllocator<WSurfaceResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WSurfaceResource);
// clang-format on

WEvent<const WSurfaceResourceEvent&, WMutex> WSurfaceResource::s_Events;

WSurfaceResource::WSurfaceResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WSurfaceResource::~WSurfaceResource()
{
  WSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = WSurfaceResourceEvent::Type::Destroyed;
  s_Events.Broadcast(e);

  W_ASSERT_DEV(m_pPhysicsMaterialPhysX == nullptr, "Physics material has not been cleaned up properly");
  W_ASSERT_DEV(m_pPhysicsMaterialJolt == nullptr, "Physics material has not been cleaned up properly");
}

WResourceLoadDesc WSurfaceResource::UnloadData(Unload WhatToUnload)
{
  W_IGNORE_UNUSED(WhatToUnload);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WSurfaceResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WSurfaceResource::UpdateContent", GetResourceIdOrDescription());

  m_Interactions.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    WSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      WTempHashedString s(i.m_sInteractionType.GetData());
      auto& item = m_Interactions.ExpandAndGetRef();
      item.m_uiInteractionTypeHash = s.GetHash();
      item.m_pInteraction = &i;
    }

    m_Interactions.Sort([](const SurfInt& lhs, const SurfInt& rhs) -> bool
      {
      if (lhs.m_uiInteractionTypeHash != rhs.m_uiInteractionTypeHash)
        return lhs.m_uiInteractionTypeHash < rhs.m_uiInteractionTypeHash;

      return lhs.m_pInteraction->m_fImpulseThreshold > rhs.m_pInteraction->m_fImpulseThreshold; });
  }

  res.m_State = WResourceState::Loaded;
  return res;
}

void WSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WSurfaceResource, WSurfaceResourceDescriptor)
{
  m_Descriptor = descriptor;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  WSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = WSurfaceResourceEvent::Type::Created;
  s_Events.Broadcast(e);

  return res;
}

const WSurfaceInteraction* WSurfaceResource::FindInteraction(const WSurfaceResource* pCurSurf, WUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue)
{
  while (true)
  {
    bool bFoundAny = false;

    // try to find a matching interaction
    for (const auto& interaction : pCurSurf->m_Interactions)
    {
      if (interaction.m_uiInteractionTypeHash > uiHash)
        break;

      if (interaction.m_uiInteractionTypeHash == uiHash)
      {
        bFoundAny = true;

        // only use it if the threshold is large enough
        if (fImpulseSqr >= WMath::Square(interaction.m_pInteraction->m_fImpulseThreshold))
        {
          const float fImpulse = WMath::Sqrt(fImpulseSqr);
          out_fImpulseParamValue = (fImpulse - interaction.m_pInteraction->m_fImpulseThreshold) * interaction.m_pInteraction->m_fImpulseScale;

          return interaction.m_pInteraction;
        }
      }
    }

    // if we did find something, we just never exceeded the threshold, then do not search in the base surface
    if (bFoundAny)
      break;

    if (pCurSurf->m_Descriptor.m_hBaseSurface.IsValid())
    {
      WResourceLock<WSurfaceResource> pBase(pCurSurf->m_Descriptor.m_hBaseSurface, WResourceAcquireMode::BlockTillLoaded);
      pCurSurf = pBase.GetPointer();
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

bool WSurfaceResource::InteractWithSurface(WWorld* pWorld, WGameObjectHandle hObject, const WVec3& vPosition, const WVec3& vSurfaceNormal, const WVec3& vIncomingDirection, const WTempHashedString& sInteraction, const WUInt16* pOverrideTeamID, float fImpulseSqr /*= 0.0f*/) const
{
  float fImpulseParam = 0;
  const WSurfaceInteraction* pIA = FindInteraction(this, sInteraction.GetHash(), fImpulseSqr, fImpulseParam);

  if (pIA == nullptr)
    return false;

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  WResourceLock<WPrefabResource> pPrefab(pIA->m_hPrefab, WResourceAcquireMode::BlockTillLoaded);

  WVec3 vDir;

  switch (pIA->m_Alignment)
  {
    case WSurfaceInteractionAlignment::SurfaceNormal:
      vDir = vSurfaceNormal;
      break;

    case WSurfaceInteractionAlignment::IncidentDirection:
      vDir = -vIncomingDirection;
      ;
      break;

    case WSurfaceInteractionAlignment::ReflectedDirection:
      vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;

    case WSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vDir = -vSurfaceNormal;
      break;

    case WSurfaceInteractionAlignment::ReverseIncidentDirection:
      vDir = vIncomingDirection;
      ;
      break;

    case WSurfaceInteractionAlignment::ReverseReflectedDirection:
      vDir = -vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;
  }

  vDir.Normalize();
  WVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();

  // random rotation around the spawn direction
  {
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleMinMax(0.0, WMath::Pi<double>() * 2.0);

    WMat3 rotMat = WMat3::MakeAxisRotation(vDir, WAngle::MakeFromRadian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > WAngle::MakeFromRadian(0.0f))
  {
    WAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case WSurfaceInteractionAlignment::IncidentDirection:
      case WSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = WMath::Pi<float>() - WMath::ACos(fCosAngle).GetRadian();

        maxDeviation = WMath::Min(pIA->m_Deviation, WAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      case WSurfaceInteractionAlignment::ReflectedDirection:
      case WSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = WMath::Pi<float>() - WMath::ACos(fCosAngle).GetRadian();

        maxDeviation = WMath::Min(pIA->m_Deviation, WAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const WAngle deviation = WAngle::MakeFromRadian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    WMat3 matTilt = WMat3::MakeAxisRotation(vTangent, deviation);

    vDir = matTilt * vDir;
  }


  // finally compute the bi-tangent
  const WVec3 vBiTangent = vDir.CrossRH(vTangent);

  WMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  WTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation = WQuat::MakeFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  WGameObjectHandle hParent;

  WGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t = WTransform::MakeLocalTransform(pObject->GetGlobalTransform(), t);
  }

  WTempHybridArray<WGameObject*, 8> rootObjects;

  WPrefabInstantiationOptions options;
  options.m_hParent = hParent;
  options.m_pCreatedRootObjectsOut = &rootObjects;
  options.m_pOverrideTeamID = pOverrideTeamID;

  pPrefab->InstantiatePrefab(*pWorld, t, options, &pIA->m_Parameters);

  {
    WMsgSetFloatParameter msgSetFloat;
    msgSetFloat.m_sParameterName = "Impulse";
    msgSetFloat.m_fValue = fImpulseParam;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msgSetFloat, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
    }
  }

  if (pObject != nullptr && pObject->IsDynamic())
  {
    WMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}

bool WSurfaceResource::IsBasedOn(const WSurfaceResource* pThisOrBaseSurface) const
{
  if (pThisOrBaseSurface == this)
    return true;

  if (m_Descriptor.m_hBaseSurface.IsValid())
  {
    WResourceLock<WSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, WResourceAcquireMode::BlockTillLoaded);

    return pBase->IsBasedOn(pThisOrBaseSurface);
  }

  return false;
}

bool WSurfaceResource::IsBasedOn(const WSurfaceResourceHandle hThisOrBaseSurface) const
{
  WResourceLock<WSurfaceResource> pThisOrBaseSurface(hThisOrBaseSurface, WResourceAcquireMode::BlockTillLoaded);

  return IsBasedOn(pThisOrBaseSurface.GetPointer());
}


W_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResource);
