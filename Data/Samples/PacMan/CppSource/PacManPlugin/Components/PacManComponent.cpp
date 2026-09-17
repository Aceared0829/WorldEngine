#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PacManPlugin/Components/PacManComponent.h>
#include <PacManPlugin/GameState/PacManGameState.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(PacManComponent, 1 /* version */, WComponentMode::Dynamic) // 'Dynamic' because we want to change the owner's transform
{
  // if we wanted to show properties in the editor, we would need to register them here
  //W_BEGIN_PROPERTIES
  //{
  //  W_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 10)),
  //}
  //W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("PacMan"), // Component menu group
  }
  W_END_ATTRIBUTES;

  // declare the message handlers that we have, so that messages can be delivered to us
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgInputActionTriggered, OnMsgInputActionTriggered),
    W_MESSAGE_HANDLER(WMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

PacManComponent::PacManComponent() = default;
PacManComponent::~PacManComponent() = default;

void PacManComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // preload prefabs needed later
  {
    m_hCollectCoinEffect = WResourceManager::LoadResource<WPrefabResource>("{ 9b006872-70ba-4086-8ce5-244304032851 }"); // GUID of the prefab copied in the editor
    WResourceManager::PreloadResource(m_hCollectCoinEffect);

    m_hLoseGameEffect = WResourceManager::LoadResource<WPrefabResource>("{ 02314d71-f49e-45f3-89e2-ce4b7b1cba09 }"); // GUID copied in the editor
    WResourceManager::PreloadResource(m_hLoseGameEffect);
  }

  m_pStateBlackboard = WBlackboard::GetOrCreateGlobal(PacManGameState::s_sStats);

  // store the start state of PacMan in the global blackboard
  m_pStateBlackboard->SetEntryValue(PacManGameState::s_sPacManState, PacManState::Alive);
  m_pStateBlackboard->SetEntryValue(PacManGameState::s_sCoinsEaten, 0);
}

void PacManComponent::Update()
{
  // this function is called once per frame

  if (auto pBlackboard = WBlackboard::FindGlobal(PacManGameState::s_sStats))
  {
    // retrieve the current state of PacMan
    const PacManState state = static_cast<PacManState>(pBlackboard->GetEntryValue(PacManGameState::s_sPacManState, PacManState::Alive).Get<WInt32>());

    // if it was eaten by a ghost recently, play the lose effect and delete PacMan
    if (state == PacManState::EatenByGhost)
    {
      // access the lose game effect
      // the prefab may still be loading (though very unlikely)
      // only if it is fully loaded, do we instantiate it, in all other cases we just ignore it
      WPrefabResource::InstantiatePrefab(m_hLoseGameEffect, true, *GetWorld(), GetOwner()->GetGlobalTransform());

      // since we lost, just delete the PacMan object altogether at the end of the frame
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());

      return;
    }
  }

  // if PacMan is still alive, update his movement

  bool bWall[4] = {false, false, false, false};

  if (WPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>())
  {
    // access the physics system for raycasts

    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = WPhysicsShapeType::Static; // we only want to hit static geometry, and ignore dynamic/kinematic objects like the other ghosts and the coins

    WVec3 pos = GetOwner()->GetGlobalPosition();
    pos.z += 0.5f;
    const float dist = 1.0f;
    const float off = 0.35f;
    const WVec3 offx(off, 0.0f, 0.0f);
    const WVec3 offy(0.0f, off, 0.0f);

    // do two raycasts into each of the 4 directions, and use a slight offset
    // this way, if both raycasts into one direction hit nothing, we know that there is enough free space, that we can now change direction
    // if only one raycast hits nothing, we need to travel further, otherwise we bump into a corner

    //    ^ ^
    //    | |
    // <--+ +-->
    //     P
    // <--+ +-->
    //    | |
    //    V V

    bWall[0] |= pPhysics->Raycast(res, pos - offy, WVec3(1, 0, 0), dist, params);
    bWall[0] |= pPhysics->Raycast(res, pos + offy, WVec3(1, 0, 0), dist, params);

    bWall[1] |= pPhysics->Raycast(res, pos - offx, WVec3(0, 1, 0), dist, params);
    bWall[1] |= pPhysics->Raycast(res, pos + offx, WVec3(0, 1, 0), dist, params);

    bWall[2] |= pPhysics->Raycast(res, pos - offy, WVec3(-1, 0, 0), dist, params);
    bWall[2] |= pPhysics->Raycast(res, pos + offy, WVec3(-1, 0, 0), dist, params);

    bWall[3] |= pPhysics->Raycast(res, pos - offx, WVec3(0, -1, 0), dist, params);
    bWall[3] |= pPhysics->Raycast(res, pos + offx, WVec3(0, -1, 0), dist, params);
  }

  // if there is no wall in the direction that the player wants PacMan to go, we can safely switch direction now
  // this makes PacMan much easier to steer around the maze, than if we were to change directly immediately
  // since it prevents getting stuck on some corner
  if (!bWall[m_TargetDirection])
  {
    m_Direction = m_TargetDirection;
  }

  // now just change the rotation of PacMan to point into the current direction
  WQuat rotation = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(static_cast<float>(m_Direction * 90)));
  GetOwner()->SetGlobalRotation(rotation);

  // and communicate to the character controller component, that it should move forwards at a fixed speed
  WMsgMoveCharacterController msg;
  msg.m_fMoveForwards = 2.0f;
  GetOwner()->SendMessage(msg);
}

void PacManComponent::OnMsgInputActionTriggered(WMsgInputActionTriggered& msg)
{
  // this is called every time the input component detects that there is relevant input state
  // see https://ezengine.net/pages/docs/input/input-component.html

  if (msg.m_TriggerState != WTriggerState::Continuing)
    return;

  if (msg.m_sInputAction.GetString() == "Up")
    m_TargetDirection = WalkDirection::Up;

  if (msg.m_sInputAction.GetString() == "Down")
    m_TargetDirection = WalkDirection::Down;

  if (msg.m_sInputAction.GetString() == "Left")
    m_TargetDirection = WalkDirection::Left;

  if (msg.m_sInputAction.GetString() == "Right")
    m_TargetDirection = WalkDirection::Right;
}

void PacManComponent::OnMsgTriggerTriggered(WMsgTriggerTriggered& msg)
{
  // this is called every time a trigger component (on this object) detects overlap with another physics object
  // see https://ezengine.net/pages/docs/physics/jolt/actors/jolt-trigger-component.html
  // we have two triggers on our PacMan, one to detect Ghosts, another to detect Coins
  // we could achieve the same thing with just one trigger, though

  if (msg.m_TriggerState != WTriggerState::Activated)
    return;

  // the "Ghost" trigger had an overlap
  if (msg.m_sMessage.GetString() == "Ghost")
  {
    m_pStateBlackboard->SetEntryValue(PacManGameState::s_sPacManState, PacManState::EatenByGhost);
    return;
  }

  // the "Pickup" trigger had an overlap
  if (msg.m_sMessage.GetString() == "Pickup")
  {
    // use the handle to the game object that activated the trigger, to get a pointer to the game object
    WGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hTriggeringObject, pObject))
    {
      // if this was a coin, pick it up
      if (pObject->GetName() == "Coin")
      {
        // just delete the coin object at the end of the frame
        GetWorld()->DeleteObjectDelayed(pObject->GetHandle());

        WVariant ceValue = m_pStateBlackboard->GetEntryValue(PacManGameState::s_sCoinsEaten, 0).Get<WInt32>() + 1;
        m_pStateBlackboard->SetEntryValue(PacManGameState::s_sCoinsEaten, ceValue);

        // spawn a collect coin effect
        WPrefabResource::InstantiatePrefab(m_hCollectCoinEffect, false, *GetWorld(), pObject->GetGlobalTransform());
      }
    }
  }
}

void PacManComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // currently we have nothing to serialize
}

void PacManComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  // currently we have nothing to deserialize
}


W_STATICLINK_FILE(PacManPlugin, PacManPlugin_Components_PacManComponent);
