#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/BlackboardComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAnimationControllerComponent, 4, WComponentMode::Static);
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("AnimGraph", m_hAnimGraph)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph"), new WRequiredAttribute()),

    W_ENUM_MEMBER_PROPERTY("RootMotionMode", WRootMotionMode, m_RootMotionMode),
    W_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", WAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
    W_MEMBER_PROPERTY("EnableIK", m_bEnableIK),
    W_ARRAY_MEMBER_PROPERTY("AnimationClipOverrides", m_AnimationClipOverrides),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetAnimationClipOverride, In, "sAnimationName", In, "sAnimationClipResource"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WAnimationControllerComponent::WAnimationControllerComponent() = default;
WAnimationControllerComponent::~WAnimationControllerComponent() = default;

void WAnimationControllerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hAnimGraph;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
  s << m_bEnableIK;

  s << m_AnimationClipOverrides.GetCount();
  for (const auto& clip : m_AnimationClipOverrides)
  {
    s << clip.m_sClipName;
    s << clip.m_hClip;
  }
}

void WAnimationControllerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hAnimGraph;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }

  if (uiVersion >= 3)
  {
    s >> m_bEnableIK;
  }

  if (uiVersion >= 4)
  {
    WUInt32 uiNumOverrides = 0;
    s >> uiNumOverrides;
    m_AnimationClipOverrides.SetCount(uiNumOverrides);

    for (WUInt32 i = 0; i < uiNumOverrides; ++i)
    {
      s >> m_AnimationClipOverrides[i].m_sClipName;
      s >> m_AnimationClipOverrides[i].m_hClip;
    }
  }
}

void WAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimGraph.IsValid())
    return;

  WMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  m_AnimController.Initialize(msg.m_hSkeleton, m_PoseGenerator, WBlackboardComponent::FindBlackboard(*GetOwner()));
  m_AnimController.AddAnimGraph(m_hAnimGraph);

  for (const auto& clip : m_AnimationClipOverrides)
  {
    WAnimController::AnimClipInfo info;
    info.m_hClip = clip.m_hClip;
    m_AnimController.SetAnimationClipInfo(clip.m_sClipName, info);
  }
}

void WAnimationControllerComponent::SetAnimationClipOverride(WStringView sAnimationName, WStringView sAnimationClipResource)
{
  WAnimController::AnimClipInfo info;
  info.m_hClip = WResourceManager::LoadResource<WAnimationClipResource>(sAnimationClipResource);

  WHashedString sName;
  sName.Assign(sAnimationName);

  m_AnimController.SetAnimationClipInfo(sName, info);
}

void WAnimationControllerComponent::Update()
{
  WTime tMinStep = WTime::MakeFromSeconds(0);
  WVisibilityState::Enum visType = GetOwner()->GetVisibilityState();

  if (visType != WVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == WAnimationInvisibleUpdateRate::Pause && visType == WVisibilityState::Invisible)
      return;

    tMinStep = WAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  W_PROFILE_SCOPE("WAnimationControllerComponent::Update");

  if (!m_AnimController.Update(m_ElapsedTimeSinceUpdate, GetOwner(), m_bEnableIK))
  {
    // if there is an error, OR something else completely took over the animation (usually a ragdoll)
    // disable this component
    SetActiveFlag(false);
  }

  m_ElapsedTimeSinceUpdate = WTime::MakeZero();

  m_AnimController.GetRootMotion(m_vPendingTranslation, m_PendingRotationX, m_PendingRotationY, m_PendingRotationZ);
}

void WAnimationControllerComponent::ApplyRootMotion()
{
  WRootMotionMode::Apply(m_RootMotionMode, GetOwner(), m_vPendingTranslation, m_PendingRotationX, m_PendingRotationY, m_PendingRotationZ);
  m_vPendingTranslation = WVec3::MakeZero();
  m_PendingRotationX = m_PendingRotationY = m_PendingRotationZ = WAngle();
}

//////////////////////////////////////////////////////////////////////////


WAnimationControllerComponentManager::WAnimationControllerComponentManager(WWorld* pWorld)
  : WComponentManager<class WAnimationControllerComponent, WBlockStorageType::FreeList>(pWorld)
{
}

void WAnimationControllerComponentManager::Initialize()
{
  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WAnimationControllerComponentManager::Update, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_uiAsyncPhaseBatchSize = 2;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WAnimationControllerComponentManager::ApplyRootMotion, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_Phase = WWorldUpdatePhase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }

  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WAnimationControllerComponentManager::ResourceEvent, this));
}

void WAnimationControllerComponentManager::Deinitialize()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WAnimationControllerComponentManager::ResourceEvent, this));
}

void WAnimationControllerComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  {
    for (auto hComponent : m_ComponentsToReset)
    {
      WAnimationControllerComponent* pComp = nullptr;
      if (GetWorld()->TryGetComponent(hComponent, pComp))
      {
        pComp->OnSimulationStarted(); // just run this again
      }
    }

    m_ComponentsToReset.Clear();
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void WAnimationControllerComponentManager::ResourceEvent(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading)
  {
    if (e.m_pResource->GetDynamicRTTI() == WGetStaticRTTI<WAnimGraphResource>())
    {
      WAnimGraphResourceHandle hResource((WAnimGraphResource*)(e.m_pResource));

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (!it->IsActiveAndSimulating())
          continue;

        if (it->m_hAnimGraph == hResource)
        {
          if (!m_ComponentsToReset.Contains(it->GetHandle()))
          {
            m_ComponentsToReset.PushBack(it->GetHandle());
          }
        }
      }
    }
  }
}

void WAnimationControllerComponentManager::ApplyRootMotion(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->m_RootMotionMode != WRootMotionMode::Ignore && pComponent->IsActiveAndInitialized())
    {
      pComponent->ApplyRootMotion();
    }
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
