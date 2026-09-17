#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRenderTargetActivatorComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("RenderTarget", GetRenderTarget, SetRenderTarget)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_Target", WDependencyFlags::Package), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WRenderTargetActivatorComponent::WRenderTargetActivatorComponent() = default;
WRenderTargetActivatorComponent::~WRenderTargetActivatorComponent() = default;

void WRenderTargetActivatorComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void WRenderTargetActivatorComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

WResult WRenderTargetActivatorComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 0.1f);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WRenderTargetActivatorComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hRenderTarget.IsValid())
    return;

  WResourceLock<WRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, WResourceAcquireMode::BlockTillLoaded);

  // The consumer view will sample from the render target texture. This includes shadow maps because we don't know how the texture is used in the material.
  WRenderWorld::AddViewDependency(*msg.m_pView, pRenderTarget->GetGALTexture(), WGALResourceState::ShaderResource);

  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView)
    return;

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    WRenderWorld::AddViewToRender(hView);
  }
}

void WRenderTargetActivatorComponent::SetRenderTarget(const WRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
