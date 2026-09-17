#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WJoltConstraintComponent, 2)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    W_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Constraints"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WJoltMsgDisconnectConstraints, OnJoltMsgDisconnectConstraints),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_ABSTRACT_COMPONENT_TYPE

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltConstraintLimitMode, 1)
  W_ENUM_CONSTANTS(WJoltConstraintLimitMode::NoLimit, WJoltConstraintLimitMode::HardLimit/*, WJoltConstraintLimitMode::SoftLimit*/)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WJoltConstraintDriveMode, 1)
  W_ENUM_CONSTANTS(WJoltConstraintDriveMode::NoDrive, WJoltConstraintDriveMode::DriveVelocity, WJoltConstraintDriveMode::DrivePosition)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WJoltConstraintComponent::WJoltConstraintComponent() = default;
WJoltConstraintComponent::~WJoltConstraintComponent() = default;

void WJoltConstraintComponent::BreakConstraint()
{
  if (m_pConstraint == nullptr)
    return;

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  pModule->m_BreakableConstraints.Remove(GetHandle());

  // wake up the joined bodies, so that removing a constraint doesn't let them hang in the air
  {
    JPH::BodyID bodies[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(JPH::BodyID::cInvalidBodyID)};
    WInt32 iBodies = 0;

    if (!m_hActorA.IsInvalidated())
    {
      WGameObject* pObject = nullptr;
      WJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorA, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (!m_hActorB.IsInvalidated())
    {
      WGameObject* pObject = nullptr;
      WJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorB, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (iBodies > 0)
    {
      pModule->GetJoltSystem()->GetBodyInterface().ActivateBodies(bodies, iBodies);
    }
  }

  m_pConstraint->Release();
  m_pConstraint = nullptr;
}

void WJoltConstraintComponent::SetBreakForce(float value)
{
  m_fBreakForce = value;
  QueueApplySettings();
}

void WJoltConstraintComponent::SetBreakTorque(float value)
{
  m_fBreakTorque = value;
  QueueApplySettings();
}

void WJoltConstraintComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void WJoltConstraintComponent::OnSimulationStarted()
{
  WUInt32 uiBodyIdA = WInvalidIndex;
  WUInt32 uiBodyIdB = WInvalidIndex;

  WJoltDynamicActorComponent* pRbParent = nullptr;
  WJoltDynamicActorComponent* pRbChild = nullptr;

  if (FindParentBody(uiBodyIdA, pRbParent).Failed())
    return;

  if (FindChildBody(uiBodyIdB, pRbChild).Failed())
    return;

  if (uiBodyIdB == WInvalidIndex)
    return;

  if (uiBodyIdA == uiBodyIdB)
  {
    WLog::Error("Constraint can't be linked to the same body twice");
    return;
  }

  m_LocalFrameA.m_qRotation.Normalize();
  m_LocalFrameB.m_qRotation.Normalize();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  {
    JPH::BodyID bodies[2] = {JPH::BodyID(uiBodyIdA), JPH::BodyID(uiBodyIdB)};
    JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

    if (uiBodyIdB != WInvalidIndex && bodyLock.GetBody(1) != nullptr)
    {
      if (uiBodyIdA != WInvalidIndex && bodyLock.GetBody(0) != nullptr)
      {
        CreateContstraintType(bodyLock.GetBody(0), bodyLock.GetBody(1));

        pModule->EnableJoinedBodiesCollisions(bodyLock.GetBody(0)->GetCollisionGroup().GetGroupID(), bodyLock.GetBody(1)->GetCollisionGroup().GetGroupID(), m_bPairCollision);
      }
      else
      {
        CreateContstraintType(&JPH::Body::sFixedToWorld, bodyLock.GetBody(1));
      }
    }
  }

  if (m_pConstraint)
  {
    m_pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
    ApplySettings();

    if (pRbParent)
    {
      pRbParent->AddConstraint(GetHandle());
    }

    if (pRbChild)
    {
      pRbChild->AddConstraint(GetHandle());
    }
  }
}

void WJoltConstraintComponent::OnDeactivated()
{
  BreakConstraint();

  SUPER::OnDeactivated();
}

void WJoltConstraintComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bPairCollision;

  inout_stream.WriteGameObjectHandle(m_hActorA);
  inout_stream.WriteGameObjectHandle(m_hActorB);

  s << m_LocalFrameA;
  s << m_LocalFrameB;

  inout_stream.WriteGameObjectHandle(m_hActorBAnchor);

  s << m_fBreakForce;
  s << m_fBreakTorque;
}

void WJoltConstraintComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bPairCollision;

  m_hActorA = inout_stream.ReadGameObjectHandle();
  m_hActorB = inout_stream.ReadGameObjectHandle();

  s >> m_LocalFrameA;
  s >> m_LocalFrameB;

  m_hActorBAnchor = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_fBreakForce;
    s >> m_fBreakTorque;
  }
}

void WJoltConstraintComponent::SetParentActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetParentActor(resolver(szReference, GetHandle(), "ParentActor"));
}

void WJoltConstraintComponent::SetChildActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetChildActor(resolver(szReference, GetHandle(), "ChildActor"));
}

void WJoltConstraintComponent::SetChildActorAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = resolver(szReference, GetHandle(), "ChildActorAnchor");
}

void WJoltConstraintComponent::SetParentActor(WGameObjectHandle hActor)
{
  SetUserFlag(0, false); // local frame A is not valid
  m_hActorA = hActor;
}

void WJoltConstraintComponent::SetChildActor(WGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorB = hActor;
}

void WJoltConstraintComponent::SetChildActorAnchor(WGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = hActor;
}

void WJoltConstraintComponent::SetActors(WGameObjectHandle hActorA, const WTransform& localFrameA, WGameObjectHandle hActorB, const WTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  // local frame A and B are already valid
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_LocalFrameA = localFrameA;
  m_LocalFrameB = localFrameB;
}

void WJoltConstraintComponent::ApplySettings()
{
  SetUserFlag(2, false);

  if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  {
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->m_BreakableConstraints.Insert(GetHandle());
  }
  else
  {
    WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
    pModule->m_BreakableConstraints.Remove(GetHandle());
  }
}

void WJoltConstraintComponent::OnJoltMsgDisconnectConstraints(WJoltMsgDisconnectConstraints& ref_msg)
{
  BreakConstraint();
}

WResult WJoltConstraintComponent::FindParentBody(WUInt32& out_uiJoltBodyID, WJoltDynamicActorComponent*& pRbComp)
{
  WGameObject* pObject = nullptr;
  pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject) || !pObject->IsActive())
    {
      WLog::Error("{0} '{1}' parent reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return W_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      WLog::Error("{0} '{1}' parent reference is an object without a WJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return W_FAILURE;
    }
  }
  else
  {
    pObject = GetOwner();

    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pRbComp))
        break;

      pObject = pObject->GetParent();
    }

    if (pRbComp == nullptr)
    {
      out_uiJoltBodyID = WInvalidIndex;

      if (GetUserFlag(0) == false)
      {
        // m_localFrameA is now valid
        SetUserFlag(0, true);
        m_LocalFrameA = GetOwner()->GetGlobalTransform();
      }
      return W_SUCCESS;
    }
    else
    {
      W_ASSERT_DEBUG(pObject != nullptr, "pRbComp and pObject should always be valid together");
      if (GetUserFlag(0) == true)
      {
        WTransform globalFrame = m_LocalFrameA;

        // m_localFrameA is already valid
        // assume it was in global space and move it into local space of the found parent
        m_LocalFrameA = WTransform::MakeLocalTransform(pRbComp->GetOwner()->GetGlobalTransform(), globalFrame);
        m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
      }
    }
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == WInvalidIndex)
  {
    WLog::Error("{0} '{1}' parent reference is an object with an invalid WJoltDynamicActorComponent. Constraint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return W_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_LocalFrameA = WTransform::MakeLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return W_SUCCESS;
}

WResult WJoltConstraintComponent::FindChildBody(WUInt32& out_uiJoltBodyID, WJoltDynamicActorComponent*& pRbComp)
{
  WGameObject* pObject = nullptr;
  pRbComp = nullptr;

  if (m_hActorB.IsInvalidated())
  {
    WLog::Error("{0} '{1}' has no child reference. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return W_FAILURE;
  }

  if (!GetWorld()->TryGetObject(m_hActorB, pObject) || !pObject->IsActive())
  {
    WLog::Error("{0} '{1}' child reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return W_FAILURE;
  }

  if (!pObject->TryGetComponentOfBaseType(pRbComp))
  {
    // this makes it possible to link the Constraint to a prefab, because it may skip the top level hierarchy of the prefab
    pObject = pObject->SearchForChildByNameSequence("/", WGetStaticRTTI<WJoltDynamicActorComponent>());

    if (pObject == nullptr)
    {
      WLog::Error("{0} '{1}' child reference is an object without a WJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return W_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      W_REPORT_FAILURE("Component should exist.");
    }
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == WInvalidIndex)
  {
    WLog::Error("{0} '{1}' child reference is an object with an invalid WJoltDynamicActorComponent. Constraint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return W_FAILURE;
  }

  m_hActorB = pObject->GetHandle();

  if (GetUserFlag(1) == false)
  {
    WGameObject* pAnchorObject = GetOwner();

    if (!m_hActorBAnchor.IsInvalidated())
    {
      if (!GetWorld()->TryGetObject(m_hActorBAnchor, pAnchorObject))
      {
        WLog::Error("{0} '{1}' anchor reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
        return W_FAILURE;
      }
    }

    // m_localFrameB is now valid
    SetUserFlag(1, true);
    m_LocalFrameB = WTransform::MakeLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
    m_LocalFrameB.m_vPosition = m_LocalFrameB.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return W_SUCCESS;
}

WTransform WJoltConstraintComponent::ComputeParentBodyGlobalFrame() const
{
  if (!m_hActorA.IsInvalidated())
  {
    const WGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorA, pObject))
    {
      WTransform res;
      res = WTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameA);
      return res;
    }
  }

  return m_LocalFrameA;
}

WTransform WJoltConstraintComponent::ComputeChildBodyGlobalFrame() const
{
  if (!m_hActorB.IsInvalidated())
  {
    const WGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorB, pObject))
    {
      WTransform res;
      res = WTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameB);
      return res;
    }
  }

  return m_LocalFrameB;
}

void WJoltConstraintComponent::QueueApplySettings()
{
  if (m_pConstraint == nullptr)
    return;

  // already in queue ?
  if (GetUserFlag(2))
    return;

  SetUserFlag(2, true);

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();
  pModule->m_RequireUpdate.PushBack(GetHandle());
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConstraintComponent);
