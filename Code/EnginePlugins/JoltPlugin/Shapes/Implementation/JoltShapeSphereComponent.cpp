#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/EmptyShape.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltShapeSphereComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereManipulatorAttribute("Radius"),
    new WSphereVisualizerAttribute("Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltShapeSphereComponent::WJoltShapeSphereComponent() = default;
WJoltShapeSphereComponent::~WJoltShapeSphereComponent() = default;

void WJoltShapeSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
}

void WJoltShapeSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
}

void WJoltShapeSphereComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), WInvalidSpatialDataCategory);
}

void WJoltShapeSphereComponent::SetRadius(float f)
{
  m_fRadius = WMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeSphereComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());

  if (m_fRadius > 0.0f)
  {
    auto pNewShape = new JPH::SphereShape(m_fRadius);
    pNewShape->SetDensity(fDensity);
    pNewShape->SetMaterial(pMaterial);

    sub.m_pShape = pNewShape;
  }
  else
  {
    sub.m_pShape = new JPH::EmptyShape();
  }

  sub.m_pShape->AddRef();
  sub.m_pShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeSphereComponent);
