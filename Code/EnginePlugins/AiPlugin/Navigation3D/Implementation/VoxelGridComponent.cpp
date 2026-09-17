#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAiVoxelGridComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new WDefaultValueAttribute(WVec3(32)), new WClampValueAttribute(WVec3(4), WVec3(1024))),
    W_ACCESSOR_PROPERTY("VoxelSize", GetVoxelSize, SetVoxelSize)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("Visualize", m_bVisualize),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
    new WBoxVisualizerAttribute("Size", 1.0f, WColorScheme::LightUI(WColorScheme::Green), nullptr),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSpatialData::Category WAiVoxelGridComponent::SpatialDataCategory = WSpatialData::RegisterCategory("AiVoxelGrid", WSpatialData::Flags::None);

WAiVoxelGridComponent::WAiVoxelGridComponent() = default;
WAiVoxelGridComponent::~WAiVoxelGridComponent() = default;

void WAiVoxelGridComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_fVoxelSize;
  s << m_uiCollisionLayer;
  s << m_bVisualize;
}

void WAiVoxelGridComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_fVoxelSize;
  s >> m_uiCollisionLayer;
  s >> m_bVisualize;
}

void WAiVoxelGridComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void WAiVoxelGridComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void WAiVoxelGridComponent::OnMsgUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), m_vSize * 0.5f)), SpatialDataCategory);
}

void WAiVoxelGridComponent::SetSize(const WVec3& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;
    m_bNeedsVoxelization = true;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void WAiVoxelGridComponent::SetVoxelSize(float fValue)
{
  if (m_fVoxelSize != fValue)
  {
    m_fVoxelSize = fValue;
    m_bNeedsVoxelization = true;
  }
}

void WAiVoxelGridComponent::SetCollisionLayer(WUInt32 uiValue)
{
  if (m_uiCollisionLayer != uiValue)
  {
    m_uiCollisionLayer = uiValue;
    m_bNeedsVoxelization = true;
  }
}

void WAiVoxelGridComponent::VoxelizeWorld()
{
  if (!m_bNeedsVoxelization)
    return;

  // retrieve collision geometry and rasterize triangles
  auto* pGeoModule = GetWorld()->GetOrCreateModule<WNavmeshGeoWorldModuleInterface>();
  if (pGeoModule == nullptr)
    return;

  m_bNeedsVoxelization = false;

  WVec3U32 res;
  res.x = WMath::CeilToInt(m_vSize.x / m_fVoxelSize);
  res.y = WMath::CeilToInt(m_vSize.y / m_fVoxelSize);
  res.z = WMath::CeilToInt(m_vSize.z / m_fVoxelSize);

  WVec3 vGridCenter = GetOwner()->GetGlobalPosition();

  m_StaticGrid.Initialize(res, vGridCenter, m_fVoxelSize);

  const WBoundingBox gridAABB = m_StaticGrid.GetAABB();

  WDynamicArray<WNavmeshTriangle> triangles;
  pGeoModule->RetrieveGeometryInArea(m_uiCollisionLayer, gridAABB, triangles);

  for (const auto& tri : triangles)
  {
    m_StaticGrid.SetVoxelsOnTriangle(tri.m_Vertices[0], tri.m_Vertices[1], tri.m_Vertices[2]);
  }
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelGridComponent);
