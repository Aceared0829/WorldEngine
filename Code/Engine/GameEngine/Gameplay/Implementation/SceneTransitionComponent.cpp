#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Gameplay/SceneTransitionComponent.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSceneLoadMode, 1)
  W_ENUM_CONSTANT(WSceneLoadMode::None),
  W_ENUM_CONSTANT(WSceneLoadMode::LoadAndSwitch),
  W_ENUM_CONSTANT(WSceneLoadMode::Preload),
  W_ENUM_CONSTANT(WSceneLoadMode::CancelPreload),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WSceneTransitionComponent, 1 /* version */, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Mode", WSceneLoadMode, m_Mode),
    W_MEMBER_PROPERTY("TargetScene", m_sTargetScene)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Scene",  WDependencyFlags::Package), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("PreloadCollection", m_sPreloadCollectionFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_AssetCollection", WDependencyFlags::Package)),
    W_MEMBER_PROPERTY("SpawnPoint", m_sSpawnPoint),
    W_MEMBER_PROPERTY("RelativeSpawnPosition", m_bRelativeSpawnPosition)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  W_END_MESSAGEHANDLERS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(StartTransition, In, "PositionOffset", In, "RotationOffset"),
    W_SCRIPT_FUNCTION_PROPERTY(StartTransitionWithOffsetTo, In, "GlobalPosition", In, "GlobalRotation"),
    W_SCRIPT_FUNCTION_PROPERTY(StartPreload),
    W_SCRIPT_FUNCTION_PROPERTY(CancelPreload),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WSceneTransitionComponent::WSceneTransitionComponent() = default;
WSceneTransitionComponent::~WSceneTransitionComponent() = default;

void WSceneTransitionComponent::StartTransition(const WVec3& vPositionOffset, const WQuat& qRotationOffset)
{
  if (auto pGameStateBase = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    // we could move these functions into WGameStateBase, but for now the dynamic cast should be fine
    // there is no good reason to have this functionality on the base class
    if (WGameState* pGameState = WDynamicCast<WGameState*>(pGameStateBase))
    {
      pGameState->LoadScene(m_sTargetScene, m_sPreloadCollectionFile, m_sSpawnPoint, WTransform(vPositionOffset, qRotationOffset));
    }
  }
}

void WSceneTransitionComponent::StartTransitionWithOffsetTo(const WVec3& vGlobalPosition, const WQuat& qGlobalRotation)
{
  const WTransform ownGlobal(vGlobalPosition, qGlobalRotation);
  const WTransform rel = WTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), ownGlobal);

  StartTransition(rel.m_vPosition, rel.m_qRotation);
}

void WSceneTransitionComponent::StartPreload()
{
  if (auto pGameStateBase = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    if (WGameState* pGameState = WDynamicCast<WGameState*>(pGameStateBase))
    {
      pGameState->StartBackgroundSceneLoading(m_sTargetScene, m_sPreloadCollectionFile);
    }
  }
}

void WSceneTransitionComponent::CancelPreload()
{
  if (auto pGameStateBase = WGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    if (WGameState* pGameState = WDynamicCast<WGameState*>(pGameStateBase))
    {
      pGameState->CancelBackgroundSceneLoading();
    }
  }
}

void WSceneTransitionComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Mode;
  s << m_sTargetScene;
  s << m_sSpawnPoint;
  s << m_bRelativeSpawnPosition;
  s << m_sPreloadCollectionFile;
}

void WSceneTransitionComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Mode;
  s >> m_sTargetScene;
  s >> m_sSpawnPoint;
  s >> m_bRelativeSpawnPosition;
  s >> m_sPreloadCollectionFile;
}

void WSceneTransitionComponent::OnMsgTriggerTriggered(WMsgTriggerTriggered& ref_msg)
{
  if (ref_msg.m_TriggerState == WTriggerState::Activated)
  {
    if (m_Mode == WSceneLoadMode::None)
    {
      return;
    }

    if (m_Mode == WSceneLoadMode::LoadAndSwitch)
    {
      WTransform rel = WTransform::MakeIdentity();

      if (m_bRelativeSpawnPosition)
      {
        WGameObject* pPlayer;
        if (GetWorld()->TryGetObject(ref_msg.m_hTriggeringObject, pPlayer))
        {
          rel = WTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), pPlayer->GetGlobalTransform());
        }
      }

      StartTransition(rel.m_vPosition, rel.m_qRotation);

      return;
    }

    if (m_Mode == WSceneLoadMode::Preload)
    {
      StartPreload();
      return;
    }

    if (m_Mode == WSceneLoadMode::CancelPreload)
    {
      CancelPreload();
      return;
    }
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SceneTransitionComponent);
