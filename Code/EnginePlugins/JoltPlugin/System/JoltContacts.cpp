#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/CVar.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>

WCVarInt cvar_PhysicsReactionsMaxImpacts("Jolt.Reactions.MaxImpacts", 4, WCVarFlags::Default, "Maximum number of impact reactions to spawn per frame.");
WCVarInt cvar_PhysicsReactionsMaxSlidesOrRolls("Jolt.Reactions.MaxSlidesOrRolls", 4, WCVarFlags::Default, "Maximum number of active slide or roll reactions.");
WCVarBool cvar_PhysicsReactionsVisImpacts("Jolt.Reactions.VisImpacts", false, WCVarFlags::Default, "Visualize where impact reactions are spawned.");
WCVarBool cvar_PhysicsReactionsVisDiscardedImpacts("Jolt.Reactions.VisDiscardedImpacts", false, WCVarFlags::Default, "Visualize where impact reactions were NOT spawned.");
WCVarBool cvar_PhysicsReactionsVisSlides("Jolt.Reactions.VisSlides", false, WCVarFlags::Default, "Visualize active slide reactions.");
WCVarBool cvar_PhysicsReactionsVisRolls("Jolt.Reactions.VisRolls", false, WCVarFlags::Default, "Visualize active roll reactions.");

void WJoltContactListener::RemoveTrigger(const WJoltTriggerComponent* pTrigger)
{
  W_LOCK(m_TriggerMutex);

  for (auto it = m_Trigs.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_pTrigger == pTrigger)
    {
      it = m_Trigs.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

W_ALWAYS_INLINE bool OnDebrisContact(const JPH::Body& body1, const JPH::Body& body2, JPH::ContactSettings& ref_settings)
{
  // one-way physics for debris
  // debris may be pushed by everything else, but it doesn't push anything else
  // debris also generally doesn't collide with itself

  if (body1.GetBroadPhaseLayer().GetValue() == (WUInt8)WJoltBroadphaseLayer::Debris)
  {
    ref_settings.mInvMassScale2 = 0;
    ref_settings.mInvInertiaScale2 = 0;
    return true;
  }
  else if (body2.GetBroadPhaseLayer().GetValue() == (WUInt8)WJoltBroadphaseLayer::Debris)
  {
    ref_settings.mInvMassScale1 = 0;
    ref_settings.mInvInertiaScale1 = 0;
    return true;
  }

  return false;
}

void WJoltContactListener::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings)
{
  const bool bIsDebrisContact = OnDebrisContact(body1, body2, ref_settings);

  const WUInt64 uiBody1id = body1.GetID().GetIndexAndSequenceNumber();
  const WUInt64 uiBody2id = body2.GetID().GetIndexAndSequenceNumber();

  if (!bIsDebrisContact && ActivateTrigger(body1, body2, uiBody1id, uiBody2id))
    return;

  OnContact(body1, body2, manifold, ref_settings, false, bIsDebrisContact);
}

void WJoltContactListener::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings)
{
  const bool bIsDebrisContact = OnDebrisContact(body1, body2, ref_settings);

  OnContact(body1, body2, manifold, ref_settings, true, bIsDebrisContact);
}

void WJoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)
{
  const WUInt64 uiBody1id = subShapePair.GetBody1ID().GetIndexAndSequenceNumber();
  const WUInt64 uiBody2id = subShapePair.GetBody2ID().GetIndexAndSequenceNumber();

  DeactivateTrigger(uiBody1id, uiBody2id);
}

void WJoltContactListener::OnContact(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings, bool bPersistent, bool bIsDebrisContact)
{
  // compute per-material friction and restitution
  {

    float fRes0 = 0.2f;
    float fRes1 = 0.2f;

    float fFri0 = 0.5f;
    float fFri1 = 0.5f;

    if (const WJoltMaterial* pMat0 = static_cast<const WJoltMaterial*>(body0.GetShape()->GetMaterial(manifold.mSubShapeID1)))
    {
      fRes0 = pMat0->m_fRestitution;
      fFri0 = pMat0->m_fFriction;
    }

    if (const WJoltMaterial* pMat1 = static_cast<const WJoltMaterial*>(body1.GetShape()->GetMaterial(manifold.mSubShapeID2)))
    {
      fRes1 = pMat1->m_fRestitution;
      fFri1 = pMat1->m_fFriction;
    }

    ref_settings.mCombinedRestitution = WMath::Max(fRes0, fRes1);
    ref_settings.mCombinedFriction = WMath::Sqrt(fFri0 * fFri1);
  }

  m_ContactEvents.m_pWorld = m_pWorld;

  WBitflags<WOnJoltContact> ContactFlags0 = WJoltUserData::GetContactFlags(reinterpret_cast<const void*>(body0.GetUserData()));
  WBitflags<WOnJoltContact> ContactFlags1 = WJoltUserData::GetContactFlags(reinterpret_cast<const void*>(body1.GetUserData()));

  if (bIsDebrisContact)
  {
    // debris is specifically excluded from triggering various interactions, to reduce the load on the system and prevent undesireable behavior
    ContactFlags0.Remove(WOnJoltContact::SendContactMsg | WOnJoltContact::SlideAndRollReactions);
    ContactFlags1.Remove(WOnJoltContact::SendContactMsg | WOnJoltContact::SlideAndRollReactions);
  }

  WBitflags<WOnJoltContact> CombinedContactFlags;
  CombinedContactFlags.SetValue(ContactFlags0.GetValue() | ContactFlags1.GetValue());

  if (CombinedContactFlags.IsAnySet(WOnJoltContact::AllReactions))
  {
    WVec3 vAvgPos(0);
    const WVec3 vAvgNormal = WJoltConversionUtils::ToVec3(manifold.mWorldSpaceNormal);

    const float fImpactSqr = (body0.GetLinearVelocity() - body1.GetLinearVelocity()).LengthSq();

    for (WUInt32 uiContactPointIndex = 0; uiContactPointIndex < manifold.mRelativeContactPointsOn1.size(); ++uiContactPointIndex)
    {
      vAvgPos += WJoltConversionUtils::ToVec3(manifold.GetWorldSpaceContactPointOn1(uiContactPointIndex));
      vAvgPos -= vAvgNormal * manifold.mPenetrationDepth;
    }

    vAvgPos /= (float)manifold.mRelativeContactPointsOn1.size();

    if (bPersistent)
    {
      m_ContactEvents.OnContact_SlideAndRollReaction(body0, body1, manifold, ContactFlags0, ContactFlags1, vAvgPos, vAvgNormal, CombinedContactFlags);
    }
    else if (fImpactSqr >= 1.0f && CombinedContactFlags.IsAnySet(WOnJoltContact::ImpactReactions))
    {
      const WJoltMaterial* pMat1 = static_cast<const WJoltMaterial*>(body0.GetShape()->GetMaterial(manifold.mSubShapeID1));
      const WJoltMaterial* pMat2 = static_cast<const WJoltMaterial*>(body1.GetShape()->GetMaterial(manifold.mSubShapeID2));

      if (pMat1 == nullptr)
        pMat1 = static_cast<const WJoltMaterial*>(WJoltMaterial::sDefault.GetPtr());
      if (pMat2 == nullptr)
        pMat2 = static_cast<const WJoltMaterial*>(WJoltMaterial::sDefault.GetPtr());

      m_ContactEvents.OnContact_ImpactReaction(vAvgPos, vAvgNormal, fImpactSqr, pMat1->m_pSurface, pMat2->m_pSurface, body0.IsStatic() || body0.IsKinematic());
    }

    if (CombinedContactFlags.IsSet(WOnJoltContact::SendContactMsg))
    {
      auto pComp0 = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body0.GetUserData()));
      auto pComp1 = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body1.GetUserData()));

      WMsgPhysicContact msg;
      msg.m_vGlobalPosition = vAvgPos;
      msg.m_vNormal = vAvgNormal;
      msg.m_fImpactSqr = fImpactSqr;

      if (ContactFlags0.IsSet(WOnJoltContact::SendContactMsg) && pComp0 != nullptr)
      {
        msg.m_hOtherObject = pComp1 ? pComp1->GetOwner()->GetHandle() : WGameObjectHandle();
        pComp0->SendMessage(msg);
      }

      if (ContactFlags1.IsSet(WOnJoltContact::SendContactMsg) && pComp1 != nullptr)
      {
        msg.m_hOtherObject = pComp0 ? pComp0->GetOwner()->GetHandle() : WGameObjectHandle();
        pComp1->SendMessage(msg);
      }
    }
  }
}

bool WJoltContactListener::ActivateTrigger(const JPH::Body& body1, const JPH::Body& body2, WUInt64 uiBody1id, WUInt64 uiBody2id)
{
  if (!body1.IsSensor() && !body2.IsSensor())
    return false;

  const WJoltTriggerComponent* pTrigger = nullptr;
  const WComponent* pComponent = nullptr;

  if (body1.IsSensor())
  {
    pTrigger = WJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(body1.GetUserData()));
    pComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body2.GetUserData()));
  }
  else
  {
    pTrigger = WJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(body2.GetUserData()));
    pComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body1.GetUserData()));
  }

  if (pTrigger && pComponent)
  {
    W_LOCK(m_TriggerMutex);

    const WUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);

    auto& trig = m_Trigs[uiStoreID];
    trig.m_pTrigger = pTrigger;
    trig.m_hTarget = pComponent->GetOwner()->GetHandle();

    if (trig.m_iTriggerCount == 0)
    {
      pTrigger->PostTriggerMessage(pComponent->GetOwner()->GetHandle(), WTriggerState::Activated);
    }

    ++trig.m_iTriggerCount;
  }

  // one of the bodies is a trigger
  return true;
}

void WJoltContactListener::DeactivateTrigger(WUInt64 uiBody1id, WUInt64 uiBody2id)
{
  W_LOCK(m_TriggerMutex);

  const WUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);
  auto itTrig = m_Trigs.Find(uiStoreID);

  if (itTrig.IsValid())
  {
    auto& trig = itTrig.Value();
    trig.m_iTriggerCount--;

    if (trig.m_iTriggerCount == 0)
    {
      trig.m_pTrigger->PostTriggerMessage(trig.m_hTarget, WTriggerState::Deactivated);
      m_Trigs.Remove(itTrig);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void WJoltContactEvents::SpawnPhysicsImpactReactions()
{
  W_PROFILE_SCOPE("SpawnPhysicsImpactReactions");

  W_LOCK(m_Mutex);

  WUInt32 uiMaxPrefabsToSpawn = cvar_PhysicsReactionsMaxImpacts;

  for (const auto& ic : m_InteractionContacts)
  {
    if (ic.m_pSurface != nullptr)
    {
      if (uiMaxPrefabsToSpawn > 0 && ic.m_pSurface->InteractWithSurface(m_pWorld, WGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr))
      {
        --uiMaxPrefabsToSpawn;

        if (cvar_PhysicsReactionsVisImpacts)
        {
          WDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, WColor::LightGreen, WTransform(ic.m_vPosition), WTime::MakeFromSeconds(3));
          WDebugRenderer::AddPersistentInfoText(m_pWorld, WDebugTextPlacement::BottomLeft, WFmt("Impact: {}", WMath::Sqrt(ic.m_fImpulseSqr)), WTime::Seconds(3), WColor::White);
        }
      }
      else
      {
        if (cvar_PhysicsReactionsVisDiscardedImpacts)
        {
          WDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, WColor::DarkGray, WTransform(ic.m_vPosition), WTime::MakeFromSeconds(1));
        }
      }
    }
  }

  m_InteractionContacts.Clear();
}

void WJoltContactEvents::UpdatePhysicsSlideReactions()
{
  W_PROFILE_SCOPE("UpdatePhysicsSlideReactions");

  W_LOCK(m_Mutex);

  for (auto& slideInfo : m_SlidingOrRollingActors)
  {
    if (slideInfo.m_pBody == nullptr)
      continue;

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        WPrefabResourceHandle hPrefab = WResourceManager::LoadResource<WPrefabResource>(slideInfo.m_sSlideInteractionPrefab);
        WResourceLock<WPrefabResource> pPrefab(hPrefab, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == WResourceAcquireResult::Final)
        {
          WTempHybridArray<WGameObject*, 8> created;

          WPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, WTransform(slideInfo.m_vContactPosition), options);
          slideInfo.m_hSlidePrefab = created[0]->GetHandle();
        }
      }
      else
      {
        WGameObject* pObject;
        if (m_pWorld->TryGetObject(slideInfo.m_hSlidePrefab, pObject))
        {
          pObject->SetGlobalPosition(slideInfo.m_vContactPosition);
        }
        else
        {
          slideInfo.m_hSlidePrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisSlides)
      {
        WDebugRenderer::DrawLineBox(m_pWorld, WBoundingBox::MakeFromMinMax(WVec3(-0.5f), WVec3(0.5f)), WColor::BlueViolet, WTransform(slideInfo.m_vContactPosition));
      }

      slideInfo.m_bStillSliding = false;
    }
    else
    {
      if (!slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(slideInfo.m_hSlidePrefab);
        slideInfo.m_hSlidePrefab.Invalidate();
      }
    }
  }
}

void WJoltContactEvents::UpdatePhysicsRollReactions()
{
  W_PROFILE_SCOPE("UpdatePhysicsRollReactions");

  W_LOCK(m_Mutex);

  for (auto& rollInfo : m_SlidingOrRollingActors)
  {
    if (rollInfo.m_pBody == nullptr)
      continue;

    if (rollInfo.m_bStillRolling)
    {
      if (rollInfo.m_hRollPrefab.IsInvalidated())
      {
        WPrefabResourceHandle hPrefab = WResourceManager::LoadResource<WPrefabResource>(rollInfo.m_sRollInteractionPrefab);
        WResourceLock<WPrefabResource> pPrefab(hPrefab, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == WResourceAcquireResult::Final)
        {
          WTempHybridArray<WGameObject*, 8> created;

          WPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, WTransform(rollInfo.m_vContactPosition), options);
          rollInfo.m_hRollPrefab = created[0]->GetHandle();
        }
      }
      else
      {
        WGameObject* pObject;
        if (m_pWorld->TryGetObject(rollInfo.m_hRollPrefab, pObject))
        {
          pObject->SetGlobalPosition(rollInfo.m_vContactPosition);
        }
        else
        {
          rollInfo.m_hRollPrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisRolls)
      {
        WDebugRenderer::DrawLineCapsuleZ(m_pWorld, 0.4f, 0.2f, WColor::GreenYellow, WTransform(rollInfo.m_vContactPosition));
      }

      rollInfo.m_bStillRolling = false;
      rollInfo.m_bStillSliding = false; // ensures that no slide reaction is spawned as well
    }
    else
    {
      if (!rollInfo.m_hRollPrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(rollInfo.m_hRollPrefab);
        rollInfo.m_hRollPrefab.Invalidate();
      }
    }
  }
}

void WJoltContactEvents::OnContact_ImpactReaction(const WVec3& vAvgPos, const WVec3& vAvgNormal, float fMaxImpactSqr, const WSurfaceResource* pSurface1, const WSurfaceResource* pSurface2, bool bActor1StaticOrKinematic)
{
  const float fDistanceSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  W_LOCK(m_Mutex);

  InteractionContact* ic = nullptr;

  if (m_InteractionContacts.GetCount() < (WUInt32)cvar_PhysicsReactionsMaxImpacts * 2)
  {
    ic = &m_InteractionContacts.ExpandAndGetRef();
    ic->m_pSurface = nullptr;
    ic->m_fDistanceSqr = WMath::HighValue<float>();
  }
  else
  {
    // compute a score, which contact point is best to replace
    // * prefer to replace points that are farther away than the new one
    // * prefer to replace points that have a lower impact strength than the new one

    float fBestScore = 0;
    WUInt32 uiBestScore = 0xFFFFFFFFu;

    for (WUInt32 i = 0; i < m_InteractionContacts.GetCount(); ++i)
    {
      float fScore = 0;
      fScore += m_InteractionContacts[i].m_fDistanceSqr - fDistanceSqr;
      fScore += 2.0f * (fMaxImpactSqr - m_InteractionContacts[i].m_fImpulseSqr);

      if (fScore > fBestScore)
      {
        fBestScore = fScore;
        uiBestScore = i;
      }
    }

    if (uiBestScore == 0xFFFFFFFFu)
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        WDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, WColor::DimGrey, WTransform(vAvgPos), WTime::MakeFromSeconds(3));
      }

      return;
    }
    else
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        WDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, WColor::DimGrey, WTransform(m_InteractionContacts[uiBestScore].m_vPosition), WTime::MakeFromSeconds(3));
      }
    }

    // this is the best candidate to replace
    ic = &m_InteractionContacts[uiBestScore];
  }

  if (pSurface1 || pSurface2)
  {
    // if one of the objects doesn't have a surface configured, use the other one
    if (pSurface1 == nullptr)
      pSurface1 = pSurface2;
    if (pSurface2 == nullptr)
      pSurface2 = pSurface1;

    ic->m_fDistanceSqr = fDistanceSqr;
    ic->m_vPosition = vAvgPos;
    ic->m_vNormal = vAvgNormal;
    ic->m_vNormal.NormalizeIfNotZero(WVec3(0, 0, 1)).IgnoreResult();
    ic->m_fImpulseSqr = fMaxImpactSqr;

    // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
    if (bActor1StaticOrKinematic)
    {
      ic->m_pSurface = pSurface1;
      ic->m_sInteraction = pSurface2->GetDescriptor().m_sOnCollideInteraction;
    }
    else
    {
      ic->m_pSurface = pSurface2;
      ic->m_sInteraction = pSurface1->GetDescriptor().m_sOnCollideInteraction;
    }

    return;
  }

  if (cvar_PhysicsReactionsVisDiscardedImpacts)
  {
    WDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, WColor::DarkOrange, WTransform(vAvgPos), WTime::MakeFromSeconds(10));
  }
}

WJoltContactEvents::SlideAndRollInfo* WJoltContactEvents::FindSlideOrRollInfo(const JPH::Body* pBody, const WVec3& vAvgPos)
{
  SlideAndRollInfo* pUnused = nullptr;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (info.m_pBody == pBody)
      return &info;

    if (info.m_pBody == nullptr)
      pUnused = &info;
  }

  const float fDistSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  if (m_SlidingOrRollingActors.GetCount() < (WUInt32)cvar_PhysicsReactionsMaxSlidesOrRolls)
  {
    pUnused = &m_SlidingOrRollingActors.ExpandAndGetRef();
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  float fBestDist = 0.0f;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (!info.m_hRollPrefab.IsInvalidated() || !info.m_hSlidePrefab.IsInvalidated())
      continue;

    // this slot is not yet really in use, so can be replaced by a better match

    if (fDistSqr < info.m_fDistanceSqr && info.m_fDistanceSqr > fBestDist)
    {
      fBestDist = info.m_fDistanceSqr;
      pUnused = &info;
    }
  }

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  return nullptr;
}

void WJoltContactEvents::OnContact_RollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal0)
{
  // only consider something 'rolling' when it turns faster than this (per second)
  constexpr WAngle rollThreshold = WAngle::MakeFromDegree(45);

  WBitflags<WOnJoltContact> contactFlags[2] = {onContact0, onContact1};
  const JPH::Body* bodies[2] = {&body0, &body1};
  const JPH::SubShapeID shapeIds[2] = {manifold.mSubShapeID1, manifold.mSubShapeID2};

  for (WUInt32 i = 0; i < 2; ++i)
  {
    if (contactFlags[i].IsAnySet(WOnJoltContact::AllRollReactions))
    {
      const WVec3 vAngularVel = WJoltConversionUtils::ToVec3(bodies[i]->GetRotation().InverseRotate(bodies[i]->GetAngularVelocity()));

      if ((contactFlags[i].IsSet(WOnJoltContact::RollXReactions) && WMath::Abs(vAngularVel.x) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(WOnJoltContact::RollYReactions) && WMath::Abs(vAngularVel.y) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(WOnJoltContact::RollZReactions) && WMath::Abs(vAngularVel.z) > rollThreshold.GetRadian()))
      {
        const WJoltMaterial* pMaterial = static_cast<const WJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

        if (pMaterial && pMaterial->m_pSurface)
        {
          if (!pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab.IsEmpty())
          {
            W_LOCK(m_Mutex);

            if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
            {
              pInfo->m_bStillRolling = true;
              pInfo->m_vContactPosition = vAvgPos;
              pInfo->m_sRollInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab;
            }
          }
        }
      }
    }
  }
}

void WJoltContactEvents::OnContact_SlideReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal0)
{
  WVec3 vVelocity[2] = {WVec3::MakeZero(), WVec3::MakeZero()};

  {
    vVelocity[0] = WJoltConversionUtils::ToVec3(body0.GetLinearVelocity());

    if (!vVelocity[0].IsValid())
      vVelocity[0].SetZero();
  }

  {
    vVelocity[1] = WJoltConversionUtils::ToVec3(body1.GetLinearVelocity());

    if (!vVelocity[1].IsValid())
      vVelocity[1].SetZero();
  }

  const WVec3 vRelativeVelocity = vVelocity[1] - vVelocity[0];

  if (!vRelativeVelocity.IsZero(0.0001f))
  {
    const WVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

    WVec3 vAvgNormal = vAvgNormal0;
    vAvgNormal.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

    // an object is only 'sliding' if it moves at roughly 90 degree along another object
    constexpr float slideAngle = 0.17f; // WMath ::Cos(WAngle::MakeFromDegree(80));

    if (WMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < slideAngle)
    {
      constexpr float slideSpeedThreshold = 0.5f; // in meters per second

      if (vRelativeVelocity.GetLengthSquared() > WMath::Square(slideSpeedThreshold))
      {
        WBitflags<WOnJoltContact> contactFlags[2] = {onContact0, onContact1};
        const JPH::Body* bodies[2] = {&body0, &body1};
        const JPH::SubShapeID shapeIds[2] = {manifold.mSubShapeID1, manifold.mSubShapeID2};

        for (WUInt32 i = 0; i < 2; ++i)
        {
          if (contactFlags[i].IsAnySet(WOnJoltContact::SlideReactions))
          {
            const WJoltMaterial* pMaterial = static_cast<const WJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

            if (pMaterial && pMaterial->m_pSurface)
            {
              if (!pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab.IsEmpty())
              {
                W_LOCK(m_Mutex);

                if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
                {
                  if (!pInfo->m_bStillRolling)
                  {
                    pInfo->m_bStillSliding = true;
                    pInfo->m_vContactPosition = vAvgPos;
                    pInfo->m_sSlideInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void WJoltContactEvents::OnContact_SlideAndRollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal, WBitflags<WOnJoltContact> combinedContactFlags)
{
  if (combinedContactFlags.IsAnySet(WOnJoltContact::SlideReactions) && manifold.mRelativeContactPointsOn1.size() >= 2)
  {
    OnContact_SlideReaction(body0, body1, manifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }

  if (combinedContactFlags.IsAnySet(WOnJoltContact::AllRollReactions))
  {
    OnContact_RollReaction(body0, body1, manifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }
}

JPH::SoftBodyValidateResult WJoltSoftBodyContactListener::OnSoftBodyContactValidate(const JPH::Body& softBody, const JPH::Body& otherBody, JPH::SoftBodyContactSettings& ref_settings)
{
  // give the rigid body "infinite mass" -> soft bodies can't push them
  // so the interaction is always one sided
  ref_settings.mInvMassScale2 = 0.0f;
  ref_settings.mInvInertiaScale2 = 0.0f;

  return JPH::SoftBodyValidateResult::AcceptContact;
}

W_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltContacts);
