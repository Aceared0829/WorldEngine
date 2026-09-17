#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>

WFmodListenerComponentManager::WFmodListenerComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

void WFmodListenerComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WFmodListenerComponentManager::UpdateListeners, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WFmodListenerComponentManager::UpdateListeners(const WWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WFmodListenerComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ListenerIndex", m_uiListenerIndex),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WFmodListenerComponent::WFmodListenerComponent() = default;
WFmodListenerComponent::~WFmodListenerComponent() = default;

void WFmodListenerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_uiListenerIndex;
}

void WFmodListenerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiListenerIndex;
}

void WFmodListenerComponent::Update()
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * WVec3::MakeAxisX()).GetNormalized();
  const auto up = (GetOwner()->GetGlobalRotation() * WVec3::MakeAxisZ()).GetNormalized();

  WFmod::GetSingleton()->SetListener(m_uiListenerIndex, pos, fwd, up, vel);
}

W_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodListenerComponent);
