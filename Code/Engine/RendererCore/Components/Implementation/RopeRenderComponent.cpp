#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Shader/Types.h>

WCVarBool cvar_FeatureRopesVisBones("Feature.Ropes.VisBones", false, WCVarFlags::Default, "Enables debug visualization of rope bones");

// clang-format off
W_BEGIN_COMPONENT_TYPE(WRopeRenderComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::White), new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new WDefaultValueAttribute(0.05f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new WDefaultValueAttribute(6), new WClampValueAttribute(3, 16)),
    W_ACCESSOR_PROPERTY("Subdivide", GetSubdivide, SetSubdivide),
    W_ACCESSOR_PROPERTY("UScale", GetUScale, SetUScale)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgRopePoseUpdated, OnRopePoseUpdated),
    W_MESSAGE_HANDLER(WMsgSetColor, OnMsgSetColor),
    W_MESSAGE_HANDLER(WMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    W_MESSAGE_HANDLER(WMsgCustomInstanceDataOffsetChanged, OnMsgCustomInstanceDataOffsetChanged)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects/Ropes"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRopeRenderComponent::WRopeRenderComponent() = default;
WRopeRenderComponent::~WRopeRenderComponent() = default;

void WRopeRenderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
  s << m_fThickness;
  s << m_uiDetail;
  s << m_bSubdivide;
  s << m_fUScale;
}

void WRopeRenderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
  s >> m_fThickness;
  s >> m_uiDetail;
  s >> m_bSubdivide;
  s >> m_fUScale;
}

void WRopeRenderComponent::OnActivated()
{
  SUPER::OnActivated();

  m_LocalBounds = WBoundingBoxSphere::MakeInvalid();
}

void WRopeRenderComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  WRenderDataManager* pRenderDataManager = GetWorld()->GetModule<WRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

WResult WRopeRenderComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  bounds = m_LocalBounds;
  return W_SUCCESS;
}

void WRopeRenderComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color);

  WResourceLock<WMeshResource> pMesh(m_hMesh, WResourceAcquireMode::AllowLoadingFallback);
  WMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  WSkinnedMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_DataOffsets.m_uiSkinning = m_SkinningState.m_DataOffset.m_uiOffset;
    pRenderData->m_hSkinningBuffer = msg.m_pRenderDataManager->GetSkinningDataBuffer();
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh);
  }

  WRenderData::Category category = WMaterialResource::GetRenderDataCategory(hMaterial);

  msg.AddRenderData(pRenderData, category, WRenderData::Caching::Never);

  if (cvar_FeatureRopesVisBones)
  {
    auto boneTransforms = m_SkinningState.GetBoneTransformsForReading();

    WTempHybridArray<WDebugRendererLine, 128> lines;
    lines.Reserve(boneTransforms.GetCount() * 3);

    WMat4 offsetMat;
    offsetMat.SetIdentity();

    for (WUInt32 i = 0; i < boneTransforms.GetCount(); ++i)
    {
      offsetMat.SetTranslationVector(WVec3(static_cast<float>(i), 0, 0));
      WMat4 skinningMat = boneTransforms[i].GetAsMat4() * offsetMat;

      WVec3 pos = skinningMat.GetTranslationVector();

      auto& x = lines.ExpandAndGetRef();
      x.m_start = pos;
      x.m_end = x.m_start + skinningMat.TransformDirection(WVec3::MakeAxisX());
      x.m_startColor = WColor::Red;
      x.m_endColor = WColor::Red;

      auto& y = lines.ExpandAndGetRef();
      y.m_start = pos;
      y.m_end = y.m_start + skinningMat.TransformDirection(WVec3::MakeAxisY() * 2.0f);
      y.m_startColor = WColor::Green;
      y.m_endColor = WColor::Green;

      auto& z = lines.ExpandAndGetRef();
      z.m_start = pos;
      z.m_end = z.m_start + skinningMat.TransformDirection(WVec3::MakeAxisZ() * 2.0f);
      z.m_startColor = WColor::Blue;
      z.m_endColor = WColor::Blue;
    }

    WDebugRenderer::DrawLinesOccluded(msg.m_pView->GetHandle(), lines, WColor::White.GetDarker(), GetOwner()->GetGlobalTransform());
    WDebugRenderer::DrawLines(msg.m_pView->GetHandle(), lines, WColor::White, GetOwner()->GetGlobalTransform());
  }
}

void WRopeRenderComponent::SetThickness(float fThickness)
{
  if (m_fThickness != fThickness)
  {
    m_fThickness = fThickness;

    if (IsActiveAndInitialized() && m_SkinningState.HasBoneTransforms())
    {
      auto boneTransforms = m_SkinningState.GetBoneTransformsForReading();

      WTempHybridArray<WTransform, 128> transforms;
      transforms.SetCountUninitialized(boneTransforms.GetCount());

      WMat4 offsetMat;
      offsetMat.SetIdentity();

      for (WUInt32 i = 0; i < boneTransforms.GetCount(); ++i)
      {
        offsetMat.SetTranslationVector(WVec3(static_cast<float>(i), 0, 0));
        WMat4 skinningMat = boneTransforms[i].GetAsMat4() * offsetMat;

        transforms[i] = WTransform::MakeFromMat4(skinningMat);
      }

      UpdateSkinningTransformBuffer(transforms);
    }
  }
}

void WRopeRenderComponent::SetDetail(WUInt32 uiDetail)
{
  if (m_uiDetail != uiDetail)
  {
    m_uiDetail = uiDetail;

    if (IsActiveAndInitialized() && m_SkinningState.HasBoneTransforms())
    {
      GenerateRenderMesh(m_SkinningState.m_uiNumBones);
    }
  }
}

void WRopeRenderComponent::SetSubdivide(bool bSubdivide)
{
  if (m_bSubdivide != bSubdivide)
  {
    m_bSubdivide = bSubdivide;

    if (IsActiveAndInitialized() && m_SkinningState.HasBoneTransforms())
    {
      GenerateRenderMesh(m_SkinningState.m_uiNumBones);
    }
  }
}

void WRopeRenderComponent::SetUScale(float fUScale)
{
  if (m_fUScale != fUScale)
  {
    m_fUScale = fUScale;

    if (IsActiveAndInitialized() && m_SkinningState.HasBoneTransforms())
    {
      GenerateRenderMesh(m_SkinningState.m_uiNumBones);
    }
  }
}

void WRopeRenderComponent::OnMsgSetColor(WMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void WRopeRenderComponent::OnMsgSetMeshMaterial(WMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void WRopeRenderComponent::OnRopePoseUpdated(WMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (m_SkinningState.m_uiNumBones != msg.m_LinkTransforms.GetCount())
  {
    m_SkinningState.Clear();

    GenerateRenderMesh(msg.m_LinkTransforms.GetCount());
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  WBoundingBox newBounds = WBoundingBox::MakeFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(WTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetBox().Contains(newBounds))
  {
    m_LocalBounds.ExpandToInclude(WBoundingBoxSphere::MakeFromBox(newBounds));

    TriggerLocalBoundsUpdate();
  }
}

void WRopeRenderComponent::OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& msg)
{
  m_SkinningState.m_DataOffset = msg.m_NewOffset;
}

void WRopeRenderComponent::GenerateRenderMesh(WUInt32 uiNumRopePieces)
{
  WStringBuilder sResourceName;
  sResourceName.SetFormat("Rope-Mesh:{}{}-d{}-u{}", uiNumRopePieces, m_bSubdivide ? "Sub" : "", m_uiDetail, m_fUScale);

  m_hMesh = WResourceManager::GetExistingResource<WMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  WGeometry geom;

  const WAngle fDegStep = WAngle::MakeFromDegree(360.0f / m_uiDetail);
  const float fVStep = 1.0f / m_uiDetail;

  auto addCap = [&](float x, const WVec3& vNormal, WUInt16 uiBoneIndex, bool bFlipWinding)
  {
    WVec4U16 boneIndices(uiBoneIndex, 0, 0, 0);

    WUInt32 centerIndex = geom.AddVertex(WVec3(x, 0, 0), vNormal, WVec2(0.5f, 0.5f), WColor::White, boneIndices);

    WAngle deg = WAngle::MakeFromRadian(0);
    for (WUInt32 s = 0; s < m_uiDetail; ++s)
    {
      const float fY = WMath::Cos(deg);
      const float fZ = WMath::Sin(deg);

      geom.AddVertex(WVec3(x, fY, fZ), vNormal, WVec2(fY, fZ), WColor::White, boneIndices);

      deg += fDegStep;
    }

    WUInt32 triangle[3];
    triangle[0] = centerIndex;
    for (WUInt32 s = 0; s < m_uiDetail; ++s)
    {
      triangle[1] = s + triangle[0] + 1;
      triangle[2] = ((s + 1) % m_uiDetail) + triangle[0] + 1;

      geom.AddPolygon(triangle, bFlipWinding);
    }
  };

  auto addPiece = [&](float x, const WVec4U16& vBoneIndices, const WColorLinearUB& boneWeights, bool bCreatePolygons)
  {
    WAngle deg = WAngle::MakeFromRadian(0);
    float fU = x * m_fUScale;
    float fV = 0;

    for (WUInt32 s = 0; s <= m_uiDetail; ++s)
    {
      const float fY = WMath::Cos(deg);
      const float fZ = WMath::Sin(deg);

      const WVec3 pos(x, fY, fZ);
      const WVec3 normal(0, fY, fZ);

      geom.AddVertex(pos, normal, WVec2(fU, fV), WColor::White, vBoneIndices, boneWeights);

      deg += fDegStep;
      fV += fVStep;
    }

    if (bCreatePolygons)
    {
      WUInt32 endIndex = geom.GetVertices().GetCount() - (m_uiDetail + 1);
      WUInt32 startIndex = endIndex - (m_uiDetail + 1);

      WUInt32 triangle[3];
      for (WUInt32 s = 0; s < m_uiDetail; ++s)
      {
        triangle[0] = startIndex + s;
        triangle[1] = startIndex + s + 1;
        triangle[2] = endIndex + s + 1;
        geom.AddPolygon(triangle, false);

        triangle[0] = startIndex + s;
        triangle[1] = endIndex + s + 1;
        triangle[2] = endIndex + s;
        geom.AddPolygon(triangle, false);
      }
    }
  };

  // cap
  {
    const WVec3 normal = WVec3(-1, 0, 0);
    addCap(0.0f, normal, 0, true);
  }

  // pieces
  {
    // first ring full weight to first bone
    addPiece(0.0f, WVec4U16(0, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), false);

    WUInt16 p = 1;

    if (m_bSubdivide)
    {
      addPiece(0.75f, WVec4U16(0, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), true);

      for (; p < uiNumRopePieces - 2; ++p)
      {
        addPiece(static_cast<float>(p) + 0.25f, WVec4U16(p, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), true);
        addPiece(static_cast<float>(p) + 0.75f, WVec4U16(p, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), true);
      }

      addPiece(static_cast<float>(p) + 0.25f, WVec4U16(p, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), true);
      ++p;
    }
    else
    {
      for (; p < uiNumRopePieces - 1; ++p)
      {
        // Middle rings half weight between bones. To ensure that weights sum up to 1 we weight one bone with 128 and the other with 127,
        // since "ubyte normalized" can't represent 0.5 perfectly.
        addPiece(static_cast<float>(p), WVec4U16(p - 1, p, 0, 0), WColorLinearUB(128, 127, 0, 0), true);
      }
    }

    // last ring full weight to last bone
    addPiece(static_cast<float>(p), WVec4U16(p, 0, 0, 0), WColorLinearUB(255, 0, 0, 0), true);
  }

  // cap
  {
    const WVec3 normal = WVec3(1, 0, 0);
    addCap(static_cast<float>(uiNumRopePieces - 1), normal, static_cast<WUInt16>(uiNumRopePieces - 1), false);
  }

  geom.ComputeTangents();

  WMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.WMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  auto& meshBufferDesc = desc.MeshBufferDesc();
  meshBufferDesc.AddCommonStreams();
  meshBufferDesc.AddStream(WMeshVertexStreamType::SkinningData);
  meshBufferDesc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = WResourceManager::CreateResource<WMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void WRopeRenderComponent::UpdateSkinningTransformBuffer(WArrayPtr<const WTransform> skinningTransforms)
{
  auto boneTransforms = m_SkinningState.GetOrCreateBoneTransformsForWriting(*this, skinningTransforms.GetCount());

  WMat4 bindPoseMat;
  bindPoseMat.SetIdentity();

  const WVec3 newScale = WVec3(1.0f, m_fThickness * 0.5f, m_fThickness * 0.5f);
  for (WUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    WTransform t = skinningTransforms[i];
    t.m_vScale = newScale;

    // scale x axis to match the distance between this bone and the next bone
    if (i < skinningTransforms.GetCount() - 1)
    {
      t.m_vScale.x = (skinningTransforms[i + 1].m_vPosition - skinningTransforms[i].m_vPosition).GetLength();
    }

    bindPoseMat.SetTranslationVector(WVec3(-static_cast<float>(i), 0, 0));

    boneTransforms[i] = t.GetAsMat4() * bindPoseMat;
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RopeRenderComponent);
