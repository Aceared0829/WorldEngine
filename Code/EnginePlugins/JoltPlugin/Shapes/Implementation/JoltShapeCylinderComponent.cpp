#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCylinderComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltShapeCylinderComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCylinderVisualizerAttribute(WBasisAxis::PositiveZ, "Height", "Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltShapeCylinderComponent::WJoltShapeCylinderComponent() = default;
WJoltShapeCylinderComponent::~WJoltShapeCylinderComponent() = default;

void WJoltShapeCylinderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void WJoltShapeCylinderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void WJoltShapeCylinderComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  WBoundingBox box = WBoundingBox::MakeFromMinMax(WVec3(-m_fRadius, -m_fRadius, -m_fHeight * 0.5f), WVec3(m_fRadius, m_fRadius, m_fHeight * 0.5f));
  msg.AddBounds(WBoundingBoxSphere::MakeFromBox(box), WInvalidSpatialDataCategory);
}

void WJoltShapeCylinderComponent::SetRadius(float f)
{
  m_fRadius = WMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeCylinderComponent::SetHeight(float f)
{
  m_fHeight = WMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeCylinderComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::CylinderShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  const WQuat qTilt = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveY, WBasisAxis::PositiveZ);

  WTransform tOwn = GetOwner()->GetGlobalTransform();
  tOwn.m_vScale.x = tOwn.m_vScale.z;
  tOwn.m_qRotation = tOwn.m_qRotation * qTilt;

  WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, tOwn);
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCylinderComponent);
