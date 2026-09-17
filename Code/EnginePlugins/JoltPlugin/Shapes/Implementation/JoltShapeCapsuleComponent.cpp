#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltShapeCapsuleComponent, 1, WComponentMode::Static)
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
    new WCapsuleManipulatorAttribute("Height", "Radius"),
    new WCapsuleVisualizerAttribute("Height", "Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltShapeCapsuleComponent::WJoltShapeCapsuleComponent() = default;
WJoltShapeCapsuleComponent::~WJoltShapeCapsuleComponent() = default;

void WJoltShapeCapsuleComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void WJoltShapeCapsuleComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void WJoltShapeCapsuleComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3(0, 0, -m_fHeight * 0.5f), m_fRadius), WInvalidSpatialDataCategory);
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3(0, 0, +m_fHeight * 0.5f), m_fRadius), WInvalidSpatialDataCategory);
}

void WJoltShapeCapsuleComponent::SetRadius(float f)
{
  m_fRadius = WMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeCapsuleComponent::SetHeight(float f)
{
  m_fHeight = WMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeCapsuleComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  JPH::Ref<JPH::CapsuleShape> pNewShape = new JPH::CapsuleShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  JPH::Ref<JPH::RotatedTranslatedShapeSettings> pRotShapeSet = new JPH::RotatedTranslatedShapeSettings(JPH::Vec3::sZero(), JPH::Quat::sRotation(JPH::Vec3::sAxisX(), WAngle::MakeFromDegree(90).GetRadian()), pNewShape);

  JPH::Shape* pRotShape = pRotShapeSet->Create().Get().GetPtr();
  pRotShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));

  WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pRotShape;
  sub.m_pShape->AddRef();
  sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCapsuleComponent);
