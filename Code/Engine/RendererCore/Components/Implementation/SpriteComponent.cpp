#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSpriteBlendMode, 1)
  W_ENUM_CONSTANTS(WSpriteBlendMode::Masked, WSpriteBlendMode::Transparent, WSpriteBlendMode::Additive)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
WTempHashedString WSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case WSpriteBlendMode::Masked:
    case WSpriteBlendMode::ShapeIcon:
      return "BLEND_MODE_MASKED";
    case WSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case WSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpriteRenderData, 1, WRTTIDefaultAllocator<WSpriteRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSpriteRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const WUInt32 uiTextureIDHash = static_cast<WUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}

bool WSpriteRenderData::CanBatch(const WRenderData& other0) const
{
  const auto& other = WStaticCast<const WSpriteRenderData&>(other0);

  return m_BlendMode == other.m_BlendMode && m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSpriteComponent, 4, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Texture", GetTexture, SetTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("BlendMode", WSpriteBlendMode, m_BlendMode),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f), new WSuffixAttribute(" m")),

    W_MEMBER_PROPERTY("UseMaxScreenSize", m_bUseMaxScreenSize)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("MaxScreenSize", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(64.0f), new WSuffixAttribute(" px")),
    W_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),

    W_MEMBER_PROPERTY("IsAnimated", m_bIsAnimated),
    W_MEMBER_PROPERTY("Columns", m_uiColumns)->AddAttributes(new WClampValueAttribute(1, WVariant()), new WDefaultValueAttribute(1)),
    W_MEMBER_PROPERTY("Rows", m_uiRows)->AddAttributes(new WClampValueAttribute(1, WVariant()), new WDefaultValueAttribute(1)),
    W_MEMBER_PROPERTY("Framerate", m_fFramerate)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(24.0f)),
    W_MEMBER_PROPERTY("Loops", m_uiLoops)->AddAttributes(new WClampValueAttribute(0, WVariant()), new WDefaultValueAttribute(0), new WMinValueTextAttribute("Infinite")),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction, m_OnFinishedAction)
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
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WSpriteComponent::WSpriteComponent() = default;
WSpriteComponent::~WSpriteComponent() = default;

void WSpriteComponent::Update()
{
  if (!m_bIsAnimated || !m_hTexture.IsValid())
    return;

  const WUInt32 uiTotalFrames = m_uiColumns * m_uiRows;
  if (uiTotalFrames <= 1)
    return; // Nothing to animate.

  WTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  m_TimeSinceStart += tDiff;

  const float fTotalAnimTime = uiTotalFrames / WMath::Max(0.001f, m_fFramerate);

  if (m_TimeSinceStart.GetSeconds() >= fTotalAnimTime)
  {
    m_uiCurrentLoop++;

    m_TimeSinceStart -= WTime::MakeFromSeconds(fTotalAnimTime);

    if (m_uiLoops != 0 && m_uiCurrentLoop >= m_uiLoops)
    {
      m_bIsAnimated = false;

      WOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
    }
  }
}

WResult WSpriteComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fSize * 0.5f);
  return W_SUCCESS;
}

void WSpriteComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if (!m_hTexture.IsValid())
    return;

  WSpriteRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WSpriteRenderData>(GetOwner());
  {
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_fSize = m_fSize;

    pRenderData->m_fMaxScreenSize = m_bUseMaxScreenSize ? m_fMaxScreenSize : WMath::HighValue<float>();

    pRenderData->m_fAspectRatio = m_fAspectRatio;
    pRenderData->m_BlendMode = m_BlendMode;
    pRenderData->m_color = m_Color;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    const WUInt16 uiTotalFrames = WMath::Max((WUInt16)1u, (WUInt16)(m_uiColumns * m_uiRows));
    WUInt32 uiCurrentFrame = 0;

    if (uiTotalFrames > 1)
    {
      if (!m_bIsAnimated && m_OnFinishedAction == WOnComponentFinishedAction::None)
      {
        uiCurrentFrame = uiTotalFrames - 1; // Park at the last frame.
      }
      else
      {
        uiCurrentFrame = (WUInt32)(m_TimeSinceStart.GetSeconds() * m_fFramerate) % uiTotalFrames;
      }
    }

    // Protect against division by zero before clamping takes effect.

    const WUInt16 safeCols = WMath::Max((WUInt16)1u, (WUInt16)m_uiColumns);
    const WUInt16 safeRows = WMath::Max((WUInt16)1u, (WUInt16)m_uiRows);

    const WUInt32 x = uiCurrentFrame % safeCols;
    const WUInt32 y = uiCurrentFrame / safeCols;

    pRenderData->m_texCoordScale = WVec2(1.0f / safeCols, 1.0f / safeRows);
    pRenderData->m_texCoordOffset = WVec2((float)x * pRenderData->m_texCoordScale.x, (float)y * pRenderData->m_texCoordScale.y);

    pRenderData->FillSortingKey();
  }

  // Determine render data category.
  WRenderData::Category category = WDefaultRenderDataCategories::LitTransparent;
  if (m_BlendMode == WSpriteBlendMode::Masked)
  {
    category = WDefaultRenderDataCategories::LitMasked;
  }

  const auto cachingFlags = m_bIsAnimated ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic;

  msg.AddRenderData(pRenderData, category, cachingFlags);
}

void WSpriteComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_hTexture;
  s << m_fSize;
  s << m_fMaxScreenSize;

  // Version 3
  s << m_Color; // HDR now
  s << m_fAspectRatio;
  s << m_BlendMode;

  // Version 4
  s << m_bUseMaxScreenSize;
  s << m_bIsAnimated;
  s << m_uiColumns;
  s << m_uiRows;
  s << m_fFramerate;
  s << m_uiLoops;

  WOnComponentFinishedAction::StorageType type = m_OnFinishedAction;
  s << type;
}

void WSpriteComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_hTexture;

  if (uiVersion < 3)
  {
    WColorGammaUB color;
    s >> color;
    m_Color = color;
  }

  s >> m_fSize;
  s >> m_fMaxScreenSize;

  if (uiVersion >= 3)
  {
    s >> m_Color;
    s >> m_fAspectRatio;
    s >> m_BlendMode;
  }

  if (uiVersion >= 4)
  {
    s >> m_bUseMaxScreenSize;
    s >> m_bIsAnimated;
    s >> m_uiColumns;
    s >> m_uiRows;
    s >> m_fFramerate;
    s >> m_uiLoops;

    WOnComponentFinishedAction::StorageType type;
    s >> type;
    m_OnFinishedAction = (WOnComponentFinishedAction::Enum)type;
  }
}

void WSpriteComponent::SetTexture(const WTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const WTexture2DResourceHandle& WSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void WSpriteComponent::SetColor(WColor color)
{
  m_Color = color;
}

WColor WSpriteComponent::GetColor() const
{
  return m_Color;
}

void WSpriteComponent::SetSize(float fSize)
{
  m_fSize = fSize;

  TriggerLocalBoundsUpdate();
}

float WSpriteComponent::GetSize() const
{
  return m_fSize;
}

void WSpriteComponent::SetMaxScreenSize(float fSize)
{
  m_fMaxScreenSize = fSize;
}

float WSpriteComponent::GetMaxScreenSize() const
{
  return m_fMaxScreenSize;
}

void WSpriteComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void WSpriteComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  WOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WSpriteComponentPatch_1_2 : public WGraphPatch
{
public:
  WSpriteComponentPatch_1_2()
    : WGraphPatch("WSpriteComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override { pNode->RenameProperty("Max Screen Size", "MaxScreenSize"); }
};

WSpriteComponentPatch_1_2 g_WSpriteComponentPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteComponent);
