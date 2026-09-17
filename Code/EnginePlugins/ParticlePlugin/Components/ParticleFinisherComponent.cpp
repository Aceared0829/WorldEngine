#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

WParticleFinisherComponentManager::WParticleFinisherComponentManager(WWorld* pWorld)
  : SUPER(pWorld)
{
}

void WParticleFinisherComponentManager::UpdateBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdateBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WParticleFinisherComponent, 1, WComponentMode::Static)
{
  W_BEGIN_ATTRIBUTES
  {
    new WHiddenAttribute,
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgInterruptPlaying, OnMsgInterruptPlaying),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE
// clang-format on

WParticleFinisherComponent::WParticleFinisherComponent() = default;
WParticleFinisherComponent::~WParticleFinisherComponent() = default;

void WParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();

  WRenderComponent::OnDeactivated();
}

WResult WParticleFinisherComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.GetBoundingVolume(ref_bounds);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WParticleFinisherComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  switch (msg.m_pView->GetCameraUsageHint())
  {
    case WCameraUsageHint::Shadow:
    case WCameraUsageHint::Reflection:
      return;

    default:
      break;
  }

  m_EffectController.ExtractRenderData(msg, GetOwner()->GetGlobalTransform());
}

void WParticleFinisherComponent::UpdateBounds()
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.CombineSystemBoundingVolumes();

    // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
    // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
    GetOwner()->UpdateLocalBounds();
    GetOwner()->UpdateGlobalBounds();
  }
  else
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}

void WParticleFinisherComponent::OnMsgInterruptPlaying(WMsgInterruptPlaying& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.StopImmediate();
  }
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleFinisherComponent);
