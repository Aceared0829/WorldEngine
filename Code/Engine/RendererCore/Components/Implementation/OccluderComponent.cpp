#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

// TODO:
// * in editor, at startup the collider will be created multiple times, until all properties are set -> cache, do once
// * have a way to render the occluder when selected in editor ?

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WOccluderType, 1)
  W_ENUM_CONSTANTS(WOccluderType::Box, WOccluderType::QuadPosX, WOccluderType::Mesh)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WOccluderComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("Type", WOccluderType, GetType, SetType),
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WClampValueAttribute(WVec3(0.0f), {}), new WDefaultValueAttribute(WVec3(1.0f))),
    W_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::LightUI(WColorScheme::Blue)),
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WShapeIconAlwaysVisibleAttribute(),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WOccluderComponentManager::WOccluderComponentManager(WWorld* pWorld)
  : WComponentManager<WOccluderComponent, WBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

WOccluderComponent::WOccluderComponent() = default;
WOccluderComponent::~WOccluderComponent() = default;

void WOccluderComponent::SetExtents(const WVec3& vExtents)
{
  if (m_vExtents == vExtents)
    return;

  m_vExtents = vExtents;

  if (m_Type != WOccluderType::Mesh)
  {
    m_pOccluderObject.Clear();
    UpdateOccluder();
  }
}

void WOccluderComponent::SetType(WEnum<WOccluderType> type)
{
  if (m_Type == type)
    return;

  m_Type = type;
  m_pOccluderObject.Clear();

  UpdateOccluder();
}

void WOccluderComponent::SetMesh(const WCpuMeshResourceHandle& hMesh)
{
  if (m_hMesh == hMesh)
    return;

  m_hMesh = hMesh;

  if (m_Type == WOccluderType::Mesh)
  {
    m_pOccluderObject.Clear();
    UpdateOccluder();
  }
}

const WCpuMeshResourceHandle& WOccluderComponent::GetMesh() const
{
  return m_hMesh;
}

void WOccluderComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  auto category = WDefaultSpatialDataCategories::OcclusionDynamic;

  if (GetOwner()->IsStatic())
  {
    category = WDefaultSpatialDataCategories::OcclusionStatic;
  }

  if (m_Type == WOccluderType::Mesh)
  {
    if (m_pOccluderObject)
    {
      WResourceLock<WCpuMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pMesh.GetAcquireResult() == WResourceAcquireResult::Final)
      {
        msg.AddBounds(pMesh->GetDescriptor().GetBounds(), category);
      }
    }
  }
  else
  {
    msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), category);
  }
}

void WOccluderComponent::UpdateOccluder()
{
  if (!IsActiveAndInitialized())
    return;

  if (m_pOccluderObject != nullptr)
    return;

  switch (m_Type)
  {
    case WOccluderType::Box:
      m_pOccluderObject = WRasterizerObject::CreateBox(m_vExtents);
      break;

    case WOccluderType::QuadPosX:
      m_pOccluderObject = WRasterizerObject::CreateQuadX(WVec2(m_vExtents.z, m_vExtents.y));
      break;

    case WOccluderType::Mesh:
    {
      if (!m_hMesh.IsValid())
        return;

      WResourceLock<WCpuMeshResource> pMesh(m_hMesh, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pMesh.GetAcquireResult() != WResourceAcquireResult::Final)
        return;

      const auto& desc = pMesh->GetDescriptor().MeshBufferDesc();
      if (desc.GetTopology() != WGALPrimitiveTopology::Triangles)
      {
        WLog::Error("Mesh can't be used as a occluder, invalid topology: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (!desc.GetVertexStreamConfig().HasPosition())
      {
        WLog::Error("Mesh can't be used as a occluder, invalid vertex stream configuration: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (!desc.HasIndexBuffer())
      {
        WLog::Error("Mesh can't be used as a occluder, no index buffer: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (desc.Uses32BitIndices())
      {
        WLog::Error("Mesh can't be used as a occluder, too many triangles: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      WGeometry geo;

      const WVec3* pPositions = desc.GetPositionData().GetPtr();
      const WUInt16* pIndices = (const WUInt16*)desc.GetIndexBufferData().GetPtr();

      for (WUInt32 vtx = 0; vtx < desc.GetVertexCount(); ++vtx)
      {
        const WVec3& v = pPositions[vtx];
        geo.AddVertex(v, WVec3(0, 0, 1));
      }

      WUInt32 idx[3];

      for (WUInt32 p = 0; p < desc.GetPrimitiveCount(); ++p)
      {
        idx[0] = pIndices[0];
        idx[1] = pIndices[1];
        idx[2] = pIndices[2];
        pIndices += 3;

        geo.AddPolygon(idx, false);
      }

      m_pOccluderObject = WRasterizerObject::CreateMesh(pMesh->GetResourceID(), geo);

      break;
    }
  }

  GetOwner()->UpdateLocalBounds();
}

void WOccluderComponent::OnMsgExtractOccluderData(WMsgExtractOccluderData& msg) const
{
  if (m_pOccluderObject == nullptr)
    return;

  switch (m_Type)
  {
    case WOccluderType::Box:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
      break;

    case WOccluderType::QuadPosX:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform() + GetOwner()->GetGlobalRotation() * WVec3(m_vExtents.x * 0.5f, 0, 0));
      break;

    case WOccluderType::Mesh:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
      break;
  }
}

void WOccluderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Type;
  s << m_hMesh;
}

void WOccluderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 2)
  {
    s >> m_Type;
  }

  if (uiVersion >= 3)
  {
    s >> m_hMesh;
  }
}

void WOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();

  UpdateOccluder();
}

void WOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_OccluderComponent);
