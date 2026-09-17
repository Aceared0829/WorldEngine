#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WGreyBoxShape, 2)
  W_ENUM_CONSTANTS(WGreyBoxShape::Box)
  W_ENUM_CONSTANTS(WGreyBoxShape::RampPosX, WGreyBoxShape::RampNegX)
  W_ENUM_CONSTANTS(WGreyBoxShape::RampPosY, WGreyBoxShape::RampNegY)
  W_ENUM_CONSTANTS(WGreyBoxShape::Column)
  W_ENUM_CONSTANTS(WGreyBoxShape::StairsPosX, WGreyBoxShape::StairsNegX)
  W_ENUM_CONSTANTS(WGreyBoxShape::StairsPosY, WGreyBoxShape::StairsNegY)
  W_ENUM_CONSTANTS(WGreyBoxShape::ArchX, WGreyBoxShape::ArchY)
  W_ENUM_CONSTANTS(WGreyBoxShape::SpiralStairs)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WGreyBoxComponent, 7, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("Shape", WGreyBoxShape, GetShape, SetShape),
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX)->AddAttributes(new WGroupAttribute("Size", "Size")),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),//->AddAttributes(new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new WGroupAttribute("Misc"), new WDefaultValueAttribute(16), new WClampValueAttribute(3, 32)),
    W_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature)->AddAttributes(new WClampValueAttribute(WAngle::MakeFromDegree(-360), WAngle::MakeFromDegree(360))),
    W_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
    W_ACCESSOR_PROPERTY("SlopedBottom", GetSlopedBottom, SetSlopedBottom),
    W_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("UseAsOccluder", m_bUseAsOccluder)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Construction"),
    new WNonUniformBoxManipulatorAttribute("SizeNegX", "SizePosX", "SizeNegY", "SizePosY", "SizeNegZ", "SizePosZ"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgBuildStaticMesh, OnBuildStaticMesh),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
    W_MESSAGE_HANDLER(WMsgExtractOccluderData, OnMsgExtractOccluderData),
    W_MESSAGE_HANDLER(WMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetCustomData, OnMsgSetCustomData),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WGreyBoxComponent::WGreyBoxComponent() = default;
WGreyBoxComponent::~WGreyBoxComponent() = default;

void WGreyBoxComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_Shape;
  s << m_hMaterial;
  s << m_fSizeNegX;
  s << m_fSizePosX;
  s << m_fSizeNegY;
  s << m_fSizePosY;
  s << m_fSizeNegZ;
  s << m_fSizePosZ;
  s << m_uiDetail;

  // Version 2
  s << m_Curvature;
  s << m_fThickness;
  s << m_bSlopedTop;
  s << m_bSlopedBottom;

  // Version 3
  s << m_Color;

  // Version 4
  s << m_bGenerateCollision;
  bool m_bIncludeInNavmesh = true; // dummy
  s << m_bIncludeInNavmesh;

  // Version 5
  s << m_bUseAsOccluder;

  // Version 7
  s << m_vCustomData;
}

void WGreyBoxComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_Shape;
  s >> m_hMaterial;
  s >> m_fSizeNegX;
  s >> m_fSizePosX;
  s >> m_fSizeNegY;
  s >> m_fSizePosY;
  s >> m_fSizeNegZ;
  s >> m_fSizePosZ;
  s >> m_uiDetail;

  if (uiVersion >= 2)
  {
    s >> m_Curvature;
    s >> m_fThickness;
    s >> m_bSlopedTop;
    s >> m_bSlopedBottom;
  }

  if (uiVersion >= 3)
  {
    s >> m_Color;
  }

  if (uiVersion >= 4)
  {
    s >> m_bGenerateCollision;
    bool m_bIncludeInNavmesh = true; // dummy
    s >> m_bIncludeInNavmesh;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseAsOccluder;
  }

  if (uiVersion >= 7)
  {
    s >> m_vCustomData;
  }
}

void WGreyBoxComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<WMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

void WGreyBoxComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

WResult WGreyBoxComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();

    if (m_bUseAsOccluder)
    {
      msg.AddBounds(bounds, GetOwner()->IsStatic() ? WDefaultSpatialDataCategories::OcclusionStatic : WDefaultSpatialDataCategories::OcclusionDynamic);
    }

    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WGreyBoxComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WArrayPtr<const WMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (WUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const WUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    WMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    WMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WMeshRenderData>(GetOwner());
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex);

    bool bDontCacheYet = false;
    WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? WRenderData::Caching::Never : WRenderData::Caching::IfStatic);
  }
}

void WGreyBoxComponent::SetShape(WEnum<WGreyBoxShape> shape)
{
  m_Shape = shape;
  InvalidateMesh();
}

void WGreyBoxComponent::SetColor(const WColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const WColor& WGreyBoxComponent::GetColor() const
{
  return m_Color;
}

void WGreyBoxComponent::SetCustomData(const WVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const WVec4& WGreyBoxComponent::GetCustomData() const
{
  return m_vCustomData;
}

void WGreyBoxComponent::SetSizeNegX(float f)
{
  m_fSizeNegX = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSizePosX(float f)
{
  m_fSizePosX = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSizeNegY(float f)
{
  m_fSizeNegY = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSizePosY(float f)
{
  m_fSizePosY = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSizeNegZ(float f)
{
  m_fSizeNegZ = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSizePosZ(float f)
{
  m_fSizePosZ = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetDetail(WUInt32 uiDetail)
{
  m_uiDetail = uiDetail;
  InvalidateMesh();
}

void WGreyBoxComponent::SetCurvature(WAngle curvature)
{
  m_Curvature = WAngle::MakeFromDegree(WMath::RoundToMultiple(curvature.GetDegree(), 5.0f));
  InvalidateMesh();
}

void WGreyBoxComponent::SetSlopedTop(bool b)
{
  m_bSlopedTop = b;
  InvalidateMesh();
}

void WGreyBoxComponent::SetSlopedBottom(bool b)
{
  m_bSlopedBottom = b;
  InvalidateMesh();
}

void WGreyBoxComponent::SetThickness(float f)
{
  m_fThickness = f;
  InvalidateMesh();
}

void WGreyBoxComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void WGreyBoxComponent::OnBuildStaticMesh(WMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  WGeometry geom;
  BuildGeometry(geom, m_Shape, false);
  geom.TriangulatePolygons();

  auto* pDesc = msg.m_pStaticMeshDescription;
  auto& subMesh = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const WTransform t = GetOwner()->GetGlobalTransform();

  const WUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(t * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    auto& tri = pDesc->m_Triangles.ExpandAndGetRef();
    tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
    tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[1];
    tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[2];
  }

  subMesh.m_uiNumTriangles = pDesc->m_Triangles.GetCount() - subMesh.m_uiFirstTriangle;

  WMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    // Data/Base/Materials/Common/Pattern.WMaterialAsset
    hMaterial = WResourceManager::LoadResource<WMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
  }

  if (hMaterial.IsValid())
  {
    WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pMaterial.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      const WString surface = pMaterial->GetSurface().GetString();

      if (!surface.IsEmpty())
      {
        WUInt32 idx = pDesc->m_Surfaces.IndexOf(surface);
        if (idx == WInvalidIndex)
        {
          idx = pDesc->m_Surfaces.GetCount();
          pDesc->m_Surfaces.PushBack(surface);
        }

        subMesh.m_uiSurfaceIndex = static_cast<WUInt16>(idx);
      }
    }
  }
}

void WGreyBoxComponent::OnMsgExtractGeometry(WMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == WWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<WCpuMeshResource>());
}

void WGreyBoxComponent::OnMsgExtractOccluderData(WMsgExtractOccluderData& msg) const
{
  if (!IsActiveAndInitialized() || !m_bUseAsOccluder)
    return;

  if (m_pOccluderObject == nullptr)
  {
    WEnum<WGreyBoxShape> shape = m_Shape;
    if (shape == WGreyBoxShape::StairsPosX && m_Curvature == WAngle())
      shape = WGreyBoxShape::RampPosX;
    if (shape == WGreyBoxShape::StairsNegX && m_Curvature == WAngle())
      shape = WGreyBoxShape::RampNegX;
    if (shape == WGreyBoxShape::StairsPosY && m_Curvature == WAngle())
      shape = WGreyBoxShape::RampPosY;
    if (shape == WGreyBoxShape::StairsNegY && m_Curvature == WAngle())
      shape = WGreyBoxShape::RampNegY;

    WStringBuilder sResourceName;
    GenerateMeshName(sResourceName);

    m_pOccluderObject = WRasterizerObject::GetObject(sResourceName);

    if (m_pOccluderObject == nullptr)
    {
      WGeometry geom;
      BuildGeometry(geom, shape, true);

      m_pOccluderObject = WRasterizerObject::CreateMesh(sResourceName, geom);
    }
  }

  msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
}

void WGreyBoxComponent::OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg)
{
  m_hMaterial = ref_msg.m_hMaterial;

  InvalidateCachedRenderData();
}

void WGreyBoxComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void WGreyBoxComponent::OnMsgSetCustomData(WMsgSetCustomData& ref_msg)
{
  m_vCustomData = ref_msg.m_vData;

  InvalidateCachedRenderData();
}

void WGreyBoxComponent::InvalidateMesh()
{
  m_pOccluderObject = nullptr;

  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<WMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void WGreyBoxComponent::BuildGeometry(WGeometry& geom, WEnum<WGreyBoxShape> shape, bool bOnlyRoughDetails) const
{
  WGeometry::GeoOptions opt;
  opt.m_Color = m_Color;

  WVec3 size;
  size.x = m_fSizeNegX + m_fSizePosX;
  size.y = m_fSizeNegY + m_fSizePosY;
  size.z = m_fSizeNegZ + m_fSizePosZ;

  if (size.x == 0 || size.y == 0 || size.z == 0)
  {
    // create a tiny dummy box, so that we have valid geometry
    geom.AddBox(WVec3(0.01f), true, opt);
    return;
  }

  WVec3 offset(0);
  offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
  offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
  offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

  WMat4 t2, t3;

  opt.m_Transform = WMat4::MakeTranslation(offset);

  switch (shape)
  {
    case WGreyBoxShape::Box:
      geom.AddBox(size, true, opt);
      break;

    case WGreyBoxShape::RampPosX:
      geom.AddTexturedRamp(size, opt);
      break;

    case WGreyBoxShape::RampPosY:
      WMath::Swap(size.x, size.y);
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case WGreyBoxShape::RampNegX:
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(180.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case WGreyBoxShape::RampNegY:
      WMath::Swap(size.x, size.y);
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(270.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case WGreyBoxShape::Column:
      opt.m_Transform.SetScalingFactors(size).IgnoreResult();
      geom.AddCylinder(0.5f, 0.5f, 0.5f, 0.5f, true, true, WMath::Min<WUInt16>(bOnlyRoughDetails ? 14 : 32, static_cast<WUInt16>(m_uiDetail)), opt);
      break;

    case WGreyBoxShape::StairsPosX:
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case WGreyBoxShape::StairsPosY:
      WMath::Swap(size.x, size.y);
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case WGreyBoxShape::StairsNegX:
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(180.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case WGreyBoxShape::StairsNegY:
      WMath::Swap(size.x, size.y);
      opt.m_Transform = WMat4::MakeRotationZ(WAngle::MakeFromDegree(270.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case WGreyBoxShape::ArchX:
    {
      const float tmp = size.z;
      size.z = size.x;
      size.x = size.y;
      size.y = tmp;
      opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(-90));
      t2 = WMat4::MakeRotationX(WAngle::MakeFromDegree(90));
      opt.m_Transform = t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case WGreyBoxShape::ArchY:
    {
      opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(-90));
      t2 = WMat4::MakeRotationX(WAngle::MakeFromDegree(90));
      t3 = WMat4::MakeRotationZ(WAngle::MakeFromDegree(90));
      WMath::Swap(size.y, size.z);
      opt.m_Transform = t3 * t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case WGreyBoxShape::SpiralStairs:
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, true, m_bSlopedBottom, m_bSlopedTop, true, opt);
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

void WGreyBoxComponent::GenerateMeshName(WStringBuilder& out_sName) const
{
  switch (m_Shape)
  {
    case WGreyBoxShape::Box:
      out_sName.SetFormat("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case WGreyBoxShape::RampPosX:
      out_sName.SetFormat("Grey-RampPosX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;
    case WGreyBoxShape::RampNegX:
      out_sName.SetFormat("Grey-RampNegX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case WGreyBoxShape::RampPosY:
      out_sName.SetFormat("Grey-RampPosY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case WGreyBoxShape::RampNegY:
      out_sName.SetFormat("Grey-RampNegY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case WGreyBoxShape::Column:
      out_sName.SetFormat("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail);
      break;

    case WGreyBoxShape::StairsPosX:
      out_sName.SetFormat("Grey-StairsPosX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case WGreyBoxShape::StairsNegX:
      out_sName.SetFormat("Grey-StairsNegX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case WGreyBoxShape::StairsPosY:
      out_sName.SetFormat("Grey-StairsPosY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case WGreyBoxShape::StairsNegY:
      out_sName.SetFormat("Grey-StairsNegY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case WGreyBoxShape::ArchX:
      out_sName.SetFormat("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case WGreyBoxShape::ArchY:
      out_sName.SetFormat("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case WGreyBoxShape::SpiralStairs:
      out_sName.SetFormat("Grey-Spiral:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}-st{9}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness, m_bSlopedTop);
      out_sName.AppendFormat("-sb{0}", m_bSlopedBottom);
      break;


    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

void WGreyBoxComponent::GenerateMeshResourceDescriptor(WMeshResourceDescriptor& desc) const
{
  WGeometry geom;
  BuildGeometry(geom, m_Shape, false);

  bool bInvertedGeo = false;

  if (-m_fSizeNegX > m_fSizePosX)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegY > m_fSizePosY)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegZ > m_fSizePosZ)
    bInvertedGeo = !bInvertedGeo;

  if (bInvertedGeo)
  {
    for (auto vert : geom.GetVertices())
    {
      vert.m_vNormal = -vert.m_vNormal;
    }
  }

  geom.TriangulatePolygons();
  geom.ComputeTangents();

  // Data/Base/Materials/Common/Pattern.WMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();
}

template <typename ResourceType>
WTypedResourceHandle<ResourceType> WGreyBoxComponent::GenerateMesh() const
{
  WStringBuilder sResourceName;
  GenerateMeshName(sResourceName);

  WTypedResourceHandle<ResourceType> hResource = WResourceManager::GetExistingResource<ResourceType>(sResourceName);
  if (hResource.IsValid())
    return hResource;

  WMeshResourceDescriptor desc;
  GenerateMeshResourceDescriptor(desc);

  return WResourceManager::GetOrCreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
}

//////////////////////////////////////////////////////////////////////////

class WGreyBoxComponent_5_6 : public WGraphPatch
{
public:
  WGreyBoxComponent_5_6()
    : WGraphPatch("WGreyBoxComponent", 6)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    WAbstractObjectNode::Property* pProp = pNode->FindProperty("Shape");
    const WStringBuilder sOri = pProp->m_Value.Get<WString>();

    if (sOri == "WGreyBoxShape::RampX")
      pProp->m_Value = "WGreyBoxShape::RampPosX";

    if (sOri == "WGreyBoxShape::RampY")
      pProp->m_Value = "WGreyBoxShape::RampNegY";

    if (sOri == "WGreyBoxShape::StairsX")
      pProp->m_Value = "WGreyBoxShape::StairsPosX";

    if (sOri == "WGreyBoxShape::StairsY")
      pProp->m_Value = "WGreyBoxShape::StairsNegY";
  }
};

WGreyBoxComponent_5_6 g_WGreyBoxComponent_5_6;

W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GreyBoxComponent);
