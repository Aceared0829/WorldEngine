#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DebugRenderComponent.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(DebugRenderComponentMask, 1)
  W_BITFLAGS_CONSTANT(DebugRenderComponentMask::Box),
  W_BITFLAGS_CONSTANT(DebugRenderComponentMask::Sphere),
  W_BITFLAGS_CONSTANT(DebugRenderComponentMask::Cross),
  W_BITFLAGS_CONSTANT(DebugRenderComponentMask::Quad)
W_END_STATIC_REFLECTED_BITFLAGS;

// BEGIN-DOCS-CODE-SNIPPET: component-reflection-block
W_BEGIN_COMPONENT_TYPE(DebugRenderComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 10)),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White)),
    W_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    W_BITFLAGS_MEMBER_PROPERTY("Render", DebugRenderComponentMask, m_RenderTypes)->AddAttributes(new WDefaultValueAttribute(DebugRenderComponentMask::Box)),

    // BEGIN-DOCS-CODE-SNIPPET: customdata-property
    W_RESOURCE_MEMBER_PROPERTY("CustomData", m_hCustomData)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_CustomData", "SampleCustomData")),
    // END-DOCS-CODE-SNIPPET
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("SampleGamePlugin"), // Component menu group
  }
  W_END_ATTRIBUTES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSetColor, OnSetColor)
  }
  W_END_MESSAGEHANDLERS;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetRandomColor)
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// END-DOCS-CODE-SNIPPET
// clang-format on

DebugRenderComponent::DebugRenderComponent() = default;
DebugRenderComponent::~DebugRenderComponent() = default;

void DebugRenderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fSize;
  s << m_Color;
  s << m_hTexture;
  s << m_RenderTypes;
  s << m_hCustomData;
}

void DebugRenderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fSize;
  s >> m_Color;
  s >> m_hTexture;
  s >> m_RenderTypes;
  s >> m_hCustomData;
}

void DebugRenderComponent::OnSetColor(WMsgSetColor& ref_msg)
{
  m_Color = ref_msg.m_Color;
}

void DebugRenderComponent::SetRandomColor()
{
  WRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void DebugRenderComponent::Update()
{
  const WTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Box))
  {
    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(m_fSize));

    WDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Cross))
  {
    WDebugRenderer::DrawCross(GetWorld(), WVec3::MakeZero(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Sphere))
  {
    // BEGIN-DOCS-CODE-SNIPPET: debugrender-sphere
    WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fSize);
    WDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
    // END-DOCS-CODE-SNIPPET
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Quad) && m_hTexture.IsValid())
  {
    WTempHybridArray<WDebugRendererTexturedTriangle, 16> triangles;

    {
      auto& t0 = triangles.ExpandAndGetRef();

      t0.m_position[0].Set(0, -m_fSize, +m_fSize);
      t0.m_position[1].Set(0, +m_fSize, -m_fSize);
      t0.m_position[2].Set(0, -m_fSize, -m_fSize);

      t0.m_texcoord[0].Set(0.0f, 0.0f);
      t0.m_texcoord[1].Set(1.0f, 1.0f);
      t0.m_texcoord[2].Set(0.0f, 1.0f);
    }

    {
      auto& t1 = triangles.ExpandAndGetRef();

      t1.m_position[0].Set(0, -m_fSize, +m_fSize);
      t1.m_position[1].Set(0, +m_fSize, +m_fSize);
      t1.m_position[2].Set(0, +m_fSize, -m_fSize);

      t1.m_texcoord[0].Set(0.0f, 0.0f);
      t1.m_texcoord[1].Set(1.0f, 0.0f);
      t1.m_texcoord[2].Set(1.0f, 1.0f);
    }

    // move the triangles into our object space
    for (auto& tri : triangles)
    {
      tri.m_position[0] = ownerTransform.TransformPosition(tri.m_position[0]);
      tri.m_position[1] = ownerTransform.TransformPosition(tri.m_position[1]);
      tri.m_position[2] = ownerTransform.TransformPosition(tri.m_position[2]);
    }

    WDebugRenderer::DrawTexturedTriangles(GetWorld(), triangles, m_Color, m_hTexture);
  }

  // accessing custom data resources
  if (m_hCustomData.IsValid())
  {
    // BEGIN-DOCS-CODE-SNIPPET: customdata-access
    WResourceLock<SampleCustomDataResource> pCustomDataResource(m_hCustomData, WResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pCustomDataResource.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      const SampleCustomData* pCustomData = pCustomDataResource->GetData();

      WDebugRenderer::Draw3DText(GetWorld(), WFmt(pCustomData->m_sText), GetOwner()->GetGlobalPosition(), pCustomData->m_Color, pCustomData->m_iSize);
    }
    // END-DOCS-CODE-SNIPPET
  }
}
