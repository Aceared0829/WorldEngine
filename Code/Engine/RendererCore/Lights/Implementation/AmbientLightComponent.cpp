#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAmbientLightComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new WDefaultValueAttribute(WColorGammaUB(WColor(0.2f, 0.2f, 0.3f)))),
    W_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new WDefaultValueAttribute(WColorGammaUB(WColor(0.1f, 0.1f, 0.15f)))),
    W_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f))
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Lighting"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WAmbientLightComponent::WAmbientLightComponent() = default;
WAmbientLightComponent::~WAmbientLightComponent() = default;

void WAmbientLightComponent::Deinitialize()
{
  WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void WAmbientLightComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  UpdateSkyIrradiance();
}

void WAmbientLightComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  WReflectionPool::ResetConstantSkyIrradiance(GetWorld());
}

void WAmbientLightComponent::SetTopColor(WColorGammaUB color)
{
  m_TopColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

WColorGammaUB WAmbientLightComponent::GetTopColor() const
{
  return m_TopColor;
}

void WAmbientLightComponent::SetBottomColor(WColorGammaUB color)
{
  m_BottomColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

WColorGammaUB WAmbientLightComponent::GetBottomColor() const
{
  return m_BottomColor;
}

void WAmbientLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

float WAmbientLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void WAmbientLightComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic);
}

void WAmbientLightComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_TopColor;
  s << m_BottomColor;
  s << m_fIntensity;
}

void WAmbientLightComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_TopColor;
  s >> m_BottomColor;
  s >> m_fIntensity;
}

void WAmbientLightComponent::UpdateSkyIrradiance()
{
  WColor topColor = WColor(m_TopColor) * m_fIntensity;
  WColor bottomColor = WColor(m_BottomColor) * m_fIntensity;
  WColor midColor = WMath::Lerp(bottomColor, topColor, 0.5f);

  WAmbientCube<WColor> ambientLightIrradiance;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::PosX] = midColor;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::NegX] = midColor;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::PosY] = midColor;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::NegY] = midColor;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::PosZ] = topColor;
  ambientLightIrradiance.m_Values[WAmbientCubeBasis::NegZ] = bottomColor;

  WReflectionPool::SetConstantSkyIrradiance(GetWorld(), ambientLightIrradiance);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WAmbientLightComponentPatch_1_2 : public WGraphPatch
{
public:
  WAmbientLightComponentPatch_1_2()
    : WGraphPatch("WAmbientLightComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Top Color", "TopColor");
    pNode->RenameProperty("Bottom Color", "BottomColor");
  }
};

WAmbientLightComponentPatch_1_2 g_WAmbientLightComponentPatch_1_2;



W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_AmbientLightComponent);
