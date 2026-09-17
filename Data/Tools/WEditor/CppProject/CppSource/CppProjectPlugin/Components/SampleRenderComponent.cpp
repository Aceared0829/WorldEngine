#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleRenderComponent.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(SampleRenderComponentMask, 1)
  W_BITFLAGS_CONSTANT(SampleRenderComponentMask::Box),
  W_BITFLAGS_CONSTANT(SampleRenderComponentMask::Sphere),
  W_BITFLAGS_CONSTANT(SampleRenderComponentMask::Cross),
  W_BITFLAGS_CONSTANT(SampleRenderComponentMask::Quad)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_COMPONENT_TYPE(SampleRenderComponent, 1 /* version */, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(0, 10)),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White)),
    W_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    W_BITFLAGS_MEMBER_PROPERTY("Render", SampleRenderComponentMask, m_RenderTypes)->AddAttributes(new WDefaultValueAttribute(SampleRenderComponentMask::Box)),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Game"), // Component menu group
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
// clang-format on

SampleRenderComponent::SampleRenderComponent() = default;
SampleRenderComponent::~SampleRenderComponent() = default;

void SampleRenderComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    // this automatically serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual serialization
    WReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization, for example:
    // s << m_fSize;
    // s << m_Color;
    // s << m_hTexture;
    // s << m_RenderTypes;
  }
}

void SampleRenderComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    // this automatically de-serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual de-serialization
    WReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom de-serialization, for example:
    // s >> m_fSize;
    // s >> m_Color;
    // s >> m_hTexture;
    // s >> m_RenderTypes;
  }
}

void SampleRenderComponent::OnSetColor(WMsgSetColor& msg)
{
  m_Color = msg.m_Color;
}

void SampleRenderComponent::SetRandomColor()
{
  WRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void SampleRenderComponent::Update()
{
  const WTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Box))
  {
    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(m_fSize));

    WDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Cross))
  {
    WDebugRenderer::DrawCross(GetWorld(), WVec3::MakeZero(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Sphere))
  {
    WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fSize);
    WDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Quad) && m_hTexture.IsValid())
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
}
