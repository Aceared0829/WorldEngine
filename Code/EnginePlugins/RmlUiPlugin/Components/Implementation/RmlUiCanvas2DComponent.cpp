#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRmlUiCanvas2DComponent, 5, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new WClampValueAttribute(WVec2(0), WVec2(1))),
    W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WSuffixAttribute("px"), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new WDefaultValueAttribute(WVec2::MakeZero()), new WSuffixAttribute("px")),
    W_ACCESSOR_PROPERTY("CustomScale", GetCustomScale, SetCustomScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input/RmlUi"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WRmlUiCanvas2DComponent::WRmlUiCanvas2DComponent() = default;
WRmlUiCanvas2DComponent::~WRmlUiCanvas2DComponent() = default;
WRmlUiCanvas2DComponent& WRmlUiCanvas2DComponent::operator=(WRmlUiCanvas2DComponent&& rhs) = default;

void WRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  // Update once to ensure correct initial state
  Update();
}

void WRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  WGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);
}

void WRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  WVec2 viewSize = WVec2::MakeZero();
  m_bNeedsUpdate |= UpdateSizeOffsetAndTexture(viewSize);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    WVec2 mousePos;
    WInputManager::GetInputSlotState(WInputSlot_MousePositionX, &mousePos.x);
    WInputManager::GetInputSlotState(WInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - m_vFinalOffset;
    ReceiveInput(mousePos, WRmlUiInputSnapshot::MakeFromCurrentInput());
  }

  SUPER::Update();
}

void WRmlUiCanvas2DComponent::SetOffset(const WVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void WRmlUiCanvas2DComponent::SetSize(const WVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void WRmlUiCanvas2DComponent::SetAnchorPoint(const WVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
}

void WRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void WRmlUiCanvas2DComponent::SetCustomScale(float fScale)
{
  m_fCustomScale = fScale;
}

void WRmlUiCanvas2DComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_fCustomScale;
}

void WRmlUiCanvas2DComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion >= 5)
  {
    SUPER::DeserializeComponent(inout_stream);
    WStreamReader& s = inout_stream.GetStream();

    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;
    s >> m_fCustomScale;
  }
  else
  {
    WRenderComponent::DeserializeComponent(inout_stream);
    WStreamReader& s = inout_stream.GetStream();

    s >> m_hResource;
    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;

    if (uiVersion >= 2)
    {
      s >> m_bAutobindBlackboards;
    }

    if (uiVersion >= 3)
    {
      s >> m_bOnDemandUpdate;
    }

    if (uiVersion >= 4)
    {
      s >> m_fCustomScale;
    }
  }
}

void WRmlUiCanvas2DComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || m_hTexture.IsInvalidated())
    return;

  if (m_pContext != nullptr)
  {
    if (msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::Thumbnail)
      return;

    WRmlUi::GetSingleton()->ExtractContext(*m_pContext, m_hTexture);

    auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WRmlUiRenderData>(GetOwner());
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_vOffset = m_vFinalOffset;

    msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::GUI, WRenderData::Caching::Never);
    msg.AddDependency(m_hTexture, WDefaultRenderDataCategories::GUI, WGALResourceState::ShaderResource, WGALShaderStageFlags::PixelShader);
  }
}

bool WRmlUiCanvas2DComponent::UpdateSizeOffsetAndTexture(WVec2& out_viewSize)
{
  out_viewSize = WVec2(1.0f);
  if (WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView, WCameraUsageHint::EditorView, GetWorld()))
  {
    out_viewSize.x = pView->GetViewport().width;
    out_viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = out_viewSize.y / m_vReferenceResolution.y;
  }

  WVec2 size = WVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = out_viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = out_viewSize.y;
  }
  const WVec2U32 sizeU32 = WVec2U32(static_cast<WUInt32>(size.x), static_cast<WUInt32>(size.y));
  if (sizeU32.IsZero())
    return false;

  m_pContext->SetSize(sizeU32);
  m_pContext->SetDpiScale(fScale * m_fCustomScale);

  WVec2 offset = WVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset = (out_viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - WVec2(1.0f));
  m_vFinalOffset.x = WMath::Round(offset.x);
  m_vFinalOffset.y = WMath::Round(offset.y);

  // Recreate texture if necessary
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  const WGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != sizeU32.x || pTexture->GetDescription().m_uiHeight != sizeU32.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    WGALTextureCreationDescription desc;
    desc.m_uiWidth = sizeU32.x;
    desc.m_uiHeight = sizeU32.y;
    desc.m_Format = WGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTexture = pDevice->CreateTexture(desc);

    return true;
  }

  return false;
}


W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
