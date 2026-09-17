#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/BeamComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererFoundation/Device/Device.h>


// clang-format off
W_BEGIN_COMPONENT_TYPE(WBeamComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White)),
    W_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new WDefaultValueAttribute(0.1f), new WClampValueAttribute(0.001f, WVariant()), new WSuffixAttribute(" m")),
    W_ACCESSOR_PROPERTY("UVUnitsPerWorldUnit", GetUVUnitsPerWorldUnit, SetUVUnitsPerWorldUnit)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBeamComponent::WBeamComponent() = default;
WBeamComponent::~WBeamComponent() = default;

void WBeamComponent::Update()
{
  WGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    WVec3 currentOwnerPosition = GetOwner()->GetGlobalPosition();
    WVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();

    if (!pTargetObject->IsActive())
    {
      currentTargetPosition = currentOwnerPosition;
    }

    bool updateMesh = false;

    if ((currentOwnerPosition - m_vLastOwnerPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastOwnerPosition = currentOwnerPosition;
    }

    if ((currentTargetPosition - m_vLastTargetPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastTargetPosition = currentTargetPosition;
    }

    if (updateMesh)
    {
      ReinitMeshes();
    }
  }
  else
  {
    m_hMesh.Invalidate();
  }
}

void WBeamComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fUVUnitsPerWorldUnit;
  s << m_hMaterial;
  s << m_Color;
}

void WBeamComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;
  s >> m_fUVUnitsPerWorldUnit;
  s >> m_hMaterial;
  s >> m_Color;
}

WResult WBeamComponent::GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg)
{
  WGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    const WVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();
    const WVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(currentTargetPosition);

    WVec3 pts[] = {WVec3::MakeZero(), targetPositionInOwnerSpace};

    WBoundingBox box = WBoundingBox::MakeFromPoints(pts, 2);
    const float fHalfWidth = m_fWidth * 0.5f;
    box.m_vMin -= WVec3(0, fHalfWidth, fHalfWidth);
    box.m_vMax += WVec3(0, fHalfWidth, fHalfWidth);
    ref_bounds = WBoundingBoxSphere::MakeFromBox(box);

    return W_SUCCESS;
  }

  return W_FAILURE;
}


void WBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void WBeamComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();

  Cleanup();
}

void WBeamComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
  pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
  pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, m_hMaterial, m_hMesh);

  WRenderData::Category category = WMaterialResource::GetRenderDataCategory(m_hMaterial);

  msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);
}

void WBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  ReinitMeshes();
}

void WBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float WBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void WBeamComponent::SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit)
{
  if (fUVUnitsPerWorldUnit <= 0.0f)
    return;

  m_fUVUnitsPerWorldUnit = fUVUnitsPerWorldUnit;

  ReinitMeshes();
}

float WBeamComponent::GetUVUnitsPerWorldUnit() const
{
  return m_fUVUnitsPerWorldUnit;
}

WMaterialResourceHandle WBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void WBeamComponent::CreateMeshes()
{
  WVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(m_vLastTargetPosition);

  if (targetPositionInOwnerSpace.IsZero(0.01f))
    return;

  // Create the beam mesh name, it expresses the beam in local space with it's width
  // this way multiple beams in a corridor can share the same mesh for example.
  WStringBuilder meshName;
  meshName.SetFormat("WBeamComponent_{0}_{1}_{2}_{3}.createdAtRuntime.WBinMesh", m_fWidth, WArgF(targetPositionInOwnerSpace.x, 2), WArgF(targetPositionInOwnerSpace.y, 2), WArgF(targetPositionInOwnerSpace.z, 2));

  m_hMesh = WResourceManager::GetExistingResource<WMeshResource>(meshName);

  // We build a cross mesh, thus we need the following vectors, x is the origin and we need to construct
  // the star points.
  //
  //  3        1
  //
  //      x
  //
  //  4        2
  WVec3 crossVector1 = (0.5f * WVec3::MakeAxisY() + 0.5f * WVec3::MakeAxisZ());
  crossVector1.SetLength(m_fWidth * 0.5f).IgnoreResult();

  WVec3 crossVector2 = (0.5f * WVec3::MakeAxisY() - 0.5f * WVec3::MakeAxisZ());
  crossVector2.SetLength(m_fWidth * 0.5f).IgnoreResult();

  WVec3 crossVector3 = (-0.5f * WVec3::MakeAxisY() + 0.5f * WVec3::MakeAxisZ());
  crossVector3.SetLength(m_fWidth * 0.5f).IgnoreResult();

  WVec3 crossVector4 = (-0.5f * WVec3::MakeAxisY() - 0.5f * WVec3::MakeAxisZ());
  crossVector4.SetLength(m_fWidth * 0.5f).IgnoreResult();

  const float fDistance = (m_vLastOwnerPosition - m_vLastTargetPosition).GetLength();



  // Build mesh if no existing one is found
  if (!m_hMesh.IsValid())
  {
    WGeometry g;

    // Quad 1
    {
      WUInt32 index0 = g.AddVertex(WVec3::MakeZero() + crossVector1, WVec3::MakeAxisX(), WVec2(0, 0), WColor::White);
      WUInt32 index1 = g.AddVertex(WVec3::MakeZero() + crossVector4, WVec3::MakeAxisX(), WVec2(0, 1), WColor::White);
      WUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector1, WVec3::MakeAxisX(), WVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), WColor::White);
      WUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector4, WVec3::MakeAxisX(), WVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), WColor::White);

      WUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(WArrayPtr(indices), false);
      g.AddPolygon(WArrayPtr(indices), true);
    }

    // Quad 2
    {
      WUInt32 index0 = g.AddVertex(WVec3::MakeZero() + crossVector2, WVec3::MakeAxisX(), WVec2(0, 0), WColor::White);
      WUInt32 index1 = g.AddVertex(WVec3::MakeZero() + crossVector3, WVec3::MakeAxisX(), WVec2(0, 1), WColor::White);
      WUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector2, WVec3::MakeAxisX(), WVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), WColor::White);
      WUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector3, WVec3::MakeAxisX(), WVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), WColor::White);

      WUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(WArrayPtr(indices), false);
      g.AddPolygon(WArrayPtr(indices), true);
    }

    g.ComputeTangents();

    WMeshResourceDescriptor desc;
    BuildMeshResourceFromGeometry(g, desc);

    m_hMesh = WResourceManager::CreateResource<WMeshResource>(meshName, std::move(desc));
  }
}

void WBeamComponent::BuildMeshResourceFromGeometry(WGeometry& Geometry, WMeshResourceDescriptor& MeshDesc) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, WGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

void WBeamComponent::ReinitMeshes()
{
  Cleanup();

  if (IsActiveAndInitialized())
  {
    CreateMeshes();
    GetOwner()->UpdateLocalBounds();
  }
}

void WBeamComponent::Cleanup()
{
  m_hMesh.Invalidate();
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BeamComponent);
