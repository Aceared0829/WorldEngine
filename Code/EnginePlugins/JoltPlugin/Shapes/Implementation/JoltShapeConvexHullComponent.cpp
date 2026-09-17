#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WJoltShapeConvexHullComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Convex", WDependencyFlags::Package), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WJoltShapeConvexHullComponent::WJoltShapeConvexHullComponent() = default;
WJoltShapeConvexHullComponent::~WJoltShapeConvexHullComponent() = default;

void WJoltShapeConvexHullComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}

void WJoltShapeConvexHullComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
}

void WJoltShapeConvexHullComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  if (m_hCollisionMesh.IsValid())
  {
    WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);
    msg.AddBounds(pMesh->GetBounds(), WInvalidSpatialDataCategory);
  }
}

void WJoltShapeConvexHullComponent::CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
  {
    WLog::Warning("WJoltShapeConvexHullComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() == 0)
  {
    WLog::Warning("WJoltShapeConvexHullComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}'", GetOwner()->GetName(), pMesh->GetResourceIdOrDescription());
    return;
  }

  for (WUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
  {
    auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<WUInt64>(GetUserData()), pMaterial, fDensity);

    WJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pShape;
    sub.m_Transform = WTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void WJoltShapeConvexHullComponent::ExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != WWorldGeoExtractionUtil::ExtractionMode::CollisionMesh)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    WResourceLock<WJoltMeshResource> pMesh(m_hCollisionMesh, WResourceAcquireMode::BlockTillLoaded);

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeConvexHullComponent);
