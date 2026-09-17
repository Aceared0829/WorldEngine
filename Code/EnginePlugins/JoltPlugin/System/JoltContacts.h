#pragma once

#include <Core/World/Declarations.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/SoftBody/SoftBodyContactListener.h>

class WWorld;
class WJoltTriggerComponent;
class WJoltContactEvents;
class WSurfaceResource;
struct WOnJoltContact;

namespace JPH
{
  class SubShapeIDPair;
  class ContactSettings;
  class ContactManifold;
  class Body;
} // namespace JPH

class WJoltContactEvents
{
public:
  struct InteractionContact
  {
    WVec3 m_vPosition;
    WVec3 m_vNormal;
    const WSurfaceResource* m_pSurface;
    WTempHashedString m_sInteraction;
    float m_fImpulseSqr;
    float m_fDistanceSqr;
  };

  struct SlideAndRollInfo
  {
    const JPH::Body* m_pBody = nullptr;
    bool m_bStillSliding = false;
    bool m_bStillRolling = false;

    float m_fDistanceSqr;
    WVec3 m_vContactPosition;
    WGameObjectHandle m_hSlidePrefab;
    WGameObjectHandle m_hRollPrefab;
    WHashedString m_sSlideInteractionPrefab;
    WHashedString m_sRollInteractionPrefab;
  };

  WMutex m_Mutex;
  WWorld* m_pWorld = nullptr;
  WVec3 m_vMainCameraPosition = WVec3::MakeZero();
  WHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  WHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;

  SlideAndRollInfo* FindSlideOrRollInfo(const JPH::Body* pBody, const WVec3& vAvgPos);

  void OnContact_SlideReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal);

  void OnContact_RollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal0);

  void OnContact_ImpactReaction(const WVec3& vAvgPos, const WVec3& vAvgNormal, float fMaxImpactSqr, const WSurfaceResource* pSurface1, const WSurfaceResource* pSurface2, bool bActor1StaticOrKinematic);
  void OnContact_SlideAndRollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, WBitflags<WOnJoltContact> onContact0, WBitflags<WOnJoltContact> onContact1, const WVec3& vAvgPos, const WVec3& vAvgNormal, WBitflags<WOnJoltContact> combinedContactFlags);

  void SpawnPhysicsImpactReactions();
  void UpdatePhysicsSlideReactions();
  void UpdatePhysicsRollReactions();
};

class WJoltContactListener : public JPH::ContactListener
{
public:
  WWorld* m_pWorld = nullptr;
  WJoltContactEvents m_ContactEvents;

  struct TriggerObj
  {
    const WJoltTriggerComponent* m_pTrigger = nullptr;
    WGameObjectHandle m_hTarget;
    WInt32 m_iTriggerCount = 0;
  };

  WMutex m_TriggerMutex;
  WMap<WUInt64, TriggerObj> m_Trigs;

  void RemoveTrigger(const WJoltTriggerComponent* pTrigger);

  virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;
  virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;

  virtual void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;

  void OnContact(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings, bool bPersistent, bool bIsDebrisContact);

  bool ActivateTrigger(const JPH::Body& body1, const JPH::Body& body2, WUInt64 uiBody1id, WUInt64 uiBody2id);

  void DeactivateTrigger(WUInt64 uiBody1id, WUInt64 uiBody2id);
};

class WJoltSoftBodyContactListener : public JPH::SoftBodyContactListener
{
public:
  virtual JPH::SoftBodyValidateResult OnSoftBodyContactValidate(const JPH::Body& softBody, const JPH::Body& otherBody, JPH::SoftBodyContactSettings& ref_settings) override;
};
