#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class SplineMeshGenerationTask : public WTask
{
public:
  SplineMeshGenerationTask(const WGameObjectHandle& hOwnerObject, const WComponentHandle& hOwnerComponent, const WComponentHandle& hSplineComponent, const WStringView sSplineMeshPath, const WSpline& spline, WArrayMap<float, float> distanceToKey, WArrayPtr<WMeshResourceHandle> meshes, WArrayPtr<WVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ)
    : m_hOwnerObject(hOwnerObject)
    , m_hOwnerComponent(hOwnerComponent)
    , m_hSplineComponent(hSplineComponent)
    , m_sSplineMeshPath(sSplineMeshPath)
    , m_Spline(spline)
    , m_DistanceToKey(distanceToKey)
    , m_Meshes(meshes)
    , m_ScaleOffsets(scaleOffsets)
    , m_fLocalOffsetY(fLocalOffsetY)
    , m_fLocalOffsetZ(fLocalOffsetZ)
  {
  }

  virtual void Execute() override
  {
    WTempHybridArray<WCpuMeshResource*, 16> cpuMeshes;

    for (auto& hMesh : m_Meshes)
    {
      auto hMeshCpu = WResourceManager::LoadResource<WCpuMeshResource>(hMesh.GetResourceID());
      WCpuMeshResource* pMeshCpu = WResourceManager::BeginAcquireResource<WCpuMeshResource>(hMeshCpu, WResourceAcquireMode::BlockTillLoaded_NeverFail);
      W_ASSERT_DEV(pMeshCpu != nullptr, "Failed to load cpu mesh resource for spline mesh generation");
      cpuMeshes.PushBack(pMeshCpu);
    }

    W_SCOPE_EXIT(
      for (auto pMeshCpu : cpuMeshes) {
        WResourceManager::EndAcquireResource(pMeshCpu);
      });

    WMeshResourceDescriptor splineMeshDesc;
    if (WSplineMeshComponent::GenerateSplineMeshDesc(m_Spline, m_DistanceToKey, cpuMeshes, m_ScaleOffsets, m_fLocalOffsetY, m_fLocalOffsetZ, splineMeshDesc).Failed())
      return;

    WDeferredFileWriter fileWriter;
    fileWriter.SetOutput(m_sSplineMeshPath);

    WAssetFileHeader header;
    header.SetFileHashAndVersion(0, 0);
    header.Write(fileWriter).IgnoreResult();

    splineMeshDesc.Save(fileWriter);

    if (fileWriter.Close().Failed())
    {
      WLog::Error("Could not write spline mesh file to '{}'", m_sSplineMeshPath);
    }

    {
      WMsgGenericEvent msg;
      msg.m_sMessage.Assign("GenerationDone");
      msg.m_Value = m_sSplineMeshPath;

      WWorld::GetWorld(m_hOwnerComponent)->PostMessage(m_hOwnerComponent, msg, WTime::MakeZero());
    }

    {
      WMsgGenerateSplineMeshCollision msg;
      msg.m_hSplineComponent = m_hSplineComponent;
      msg.m_RenderMeshes = m_Meshes;
      msg.m_ScaleOffsets = m_ScaleOffsets;
      msg.m_fLocalOffsetY = m_fLocalOffsetY;
      msg.m_fLocalOffsetZ = m_fLocalOffsetZ;

      WWorld::GetWorld(m_hOwnerComponent)->PostMessage(m_hOwnerObject, msg, WTime::MakeZero());
    }
  }

private:
  WGameObjectHandle m_hOwnerObject;
  WComponentHandle m_hOwnerComponent;
  WComponentHandle m_hSplineComponent;

  WString m_sSplineMeshPath;
  WSpline m_Spline;
  WArrayMap<float, float> m_DistanceToKey;
  WDynamicArray<WMeshResourceHandle> m_Meshes;
  WDynamicArray<WVec2> m_ScaleOffsets;
  float m_fLocalOffsetY = 0;
  float m_fLocalOffsetZ = 0;
};

//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WSplineMeshDistributionMode, 1)
  W_ENUM_CONSTANTS(WSplineMeshDistributionMode::FitToSegment, WSplineMeshDistributionMode::ScaleEvenly, WSplineMeshDistributionMode::ScaleEvenlyPerSegment)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

/////////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgGenerateSplineMeshCollision);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgGenerateSplineMeshCollision, 1, WRTTIDefaultAllocator<WMsgGenerateSplineMeshCollision>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WSplineMeshPart, WNoBase, 1, WRTTIDefaultAllocator<WSplineMeshPart>)
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    W_MEMBER_PROPERTY("PaddingFront", m_fPaddingFront),
    W_MEMBER_PROPERTY("PaddingBack", m_fPaddingBack),
  }
  W_END_PROPERTIES;
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WSplineMeshPart::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_hMesh;
  inout_stream << m_fPaddingFront;
  inout_stream << m_fPaddingBack;

  return W_SUCCESS;
}

WResult WSplineMeshPart::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_hMesh;
  inout_stream >> m_fPaddingFront;
  inout_stream >> m_fPaddingBack;

  return W_SUCCESS;
}

WResult WSplineMeshPart::ComputeLengthAndOffset(WVec2& out_vLengthAndOffset) const
{
  if (m_hMesh.IsValid() == false)
    return W_FAILURE;

  WResourceLock<WMeshResource> pMeshResource(m_hMesh, WResourceAcquireMode::BlockTillLoaded);
  if (pMeshResource.GetAcquireResult() != WResourceAcquireResult::Final)
    return W_FAILURE;

  const auto& bounds = pMeshResource->GetBounds();
  const float fMinExtent = WMath::Min(bounds.m_vBoxHalfExtents.x, bounds.m_fSphereRadius);

  float fMinX = bounds.m_vCenter.x - fMinExtent;
  float fLength = 2 * fMinExtent;

  out_vLengthAndOffset.Set(fLength, -fMinX);
  return W_SUCCESS;
}

WVec2 WSplineMeshPart::AddPadding(const WVec2& vLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack) const
{
  float fLength = vLengthAndOffset.x;
  float fOffset = vLengthAndOffset.y;

  if (bAllowOverlapFront || m_fPaddingFront > 0.0f)
  {
    fLength += m_fPaddingFront;
    fOffset += m_fPaddingFront;
  }

  if (bAllowOverlapBack || m_fPaddingBack > 0.0f)
  {
    fLength += m_fPaddingBack;
  }

  return WVec2(fLength, fOffset);
}

/////////////////////////////////////////////////////////////////////////////

namespace
{
  static W_FORCE_INLINE WUInt32 RandomUInt(WUInt32 uiMin, WUInt32 uiMax, int& inout_iRandomPos, WUInt32 uiSeed)
  {
    WSimdVec4u res = WSimdVec4u::Truncate(WSimdRandom::FloatMinMax(WSimdVec4i(inout_iRandomPos), WSimdVec4f((float)uiMin), WSimdVec4f((float)uiMax), WSimdVec4u(uiSeed)));
    ++inout_iRandomPos;
    return res.x();
  }

  static W_FORCE_INLINE WVec2 MakeFinalScaleOffsetFromDistance(float fStartDistance, float fEndDistance, const WVec2& vLengthAndOffset)
  {
    const float fRange = fEndDistance - fStartDistance;

    const float fInvLength = 1.0f / vLengthAndOffset.x;
    const float fScale = fRange * fInvLength;
    const float fOffset = (vLengthAndOffset.y * fInvLength) * fRange + fStartDistance;

    return WVec2(fScale, fOffset);
  }
} // namespace

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSplineMeshComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("StartPart", GetStartPart, SetStartPart),
    W_ARRAY_ACCESSOR_PROPERTY("MiddleParts", MiddleParts_GetCount, MiddleParts_GetValue, MiddleParts_SetValue, MiddleParts_Insert, MiddleParts_Remove),
    W_ACCESSOR_PROPERTY("EndPart", GetEndPart, SetEndPart),
    W_ENUM_ACCESSOR_PROPERTY("DistributionMode", WSplineMeshDistributionMode, GetDistributionMode, SetDistributionMode),
    W_ACCESSOR_PROPERTY("Seed", GetSeed, SetSeed)->AddAttributes(new WDefaultValueAttribute(-1), new WClampValueAttribute(-1, WVariant()), new WMinValueTextAttribute("Auto")),
    W_ACCESSOR_PROPERTY("OffsetY", GetOffsetY, SetOffsetY),
    W_ACCESSOR_PROPERTY("OffsetZ", GetOffsetZ, SetOffsetZ),

    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new WDefaultValueAttribute(WVec4(0, 1, 0, 1))),
    W_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material")),
    W_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgSplineChanged, OnMsgSplineChanged),
    W_MESSAGE_HANDLER(WMsgExtractGeometry, OnMsgExtractGeometry),
    W_MESSAGE_HANDLER(WMsgGenericEvent, OnMsgGenericEvent),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WAtomicInteger32 s_iSplineMeshResources;

WSplineMeshComponent::WSplineMeshComponent() = default;
WSplineMeshComponent::~WSplineMeshComponent() = default;

void WSplineMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSplineMesh();
}

void WSplineMeshComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  WTaskSystem::WaitForGroup(m_TaskGroupID);
}

void WSplineMeshComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  // Do not call SUPER::SerializeComponent to avoid serializing m_hMesh
  WStreamWriter& s = inout_stream.GetStream();

  m_StartPart.Serialize(s).IgnoreResult();
  s.WriteArray(m_MiddleParts).IgnoreResult();
  m_EndPart.Serialize(s).IgnoreResult();

  s << m_DistributionMode;
  s << m_iSeed;

  s << m_fOffsetY;
  s << m_fOffsetZ;
  s << m_uiStableId;

  // WMeshComponentBase serialization
  s.WriteArray(m_Materials).IgnoreResult();
  s << m_Color;
  s << m_vCustomData;
  s << m_fSortingDepthOffset;
}

void WSplineMeshComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  // Do not call SUPER::DeserializeComponent to avoid deserializing m_hMesh
  WStreamReader& s = inout_stream.GetStream();

  m_StartPart.Deserialize(s).IgnoreResult();
  s.ReadArray(m_MiddleParts).IgnoreResult();
  m_EndPart.Deserialize(s).IgnoreResult();

  s >> m_DistributionMode;
  s >> m_iSeed;

  s >> m_fOffsetY;
  s >> m_fOffsetZ;
  s >> m_uiStableId;

  // WMeshComponentBase de-serialization
  s.ReadArray(m_Materials).IgnoreResult();
  s >> m_Color;
  s >> m_vCustomData;
  s >> m_fSortingDepthOffset;
}

void WSplineMeshComponent::SetStartPart(const WSplineMeshPart& part)
{
  m_StartPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetMiddleParts(WArrayPtr<const WSplineMeshPart> middleParts)
{
  m_MiddleParts = middleParts;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetEndPart(const WSplineMeshPart& part)
{
  m_EndPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetDistributionMode(WEnum<WSplineMeshDistributionMode> mode)
{
  m_DistributionMode = mode;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetSeed(WInt32 iSeed)
{
  m_iSeed = iSeed;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetOffsetY(float fOffsetY)
{
  if (m_fOffsetY == fOffsetY)
    return;

  m_fOffsetY = fOffsetY;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::SetOffsetZ(float fOffsetZ)
{
  if (m_fOffsetZ == fOffsetZ)
    return;

  m_fOffsetZ = fOffsetZ;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

WResult WSplineMeshComponent::GenerateSplineMeshDesc(const WSpline& spline, const WArrayMap<float, float>& distanceToKey, WArrayPtr<WCpuMeshResource*> meshes, WArrayPtr<WVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ, WMeshResourceDescriptor& out_splineMeshDesc)
{
  WHashTable<WString, WUInt32> materialMapping;

  for (auto& pMesh : meshes)
  {
    for (auto& mat : pMesh->GetDescriptor().GetMaterials())
    {
      if (materialMapping.Contains(mat.m_sPath) == false)
      {
        materialMapping.Insert(mat.m_sPath, materialMapping.GetCount());
      }
    }

    for (auto it : materialMapping)
    {
      out_splineMeshDesc.SetMaterial(it.Value(), it.Key());
    }
  }

  struct SubMeshInfo
  {
    WUInt32 uiMaterialIndex = 0;
    WUInt32 uiMeshIndex = 0;
    WUInt32 uiFirstPrimitive = 0;
    WUInt32 uiPrimitiveCount = 0;

    bool operator<(const SubMeshInfo& rhs) const
    {
      if (uiMaterialIndex != rhs.uiMaterialIndex)
        return uiMaterialIndex < rhs.uiMaterialIndex;
      if (uiMeshIndex != rhs.uiMeshIndex)
        return uiMeshIndex < rhs.uiMeshIndex;
      return uiFirstPrimitive < rhs.uiFirstPrimitive;
    }
  };

  auto& splineMeshBufferDesc = out_splineMeshDesc.MeshBufferDesc();
  const WUInt32 uiNumMeshes = meshes.GetCount();

  constexpr auto topology = WGALPrimitiveTopology::Triangles;
  WUInt32 uiNumVertices = 0;
  WUInt32 uiNumPrimitives = 0;
  WTempHybridArray<SubMeshInfo, 32> subMeshInfos;
  for (WUInt32 uiMeshIndex = 0; uiMeshIndex < uiNumMeshes; ++uiMeshIndex)
  {
    auto pMesh = meshes[uiMeshIndex];
    auto materials = pMesh->GetDescriptor().GetMaterials();

    for (auto& subMesh : pMesh->GetDescriptor().GetSubMeshes())
    {
      auto& info = subMeshInfos.ExpandAndGetRef();
      info.uiMaterialIndex = materialMapping[materials[subMesh.m_uiMaterialIndex].m_sPath];
      info.uiMeshIndex = uiMeshIndex;
      info.uiFirstPrimitive = subMesh.m_uiFirstPrimitive;
      info.uiPrimitiveCount = subMesh.m_uiPrimitiveCount;
    }

    auto& meshBufferDesc = pMesh->GetDescriptor().MeshBufferDesc();
    uiNumVertices += meshBufferDesc.GetVertexCount();
    if (meshBufferDesc.HasIndexBuffer())
    {
      uiNumPrimitives += meshBufferDesc.GetPrimitiveCount();
    }

    splineMeshBufferDesc.AddStreamConfig(meshBufferDesc.GetVertexStreamConfig());
  }

  subMeshInfos.Sort();

  splineMeshBufferDesc.AllocateStreams(uiNumVertices, topology, uiNumPrimitives);

  WUInt32 uiFirstVertex = 0;
  WUInt32 uiIndexByteOffset = 0;
  WTempHybridArray<WUInt32, 16> firstVertexPerMesh;
  firstVertexPerMesh.SetCount(uiNumMeshes);

  const bool bShouldHaveNTT = splineMeshBufferDesc.GetVertexStreamConfig().HasNormalTangentAndTexCoord0();
  const bool bShouldHaveTexCoord1 = splineMeshBufferDesc.GetVertexStreamConfig().HasTexCoord1();
  const bool bShouldHaveColor0 = splineMeshBufferDesc.GetVertexStreamConfig().HasColor0();
  const bool bShouldHaveColor1 = splineMeshBufferDesc.GetVertexStreamConfig().HasColor1();

  for (WUInt32 uiMeshIndex = 0; uiMeshIndex < uiNumMeshes; ++uiMeshIndex)
  {
    auto& meshBufferDesc = meshes[uiMeshIndex]->GetDescriptor().MeshBufferDesc();
    auto& vertexStreamConfig = meshBufferDesc.GetVertexStreamConfig();
    const WVec2& scaleOffset = scaleOffsets[uiMeshIndex];

    for (WUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
    {
      WVec3 pos = meshBufferDesc.GetPosition(v);
      const float fDistance = pos.x * scaleOffset.x + scaleOffset.y;
      const float fKey = WSplineComponent::GetKeyAtDistanceHelper(distanceToKey, fDistance);
      const WTransform transform = WSimdConversion::ToTransform(spline.EvaluateTransform(fKey));

      const WUInt32 uiTargetVertex = uiFirstVertex + v;

      pos = transform.TransformPosition(WVec3(0, pos.y + fLocalOffsetY, pos.z + fLocalOffsetZ));
      splineMeshBufferDesc.SetPosition(uiTargetVertex, pos);

      if (bShouldHaveNTT)
      {
        if (vertexStreamConfig.HasNormalTangentAndTexCoord0())
        {
          WMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
          normalTransform.Invert(0.0f).IgnoreResult();
          normalTransform.Transpose();

          WVec3 normal = normalTransform.TransformDirection(meshBufferDesc.GetNormal(v));
          normal.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

          WVec4 tangent = meshBufferDesc.GetTangent(v);
          WVec3 tangentDir = normalTransform.TransformDirection(tangent.GetAsVec3());
          tangentDir.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();
          tangent = tangentDir.GetAsVec4(tangent.w);

          splineMeshBufferDesc.SetNormal(uiTargetVertex, normal);
          splineMeshBufferDesc.SetTangent(uiTargetVertex, tangent);
          splineMeshBufferDesc.SetTexCoord0(uiTargetVertex, meshBufferDesc.GetTexCoord0(v));
        }
        else
        {
          splineMeshBufferDesc.SetNormal(uiTargetVertex, WVec3::MakeAxisZ());
          splineMeshBufferDesc.SetTangent(uiTargetVertex, WVec3::MakeAxisX().GetAsVec4(1.0f));
          splineMeshBufferDesc.SetTexCoord0(uiTargetVertex, WVec2::MakeZero());
        }
      }

      if (bShouldHaveTexCoord1)
      {
        splineMeshBufferDesc.SetTexCoord1(uiTargetVertex, vertexStreamConfig.HasTexCoord1() ? meshBufferDesc.GetTexCoord1(v) : WVec2::MakeZero());
      }

      if (bShouldHaveColor0)
      {
        splineMeshBufferDesc.SetColor0(uiTargetVertex, vertexStreamConfig.HasColor0() ? meshBufferDesc.GetColor0(v) : WColor::Black);
      }

      if (bShouldHaveColor1)
      {
        splineMeshBufferDesc.SetColor1(uiTargetVertex, vertexStreamConfig.HasColor1() ? meshBufferDesc.GetColor1(v) : WColor::Black);
      }
    }

    firstVertexPerMesh[uiMeshIndex] = uiFirstVertex;
    uiFirstVertex += meshBufferDesc.GetVertexCount();
  }

  WUInt32 uiTargetIndex = 0;
  WUInt32 uiTargetFirstPrimitive = 0;
  WUInt32 uiTargetPrimitiveCount = 0;
  auto& targetIndices = splineMeshBufferDesc.GetIndexBufferData();

  for (WUInt32 uiSubMeshInfoIndex = 0; uiSubMeshInfoIndex < subMeshInfos.GetCount(); ++uiSubMeshInfoIndex)
  {
    auto& subMeshInfo = subMeshInfos[uiSubMeshInfoIndex];

    auto& meshBufferDesc = meshes[subMeshInfo.uiMeshIndex]->GetDescriptor().MeshBufferDesc();
    if (meshBufferDesc.HasIndexBuffer())
    {
      auto sourceIndices = meshBufferDesc.GetIndexBufferData();
      bool bSourceUses32BitIndices = meshBufferDesc.Uses32BitIndices();

      const WUInt32 uiFirstVertex = firstVertexPerMesh[subMeshInfo.uiMeshIndex];
      const WUInt32 uiSourceFirstIndex = WGALPrimitiveTopology::GetIndexCount(topology, subMeshInfo.uiFirstPrimitive);
      const WUInt32 uiSourceIndexCount = WGALPrimitiveTopology::GetIndexCount(topology, subMeshInfo.uiPrimitiveCount);

      uiTargetPrimitiveCount += subMeshInfo.uiPrimitiveCount;

      if (splineMeshBufferDesc.Uses32BitIndices())
      {
        if (bSourceUses32BitIndices)
        {
          auto pSourceIndices32 = reinterpret_cast<const WUInt32*>(sourceIndices.GetPtr());
          auto pTargetIndices32 = reinterpret_cast<WUInt32*>(targetIndices.GetData());
          for (WUInt32 i = 0; i < uiSourceIndexCount; ++i)
          {
            pTargetIndices32[uiTargetIndex] = pSourceIndices32[i + uiSourceFirstIndex] + uiFirstVertex;
            ++uiTargetIndex;
          }
        }
        else
        {
          auto pSourceIndices16 = reinterpret_cast<const WUInt16*>(sourceIndices.GetPtr());
          auto pTargetIndices32 = reinterpret_cast<WUInt32*>(targetIndices.GetData());
          for (WUInt32 i = 0; i < uiSourceIndexCount; ++i)
          {
            pTargetIndices32[uiTargetIndex] = pSourceIndices16[i + uiSourceFirstIndex] + uiFirstVertex;
            ++uiTargetIndex;
          }
        }
      }
      else
      {
        W_ASSERT_DEV(bSourceUses32BitIndices == false, "Mesh uses 32 bit indices, but target buffer is configured for 16 bit indices. This should not happen.");

        auto pSourceIndices16 = reinterpret_cast<const WUInt16*>(sourceIndices.GetPtr());
        auto pTargetIndices16 = reinterpret_cast<WUInt16*>(targetIndices.GetData());
        for (WUInt32 i = 0; i < uiSourceIndexCount; ++i)
        {
          pTargetIndices16[uiTargetIndex] = pSourceIndices16[i + uiSourceFirstIndex] + uiFirstVertex;
          ++uiTargetIndex;
        }
      }
    }

    if (uiSubMeshInfoIndex == subMeshInfos.GetCount() - 1 || subMeshInfos[uiSubMeshInfoIndex + 1].uiMaterialIndex != subMeshInfo.uiMaterialIndex)
    {
      out_splineMeshDesc.AddSubMesh(uiTargetPrimitiveCount, uiTargetFirstPrimitive, subMeshInfo.uiMaterialIndex);

      uiTargetFirstPrimitive += uiTargetPrimitiveCount;
      uiTargetPrimitiveCount = 0;
    }
  }

  out_splineMeshDesc.ComputeBounds();

  return W_SUCCESS;
}

void WSplineMeshComponent::MiddleParts_SetValue(WUInt32 uiIndex, const WSplineMeshPart& value)
{
  m_MiddleParts.EnsureCount(uiIndex + 1);
  m_MiddleParts[uiIndex] = value;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::MiddleParts_Insert(WUInt32 uiIndex, const WSplineMeshPart& value)
{
  m_MiddleParts.InsertAt(uiIndex, value);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::MiddleParts_Remove(WUInt32 uiIndex)
{
  m_MiddleParts.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void WSplineMeshComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_uiStableId = WHashingUtils::xxHash64(&node.GetGuid(), sizeof(WUuid));
}

void WSplineMeshComponent::OnMsgSplineChanged(WMsgSplineChanged& ref_msg)
{
  // An invalid change counter indicates that this msg came from a spline node before the actual spline has been updated. Ignore that here.
  if (ref_msg.m_uiChangeCounter == WInvalidIndex || ref_msg.m_uiChangeCounter == m_uiLastSplineChangeCounter)
    return;

  UpdateSplineMesh();
}

void WSplineMeshComponent::OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != WWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  auto hMesh = GetMesh();
  if (hMesh.IsValid())
  {
    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), WResourceManager::LoadResource<WCpuMeshResource>(hMesh.GetResourceID()));
  }
}

void WSplineMeshComponent::OnMsgGenericEvent(WMsgGenericEvent& ref_msg)
{
  if (ref_msg.m_sMessage == "GenerationDone")
  {
    WStringView sMeshPath = ref_msg.m_Value.Get<WString>();

    auto hSplineMesh = WResourceManager::LoadResource<WMeshResource>(sMeshPath);
    if (GetMesh() == hSplineMesh)
    {
      WResourceManager::ReloadResource(hSplineMesh, true);

      auto hCpuMeshResource = WResourceManager::GetExistingResource<WCpuMeshResource>(sMeshPath);
      if (hCpuMeshResource.IsValid())
      {
        WResourceManager::ReloadResource(hCpuMeshResource, true);
      }
    }
    else
    {
      SetMesh(hSplineMesh);
    }

    m_pGenerationTask = nullptr;

    if (m_pNextGenerationTask != nullptr)
    {
      m_pGenerationTask = std::move(m_pNextGenerationTask);
      m_TaskGroupID = WTaskSystem::StartSingleTask(m_pGenerationTask, WTaskPriority::LongRunning);
      m_pNextGenerationTask = nullptr;
    }
  }
}

void WSplineMeshComponent::GenerateMeshPath(const WSplineComponent& splineComponent, WStringBuilder& out_sSplineMeshPath) const
{
  const WUInt64 uiStableSplineId = WHashingUtils::xxHash64(&splineComponent.GetUuid(), sizeof(WUuid));

  out_sSplineMeshPath.SetFormat(":project/AssetCache/Generated/SplineMesh_{}_{}.WBinMesh", WArgU(m_uiStableId, 16, true, 16, true), WArgU(uiStableSplineId, 16, true, 16, true));
}

WResult WSplineMeshComponent::GenerateDistribution(const WSplineComponent& splineComponent, WDynamicArray<WMeshResourceHandle>& out_Meshes, WDynamicArray<WVec2>& out_scaleOffsets) const
{
  if (m_MiddleParts.IsEmpty() || m_MiddleParts[0].IsValid() == false)
    return W_FAILURE;

  if (splineComponent.GetSpline().GetNumControlPoints() < 2)
    return W_FAILURE;

  // Gather part offset and length information
  WVec2 paddedStartLengthAndOffset = WVec2::MakeZero();
  WTempHybridArray<WVec2, 16> middleLengthAndOffset;
  WVec2 paddedEndLengthAndOffset = WVec2::MakeZero();
  {
    if (m_StartPart.IsValid())
    {
      W_SUCCEED_OR_RETURN(m_StartPart.ComputeLengthAndOffset(paddedStartLengthAndOffset));
      paddedStartLengthAndOffset = m_StartPart.AddPadding(paddedStartLengthAndOffset, false, true);
    }

    for (WUInt32 i = 0; i < m_MiddleParts.GetCount(); ++i)
    {
      WVec2 v = WVec2::MakeZero();
      W_SUCCEED_OR_RETURN(m_MiddleParts[i].ComputeLengthAndOffset(v));
      middleLengthAndOffset.PushBack(v);
    }

    if (m_EndPart.IsValid())
    {
      W_SUCCEED_OR_RETURN(m_EndPart.ComputeLengthAndOffset(paddedEndLengthAndOffset));
      paddedEndLengthAndOffset = m_EndPart.AddPadding(paddedEndLengthAndOffset, true, false);
    }
  }

  int iRandomPos = 46237;
  WUInt32 uiSeed = m_iSeed < 0 ? GetOwner()->GetStableRandomSeed() : static_cast<WUInt32>(m_iSeed);

  auto GenerateEvenDistribution = [&](float fStartDistance, float fTotalLength, bool bAllowStartPart, bool bAllowEndPart)
  {
    const bool bHasStartPart = bAllowStartPart && m_StartPart.IsValid();
    const bool bHasEndPart = bAllowEndPart && m_EndPart.IsValid();

    // First determine the total scale and gather middle part indices
    float fScale = 1.0f;
    WTempHybridArray<WUInt32, 16> middlePartsIndices;
    {
      float fCurrentLength = 0.0f;

      if (bHasStartPart)
      {
        fCurrentLength += paddedStartLengthAndOffset.x;
      }

      if (bHasEndPart)
      {
        fCurrentLength += paddedEndLengthAndOffset.x;
      }

      while (true)
      {
        const WUInt32 uiPartIndex = RandomUInt(0, m_MiddleParts.GetCount(), iRandomPos, uiSeed);
        const bool bAllowOverlapFront = bHasStartPart || middlePartsIndices.GetCount() > 0;
        const float fPartLength = m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, true).x;

        if (fCurrentLength + fPartLength > fTotalLength)
          break;

        middlePartsIndices.PushBack(uiPartIndex);
        fCurrentLength += fPartLength;
      }

      const float fLastPartLength = fTotalLength - fCurrentLength;
      if (fLastPartLength > 0.1f)
      {
        const bool bAllowOverlapFront = bHasStartPart || middlePartsIndices.GetCount() > 0;
        const bool bAllowOverlapBack = bHasEndPart;
        const WUInt32 uiPartIndex = FindBestMiddlePart(fLastPartLength, middleLengthAndOffset, bAllowOverlapFront, bAllowOverlapBack, iRandomPos, uiSeed);
        middlePartsIndices.PushBack(uiPartIndex);

        fCurrentLength += m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], true, m_EndPart.IsValid()).x;
      }

      fScale = fTotalLength / fCurrentLength;
    }

    if (bHasStartPart)
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedStartLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedStartLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    const WUInt32 uiNumMiddleParts = middlePartsIndices.GetCount();
    for (WUInt32 i = 0; i < uiNumMiddleParts; ++i)
    {
      const WUInt32 uiPartIndex = middlePartsIndices[i];
      auto& part = m_MiddleParts[uiPartIndex];

      out_Meshes.PushBack(part.m_hMesh);

      const bool bAllowOverlapFront = m_StartPart.IsValid() || i > 0;
      const bool bAllowOverlapBack = bHasEndPart || i < uiNumMiddleParts - 1;
      const WVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);

      const float fEndDistance = fStartDistance + paddedLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    if (bHasEndPart)
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedEndLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedEndLengthAndOffset));
    }
  };

  if (m_DistributionMode == WSplineMeshDistributionMode::ScaleEvenly)
  {
    const float fStartDistance = 0.0f;
    const float fTotalLength = splineComponent.GetTotalLength();
    const bool bAllowStartPart = !splineComponent.GetClosed();
    const bool bAllowEndPart = !splineComponent.GetClosed();
    GenerateEvenDistribution(fStartDistance, fTotalLength, bAllowStartPart, bAllowEndPart);
  }
  else if (m_DistributionMode == WSplineMeshDistributionMode::ScaleEvenlyPerSegment)
  {
    const bool bClosed = splineComponent.GetClosed();
    const WUInt32 uiNumSegments = splineComponent.GetSpline().GetNumSegments();
    float fStartDistance = 0.0f;
    for (WUInt32 uiSegmentIndex = 0; uiSegmentIndex < uiNumSegments; ++uiSegmentIndex)
    {
      const float fSegmentLength = splineComponent.GetSegmentLength(uiSegmentIndex);
      const bool bAllowStartPart = !bClosed && (uiSegmentIndex == 0);
      const bool bAllowEndPart = !bClosed && (uiSegmentIndex == uiNumSegments - 1);
      GenerateEvenDistribution(fStartDistance, fSegmentLength, bAllowStartPart, bAllowEndPart);

      fStartDistance += fSegmentLength;
    }
  }
  else if (m_DistributionMode == WSplineMeshDistributionMode::FitToSegment)
  {
    const bool bClosed = splineComponent.GetClosed();
    const bool bHasStartPart = !bClosed && m_StartPart.IsValid();
    const bool bHasEndPart = !bClosed && m_EndPart.IsValid();
    const WUInt32 uiNumSegments = splineComponent.GetSpline().GetNumSegments();
    WUInt32 uiSegmentIndex = 0;

    float fStartDistance = 0.0f;
    if (bHasStartPart)
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);

      const float fEndDistance = fStartDistance + splineComponent.GetSegmentLength(uiSegmentIndex);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedStartLengthAndOffset));

      ++uiSegmentIndex;
      fStartDistance = fEndDistance;
    }

    const WUInt32 uiMiddlePartsEndSegment = uiNumSegments - (bHasEndPart ? 1 : 0);
    for (; uiSegmentIndex < uiMiddlePartsEndSegment; ++uiSegmentIndex)
    {
      const float fSegmentLength = splineComponent.GetSegmentLength(uiSegmentIndex);
      const bool bAllowOverlapFront = uiSegmentIndex > 0;
      const bool bAllowOverlapBack = uiSegmentIndex < uiNumSegments - 1;
      const WUInt32 uiPartIndex = FindBestMiddlePart(fSegmentLength, middleLengthAndOffset, bAllowOverlapFront, bAllowOverlapBack, iRandomPos, uiSeed);

      auto& part = m_MiddleParts[uiPartIndex];
      out_Meshes.PushBack(part.m_hMesh);

      const float fEndDistance = fStartDistance + fSegmentLength;
      const WVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    if (bHasEndPart)
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);

      const float fEndDistance = splineComponent.GetTotalLength();
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedEndLengthAndOffset));
    }
  }

  return W_SUCCESS;
}

WUInt32 WSplineMeshComponent::FindBestMiddlePart(float fRequestedLength, WArrayPtr<const WVec2> middleLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack, int& inout_iRandomPos, WUInt32 uiSeed) const
{
  float fBestDiff = WMath::MaxValue<float>();
  WTempHybridArray<WUInt32, 8> candidateIndices;

  W_ASSERT_DEBUG(m_MiddleParts.GetCount() == middleLengthAndOffset.GetCount(), "Mismatched array sizes");
  for (WUInt32 i = 0; i < m_MiddleParts.GetCount(); ++i)
  {
    const float fPartLength = m_MiddleParts[i].AddPadding(middleLengthAndOffset[i], bAllowOverlapFront, bAllowOverlapBack).x;
    const float fDiff = WMath::Abs(fPartLength - fRequestedLength);
    if (WMath::IsEqual(fDiff, fBestDiff, 0.01f))
    {
      candidateIndices.PushBack(i);
    }
    else if (fDiff < fBestDiff)
    {
      fBestDiff = fDiff;

      candidateIndices.Clear();
      candidateIndices.PushBack(i);
    }
  }

  if (candidateIndices.GetCount() == 1)
    return candidateIndices[0];

  const WUInt32 uiRandom = RandomUInt(0, candidateIndices.GetCount(), inout_iRandomPos, uiSeed);
  return candidateIndices[uiRandom];
}

void WSplineMeshComponent::UpdateSplineMesh()
{
  const WSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  WStringBuilder sMeshPath;
  GenerateMeshPath(*pSplineComponent, sMeshPath);

  if (GetUniqueID() != WInvalidIndex)
  {
    WTempHybridArray<WMeshResourceHandle, 16> meshes;
    WTempHybridArray<WVec2, 16> scaleOffsets;
    if (GenerateDistribution(*pSplineComponent, meshes, scaleOffsets).Failed())
      return;

    m_uiLastSplineChangeCounter = pSplineComponent->GetChangeCounter();

    auto pTask = W_DEFAULT_NEW(SplineMeshGenerationTask, GetOwner()->GetHandle(), GetHandle(), pSplineComponent->GetHandle(), sMeshPath, pSplineComponent->GetSpline(), pSplineComponent->GetDistanceToKeyRemapping(), meshes, scaleOffsets, m_fOffsetY, m_fOffsetZ);
    pTask->ConfigureTask("Generate Spline Mesh", WTaskNesting::Maybe);

    StartGenerateTask(pTask);
  }
  else
  {
    // mirrors GenerateDistribution's own early-out (editor path above) - without at least one valid
    // middle part it never generates/caches a mesh in the first place, so don't try to load it either
    if (m_MiddleParts.IsEmpty() || m_MiddleParts[0].IsValid() == false)
      return;

    auto hSplineMesh = WResourceManager::LoadResource<WMeshResource>(sMeshPath);
    SetMesh(hSplineMesh);
  }
}

void WSplineMeshComponent::StartGenerateTask(WSharedPtr<WTask>&& pTask)
{
  if (m_pGenerationTask != nullptr)
  {
    m_pNextGenerationTask = pTask;
    return;
  }

  m_pGenerationTask = pTask;
  m_TaskGroupID = WTaskSystem::StartSingleTask(m_pGenerationTask, WTaskPriority::LongRunning);
}

const WSplineComponent* WSplineMeshComponent::GetSplineComponent() const
{
  const WGameObject* pObject = GetOwner();
  while (pObject != nullptr)
  {
    const WSplineComponent* pSplineComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pSplineComponent))
    {
      return pSplineComponent;
    }
    pObject = pObject->GetParent();
  }

  return nullptr;
}


W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SplineMeshComponent);
