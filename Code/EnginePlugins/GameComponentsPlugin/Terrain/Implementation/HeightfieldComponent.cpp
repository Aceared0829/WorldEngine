#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Terrain/HeightfieldComponent.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WHeightfieldComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("HeightfieldImage", GetHeightfield, SetHeightfield)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_2D"), new WRequiredAttribute()),
    W_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
    W_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new WDefaultValueAttribute(WVec2(50))),
    W_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new WDefaultValueAttribute(50)),
    W_ACCESSOR_PROPERTY("Tesselation", GetTesselation, SetTesselation)->AddAttributes(new WDefaultValueAttribute(WVec2U32(128))),
    W_ACCESSOR_PROPERTY("TexCoordOffset", GetTexCoordOffset, SetTexCoordOffset)->AddAttributes(new WDefaultValueAttribute(WVec2(0))),
    W_ACCESSOR_PROPERTY("TexCoordScale", GetTexCoordScale, SetTexCoordScale)->AddAttributes(new WDefaultValueAttribute(WVec2(1))),
    W_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("ColMeshTesselation", GetColMeshTesselation, SetColMeshTesselation)->AddAttributes(new WDefaultValueAttribute(WVec2U32(0))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Terrain"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WHeightfieldComponent::WHeightfieldComponent() = default;
WHeightfieldComponent::~WHeightfieldComponent() = default;

void WHeightfieldComponent::SetHalfExtents(WVec2 value)
{
  m_vHalfExtents = value;
  InvalidateMesh();
}

void WHeightfieldComponent::SetHeight(float value)
{
  m_fHeight = value;
  InvalidateMesh();
}

void WHeightfieldComponent::SetTexCoordOffset(WVec2 value)
{
  m_vTexCoordOffset = value;
  InvalidateMesh();
}

void WHeightfieldComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  WStreamWriter& s = stream.GetStream();

  s << m_hHeightfield;
  s << m_hMaterial;
  s << m_vHalfExtents;
  s << m_fHeight;
  s << m_vTexCoordOffset;
  s << m_vTexCoordScale;
  s << m_vTesselation;
  s << m_vColMeshTesselation;

  // Version 2
  s << m_bGenerateCollision;

  bool m_bIncludeInNavmesh = true; // dummy
  s << m_bIncludeInNavmesh;
}

void WHeightfieldComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = stream.GetStream();

  s >> m_hHeightfield;
  s >> m_hMaterial;
  s >> m_vHalfExtents;
  s >> m_fHeight;
  s >> m_vTexCoordOffset;
  s >> m_vTexCoordScale;
  s >> m_vTesselation;
  s >> m_vColMeshTesselation;

  if (uiVersion >= 2)
  {
    s >> m_bGenerateCollision;

    bool m_bIncludeInNavmesh = true; // dummy
    s >> m_bIncludeInNavmesh;
  }
}

void WHeightfieldComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<WMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

void WHeightfieldComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_bGenerateCollision)
  {
    PushHeightfieldCollider();
  }
}

void WHeightfieldComponent::PushHeightfieldCollider()
{
  if (!m_hHeightfield.IsValid())
    return;

  WPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  // (0,0) means "use the render mesh resolution". Otherwise use the dedicated collision resolution.
  // m_vTesselation / m_vColMeshTesselation are quad counts; the sample count = quads + 1.
  // Take the smaller axis to keep the grid square, then round down to even.
  const WVec2U32 colTess = (m_vColMeshTesselation.x == 0 || m_vColMeshTesselation.y == 0) ? m_vTesselation : m_vColMeshTesselation;
  WUInt32 N = WMath::Min(colTess.x, colTess.y) + 1;
  if ((N % 2) != 0)
    --N;
  N = WMath::Max(N, 4u);

  // Scale the extents by the game object's global scale so the collision shape matches the rendered mesh in world space.
  // Jolt's HeightFieldShape supports non-uniform scale via the per-axis mScale vector.
  const WVec3 vScale = GetOwner()->GetGlobalTransform().m_vScale;
  const WVec2 vScaledHalfExtents = m_vHalfExtents.CompMul(WVec2(vScale.x, vScale.y));
  const float fScaledHeight = m_fHeight * vScale.z;

  WStringBuilder sIdentifier;
  {
    WUInt64 uiHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiHash = WHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiHash);
    uiHash = WHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiHash);
    uiHash = WHashingUtils::xxHash64(&colTess, sizeof(colTess), uiHash);
    uiHash = WHashingUtils::xxHash64(&vScale, sizeof(vScale), uiHash);
    sIdentifier.SetFormat("Heightfield:{}", uiHash);
  }

  if (pPhysics->TrySetHeightfieldCollider(GetOwner(), sIdentifier).Succeeded())
    return;

  // Cache miss: build the full height data and create the collider.
  WResourceLock<WImageDataResource> pImageData(m_hHeightfield, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Warning("WHeightfieldComponent: could not load heightmap image '{}' for physics collision.", m_hHeightfield.GetResourceID());
    return;
  }

  const WImage& heightmap = pImageData->GetDescriptor().m_Image;
  const WUInt32 uiMaxSamples = WMath::Min(heightmap.GetWidth(), heightmap.GetHeight());
  N = WMath::Min(N, uiMaxSamples);
  if ((N % 2) != 0)
    --N;
  if (N < 4)
    N = 4;

  WPhysicsWorldModuleInterface::HeightfieldColliderData data;
  data.m_uiResolution = N;
  data.m_vHalfExtents = vScaledHalfExtents;
  data.m_Heights.SetCountUninitialized(N * N);
  data.m_MaterialIndices.SetCount((N - 1) * (N - 1), 0);

  const WColor* pImgData = heightmap.GetPixelPointer<WColor>();
  const WUInt32 imgWidth = heightmap.GetWidth();
  const WUInt32 imgHeight = heightmap.GetHeight();
  const WVec2 vToNDC = WVec2(1.0f / (N - 1), 1.0f / (N - 1));

  for (WUInt32 row = 0; row < N; ++row)
  {
    for (WUInt32 col = 0; col < N; ++col)
    {
      const WVec2 ndc = WVec2((float)col, (float)row).CompMul(vToNDC);
      data.m_Heights[row * N + col] = (WImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, WImageAddressMode::Clamp, ndc).r * fScaledHeight) - fScaledHeight;
    }
  }

  // Resolve the surface handle from the material.
  WMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    hMaterial = WResourceManager::LoadResource<WMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.WMaterialAsset
  }
  if (hMaterial.IsValid())
  {
    WResourceLock<WMaterialResource> pMaterial(hMaterial, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMaterial.GetAcquireResult() == WResourceAcquireResult::Final && !pMaterial->GetSurface().IsEmpty())
    {
      data.m_Surfaces.PushBack(WResourceManager::LoadResource<WSurfaceResource>(pMaterial->GetSurface().GetString()));
    }
  }

  pPhysics->CreateHeightfieldCollider(GetOwner(), sIdentifier, data);
}

void WHeightfieldComponent::OnDeactivated()
{
  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

WResult WHeightfieldComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WHeightfieldComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering());

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

void WHeightfieldComponent::SetTexCoordScale(WVec2 value) // [ property ]
{
  m_vTexCoordScale = value;
  InvalidateMesh();
}

void WHeightfieldComponent::SetHeightfield(const WImageDataResourceHandle& hResource)
{
  m_hHeightfield = hResource;
  InvalidateMesh();
}

void WHeightfieldComponent::SetTesselation(WVec2U32 value)
{
  m_vTesselation = value;
  InvalidateMesh();
}

void WHeightfieldComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void WHeightfieldComponent::SetColMeshTesselation(WVec2U32 value)
{
  m_vColMeshTesselation = value;
  // don't invalidate the render mesh
}

void WHeightfieldComponent::OnMsgExtractGeometry(WMsgExtractGeometry& msg) const
{
  if (msg.m_Mode != WWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<WCpuMeshResource>());
}

void WHeightfieldComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<WMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void WHeightfieldComponent::BuildGeometry(WGeometry& geom) const
{
  if (!m_hHeightfield.IsValid())
    return;

  W_PROFILE_SCOPE("Heightfield: BuildGeometry");

  WResourceLock<WImageDataResource> pImageData(m_hHeightfield, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return;
  }

  const WImage& heightmap = pImageData->GetDescriptor().m_Image;

  const WUInt32 uiNumVerticesX = WMath::Clamp(m_vColMeshTesselation.x + 1u, 5u, 512u);
  const WUInt32 uiNumVerticesY = WMath::Clamp(m_vColMeshTesselation.y + 1u, 5u, 512u);

  const WVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
  const WVec2 vToNDC = WVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
  const WVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, 0);

  const WColor* pImgData = heightmap.GetPixelPointer<WColor>();
  const WUInt32 imgWidth = heightmap.GetWidth();
  const WUInt32 imgHeight = heightmap.GetHeight();

  for (WUInt32 y = 0; y < uiNumVerticesY; ++y)
  {
    for (WUInt32 x = 0; x < uiNumVerticesX; ++x)
    {
      const WVec2 ndc = WVec2((float)x, (float)y).CompMul(vToNDC);
      const WVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
      const WVec2 heightTC = ndc;

      const float fHeightScale = 1.0f - WImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, WImageAddressMode::Clamp, heightTC).r;

      const WVec3 vNewPos = vPosOffset + WVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize);

      geom.AddVertex(vNewPos, WVec3(0, 0, 1), tc, WColor::White);
    }
  }

  WUInt32 uiVertexIdx = 0;

  for (WUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
  {
    for (WUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
    {
      WUInt32 indices[4];
      indices[0] = uiVertexIdx;
      indices[1] = uiVertexIdx + 1;
      indices[2] = uiVertexIdx + uiNumVerticesX + 1;
      indices[3] = uiVertexIdx + uiNumVerticesX;

      geom.AddPolygon(indices, false);

      ++uiVertexIdx;
    }

    ++uiVertexIdx;
  }
}

WResult WHeightfieldComponent::BuildMeshDescriptor(WMeshResourceDescriptor& desc) const
{
  W_PROFILE_SCOPE("Heightfield: GenerateRenderMesh");

  WResourceLock<WImageDataResource> pImageData(m_hHeightfield, WResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return W_FAILURE;
  }

  const WImage& heightmap = pImageData->GetDescriptor().m_Image;

  // Data/Base/Materials/Common/Pattern.WMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();

  {
    auto& mb = desc.MeshBufferDesc();

    const WUInt32 uiNumVerticesX = WMath::Clamp(m_vTesselation.x + 1u, 5u, 1024u);
    const WUInt32 uiNumVerticesY = WMath::Clamp(m_vTesselation.y + 1u, 5u, 1024u);
    const WUInt32 uiNumTriangles = (uiNumVerticesX - 1) * (uiNumVerticesY - 1) * 2;

    mb.AllocateStreams(uiNumVerticesX * uiNumVerticesY, WGALPrimitiveTopology::Triangles, uiNumTriangles);

    const WVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
    const WVec2 vToNDC = WVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
    const WVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, -m_fHeight);

    const auto texCoordFormat = mb.GetVertexStreamConfig().GetTexCoordFormat();
    const auto normalFormat = mb.GetVertexStreamConfig().GetNormalFormat();
    const auto tangentFormat = mb.GetVertexStreamConfig().GetTangentFormat();

    // access the vertex data directly
    // this is way more complicated than going through SetVertexData, but it is ~20% faster

    auto positionData = mb.GetPositionData();

    WUInt32 uiNormalDataStride = 0;
    auto normalData = mb.GetNormalData(&uiNormalDataStride);

    WUInt32 uiTangentDataStride = 0;
    auto tangentData = mb.GetTangentData(&uiTangentDataStride);

    WUInt32 uiTexCoordDataStride = 0;
    auto texcoordData = mb.GetTexCoord0Data(&uiTexCoordDataStride);

    WUInt32 uiVertexIdx = 0;

    const WColor* pImgData = heightmap.GetPixelPointer<WColor>();
    const WUInt32 imgWidth = heightmap.GetWidth();
    const WUInt32 imgHeight = heightmap.GetHeight();

    for (WUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (WUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const WVec2 ndc = WVec2((float)x, (float)y).CompMul(vToNDC);
        const WVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
        const WVec2 heightTC = ndc;

        const float fHeightScale = WImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, WImageAddressMode::Clamp, heightTC).r;

        // complicated but faster
        positionData.GetPtr()[uiVertexIdx] = vPosOffset + WVec3(ndc.x, ndc.y, fHeightScale).CompMul(vSize);

        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiTexCoordDataStride;
        WMeshBufferUtils::EncodeTexCoord(tc, WByteArrayPtr(texcoordData.GetPtr() + uiByteOffset, 32), texCoordFormat).IgnoreResult();

        // easier to understand, but slower
        // mb.SetVertexData(0, uiVertexIdx, vPosOffset + WVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize));
        // WMeshBufferUtils::EncodeTexCoord(tc, mb.GetVertexData(1, uiVertexIdx), texCoordFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    uiVertexIdx = 0;

    for (WUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (WUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const WInt32 centerIDx = uiVertexIdx;
        WInt32 leftIDx = uiVertexIdx - 1;
        WInt32 rightIDx = uiVertexIdx + 1;
        WInt32 bottomIDx = uiVertexIdx - uiNumVerticesX;
        WInt32 topIDx = uiVertexIdx + uiNumVerticesX;

        // clamp the indices
        if (x == 0)
          leftIDx = centerIDx;
        if (x + 1 == uiNumVerticesX)
          rightIDx = centerIDx;
        if (y == 0)
          bottomIDx = centerIDx;
        if (y + 1 == uiNumVerticesY)
          topIDx = centerIDx;

        const WVec3 vPosCenter = *(positionData.GetPtr() + (size_t)centerIDx);
        const WVec3 vPosLeft = *(positionData.GetPtr() + (size_t)leftIDx);
        const WVec3 vPosRight = *(positionData.GetPtr() + (size_t)rightIDx);
        const WVec3 vPosBottom = *(positionData.GetPtr() + (size_t)bottomIDx);
        const WVec3 vPosTop = *(positionData.GetPtr() + (size_t)topIDx);

        WVec3 edgeL = vPosLeft - vPosCenter;
        WVec3 edgeR = vPosRight - vPosCenter;
        WVec3 edgeB = vPosBottom - vPosCenter;
        WVec3 edgeT = vPosTop - vPosCenter;

        // rotate edges by 90 degrees, so that they become normals
        WMath::Swap(edgeL.x, edgeL.z);
        WMath::Swap(edgeR.x, edgeR.z);
        WMath::Swap(edgeB.y, edgeB.z);
        WMath::Swap(edgeT.y, edgeT.z);

        edgeL.z = -edgeL.z;
        edgeR.x = -edgeR.x;
        edgeB.z = -edgeB.z;
        edgeT.y = -edgeT.y;

        // don't normalize the edges first, if they are longer, they shall have more influence
        WVec3 vNormal(0);
        vNormal += edgeL;
        vNormal += edgeR;
        vNormal += edgeB;
        vNormal += edgeT;
        vNormal.Normalize();

        WVec3 vTangent = WVec3(1, 0, 0).CrossRH(vNormal).GetNormalized();

        // complicated but faster
        size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiNormalDataStride;
        WMeshBufferUtils::EncodeNormal(vNormal, WByteArrayPtr(normalData.GetPtr() + uiByteOffset, 32), normalFormat).IgnoreResult();

        uiByteOffset = (size_t)uiVertexIdx * (size_t)uiTangentDataStride;
        WMeshBufferUtils::EncodeTangent(vTangent, 1.0f, WByteArrayPtr(tangentData.GetPtr() + uiByteOffset, 32), tangentFormat).IgnoreResult();

        // easier to understand, but slower
        // WMeshBufferUtils::EncodeNormal(WVec3(0, 0, 1), mb.GetVertexData(2, uiVertexIdx), normalFormat).IgnoreResult();
        // WMeshBufferUtils::EncodeTangent(WVec3(1, 0, 0), 1.0f, mb.GetVertexData(3, uiVertexIdx), tangentFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    desc.SetBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(vPosOffset, vPosOffset + vSize.Abs())));

    WUInt32 uiTriangleIdx = 0;
    uiVertexIdx = 0;

    for (WUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
    {
      for (WUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
      {
        mb.SetTriangleIndices(uiTriangleIdx + 0, uiVertexIdx, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX);
        mb.SetTriangleIndices(uiTriangleIdx + 1, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX + 1, uiVertexIdx + uiNumVerticesX);
        uiTriangleIdx += 2;

        ++uiVertexIdx;
      }

      ++uiVertexIdx;
    }
  }

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  return W_SUCCESS;
}

template <typename ResourceType>
WTypedResourceHandle<ResourceType> WHeightfieldComponent::GenerateMesh() const
{
  if (!m_hHeightfield.IsValid())
    return WTypedResourceHandle<ResourceType>();

  WStringBuilder sResourceName;

  {
    WUInt64 uiSettingsHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiSettingsHash = WHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiSettingsHash);
    uiSettingsHash = WHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiSettingsHash);
    uiSettingsHash = WHashingUtils::xxHash64(&m_vTexCoordOffset, sizeof(m_vTexCoordOffset), uiSettingsHash);
    uiSettingsHash = WHashingUtils::xxHash64(&m_vTexCoordScale, sizeof(m_vTexCoordScale), uiSettingsHash);
    uiSettingsHash = WHashingUtils::xxHash64(&m_vTesselation, sizeof(m_vTesselation), uiSettingsHash);

    sResourceName.SetFormat("Heightfield:{}", uiSettingsHash);

    WTypedResourceHandle<ResourceType> hResource = WResourceManager::GetExistingResource<ResourceType>(sResourceName);
    if (hResource.IsValid())
      return hResource;
  }

  WMeshResourceDescriptor desc;
  if (BuildMeshDescriptor(desc).Succeeded())
  {
    return WResourceManager::CreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
  }

  return WTypedResourceHandle<ResourceType>();
}

//////////////////////////////////////////////////////////////////////////

WHeightfieldComponentManager::WHeightfieldComponentManager(WWorld* pWorld)
  : WComponentManager<ComponentType, WBlockStorageType::Compact>(pWorld)
{
  WResourceManager::GetResourceEvents().AddEventHandler(WMakeDelegate(&WHeightfieldComponentManager::ResourceEventHandler, this));
}

WHeightfieldComponentManager::~WHeightfieldComponentManager()
{
  WResourceManager::GetResourceEvents().RemoveEventHandler(WMakeDelegate(&WHeightfieldComponentManager::ResourceEventHandler, this));
}

void WHeightfieldComponentManager::Initialize()
{
  auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WHeightfieldComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void WHeightfieldComponentManager::ResourceEventHandler(const WResourceEvent& e)
{
  if (e.m_Type == WResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<WImageDataResource>())
  {
    WImageDataResource* pResource = (WImageDataResource*)(e.m_pResource);
    const WUInt32 uiChangeCounter = pResource->GetCurrentResourceChangeCounter();
    WImageDataResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hHeightfield == hResource)
      {
        it->m_uiHeightfieldChangeCounter = uiChangeCounter;
        AddToUpdateList(it);
      }
    }
  }
}

void WHeightfieldComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    WHeightfieldComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InvalidateMesh();
  }

  m_ComponentsToUpdate.Clear();
}

void WHeightfieldComponentManager::AddToUpdateList(WHeightfieldComponent* pComponent)
{
  WComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == WInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Terrain_Implementation_HeightfieldComponent);
