#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MiniAudioPlugin/Components/MiniAudioListenerComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>

WMiniAudioListenerComponentManager::WMiniAudioListenerComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

void WMiniAudioListenerComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WMiniAudioListenerComponentManager::UpdateListeners, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WMiniAudioListenerComponentManager::UpdateListeners(const WWorldModule::UpdateContext& context)
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
W_BEGIN_COMPONENT_TYPE(WMiniAudioListenerComponent, 1, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Sound/MiniAudio"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMiniAudioListenerComponent::WMiniAudioListenerComponent() = default;
WMiniAudioListenerComponent::~WMiniAudioListenerComponent() = default;

void WMiniAudioListenerComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
}

void WMiniAudioListenerComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
}

void WMiniAudioListenerComponent::Update()
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * WVec3::MakeAxisX()).GetNormalized();
  const auto up = (GetOwner()->GetGlobalRotation() * WVec3::MakeAxisZ()).GetNormalized();

  WMiniAudioSingleton::GetSingleton()->SetListener(0, pos, fwd, up, vel);
}

