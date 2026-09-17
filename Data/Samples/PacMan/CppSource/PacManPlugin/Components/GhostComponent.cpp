#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PacManPlugin/Components/GhostComponent.h>
#include <PacManPlugin/GameState/PacManGameState.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(GhostComponent, 1 /* version */, WComponentMode::Dynamic) // 'Dynamic' because we want to change the owner's transform
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(2.0f), new WClampValueAttribute(0.1f, 10.0f)),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("PacMan"), // Component menu group
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

GhostComponent::GhostComponent() = default;
GhostComponent::~GhostComponent() = default;

void GhostComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_pStateBlackboard = WBlackboard::GetOrCreateGlobal(PacManGameState::s_sStats);

  // preload our disappear effect for when the player wins
  m_hDisappear = WResourceManager::LoadResource<WPrefabResource>("{ bad55bab-9701-484c-b3f2-90caeb206716 }");
  WResourceManager::PreloadResource(m_hDisappear);
}

void GhostComponent::Update()
{
  // check the blackboard for whether the player just won
  {
    const PacManState state = static_cast<PacManState>(m_pStateBlackboard->GetEntryValue(PacManGameState::s_sPacManState, PacManState::Alive).Get<WInt32>());

    if (state == PacManState::WonGame)
    {
      // create the 'disappear' effect
      WPrefabResource::InstantiatePrefab(m_hDisappear, true, *GetWorld(), GetOwner()->GetGlobalTransform());

      // and delete yourself at the end of the frame
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      return;
    }
  }

  bool bWall[4] = {false, false, false, false};

  if (WPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>())
  {
    // do four raycasts into each direction, to detect which direction would be free to walk into
    // if the forwards direction is blocked, we want the ghost to turn

    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = WPhysicsShapeType::Static;

    WVec3 pos = GetOwner()->GetGlobalPosition();
    pos.z += 0.5f;

    WVec3 dir[4] =
      {
        WVec3(1, 0, 0),
        WVec3(0, 1, 0),
        WVec3(-1, 0, 0),
        WVec3(0, -1, 0),
      };

    WHybridArray<WDebugRendererLine, 4> lines;

    for (WUInt32 i = 0; i < 4; ++i)
    {
      bWall[i] = pPhysics->Raycast(res, pos, dir[i], 0.55f, params);

      auto& l = lines.ExpandAndGetRef();
      l.m_start = pos;
      l.m_end = pos + dir[i] * 0.55f;
      l.m_startColor = l.m_endColor = bWall[i] ? WColor::Red : WColor::Green;
    }

    // could be used to visualize the raycasts
    // WDebugRenderer::DrawLines(GetWorld(), lines, WColor::White);
  }

  WRandom& rng = GetWorld()->GetRandomNumberGenerator();

  // if the direction into which the ghost currently walks is occluded, randomly turn left or right and check again
  while (bWall[m_Direction])
  {
    WUInt8 uiDir = (WUInt8)m_Direction;

    if (bWall[(uiDir + 1) % 4] && bWall[(uiDir + 3) % 4]) // both left and right are blocked -> turn around
    {
      uiDir = (uiDir + 2) % 4;
    }
    else if (bWall[(uiDir + 1) % 4]) // right side is blocked -> turn left
    {
      uiDir = (uiDir + 3) % 4;
    }
    else if (bWall[(uiDir + 3) % 4]) // left side is blocked -> turn right
    {
      uiDir = (uiDir + 1) % 4;
    }
    else
    {
      // otherwise randomly turn left or right

      if (rng.Bool())
        uiDir = (uiDir + 1) % 4;
      else
        uiDir = (uiDir + 3) % 4;
    }

    m_Direction = static_cast<WalkDirection>(uiDir);
  }

  // now just change the rotation of the ghost to point into the current direction
  WQuat rotation = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromDegree(static_cast<float>(m_Direction * 90)));
  GetOwner()->SetGlobalRotation(rotation);

  // and communicate to the character controller component, that it should move forwards at a fixed speed
  WMsgMoveCharacterController msg;
  msg.m_fMoveForwards = m_fSpeed;
  GetOwner()->SendMessage(msg);
}

void GhostComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    WReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization
  }
}

void GhostComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    WReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization
  }
}


W_STATICLINK_FILE(PacManPlugin, PacManPlugin_Components_GhostComponent);
