#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

WJoltTriggerComponentManager::WJoltTriggerComponentManager(WWorld* pWorld)
  : WComponentManager<WJoltTriggerComponent, WBlockStorageType::FreeList>(pWorld)
{
}

WJoltTriggerComponentManager::~WJoltTriggerComponentManager() = default;

void WJoltTriggerComponentManager::UpdateMovingTriggers()
{
  W_PROFILE_SCOPE("MovingTriggers");

  WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
  auto& bodyInterface = pModule->GetJoltSystem()->GetBodyInterface();

  for (auto pTrigger : m_MovingTriggers)
  {
    JPH::BodyID bodyId(pTrigger->m_uiJoltBodyID);

    WSimdTransform trans = pTrigger->GetOwner()->GetGlobalTransformSimd();

    bodyInterface.SetPositionAndRotation(bodyId, WJoltConversionUtils::ToVec3(trans.m_Position), WJoltConversionUtils::ToQuat(trans.m_Rotation), JPH::EActivation::Activate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltTriggerComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage)
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGESENDERS
  {
    W_MESSAGE_SENDER(m_TriggerEventSender)
  }
  W_END_MESSAGESENDERS
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltTriggerComponent::WJoltTriggerComponent() = default;
WJoltTriggerComponent::~WJoltTriggerComponent() = default;

void WJoltTriggerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_sTriggerMessage;
}

void WJoltTriggerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sTriggerMessage;
}

void WJoltTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  WJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, nullptr).Failed())
  {
    WLog::Error("Jolt trigger actor component has no valid shape.");
    return;
  }

  const WSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  bodyCfg.mIsSensor = true;
  bodyCfg.mPosition = WJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = WJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = WJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, WJoltBroadphaseLayer::Trigger);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<WUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  W_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, false);

  if (GetOwner()->IsDynamic())
  {
    WJoltTriggerComponentManager* pManager = static_cast<WJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Insert(this);
  }
}

void WJoltTriggerComponent::OnDeactivated()
{
  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  WJoltContactListener* pContactListener = pModule->GetContactListener();
  pContactListener->RemoveTrigger(this);

  if (GetOwner()->IsDynamic())
  {
    WJoltTriggerComponentManager* pManager = static_cast<WJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Remove(this);
  }

  SUPER::OnDeactivated();
}

void WJoltTriggerComponent::PostTriggerMessage(const WGameObjectHandle& hOtherObject, WTriggerState::Enum triggerState) const
{
  WMsgTriggerTriggered msg;

  msg.m_TriggerState = triggerState;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = hOtherObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), WTime::MakeZero(), WObjectMsgQueueType::PostTransform);
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltTriggerComponent);
