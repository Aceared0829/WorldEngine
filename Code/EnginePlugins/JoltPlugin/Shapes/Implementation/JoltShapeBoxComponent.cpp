#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltShapeBoxComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new WDefaultValueAttribute(WVec3(0.5f)), new WClampValueAttribute(WVec3(0), WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new WBoxVisualizerAttribute("HalfExtents", 2.0f),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltShapeBoxComponent::WJoltShapeBoxComponent() = default;
WJoltShapeBoxComponent::~WJoltShapeBoxComponent() = default;

void WJoltShapeBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
}

void WJoltShapeBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
}

void WJoltShapeBoxComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-m_vHalfExtents, m_vHalfExtents)), WInvalidSpatialDataCategory);
}

void WJoltShapeBoxComponent::ExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  ref_msg.AddBox(GetOwner()->GetGlobalTransform(), m_vHalfExtents * 2.0f);
}

void WJoltShapeBoxComponent::SetHalfExtents(const WVec3& value)
{
  m_vHalfExtents = value.CompMax(WVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeBoxComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  WVec3 size = m_vHalfExtents;
  size.x = WMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y = WMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z = WMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(WJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<WUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeBoxComponent);
