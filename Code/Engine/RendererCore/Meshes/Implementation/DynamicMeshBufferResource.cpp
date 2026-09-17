#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IterateBits.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

namespace
{
  static WMutex s_ResourcesToUploadMutex;
  static WHashSet<WDynamicMeshBufferResource*> s_ResourcesToUpload;
} // namespace

struct WDynamicMeshBufferManager
{
  static void AddResourceToUpload(WDynamicMeshBufferResource* pResource)
  {
    W_LOCK(s_ResourcesToUploadMutex);
    s_ResourcesToUpload.Insert(pResource);
  }

  static void RemoveResourceToUpload(WDynamicMeshBufferResource* pResource)
  {
    W_LOCK(s_ResourcesToUploadMutex);
    s_ResourcesToUpload.Remove(pResource);
  }

  static void OnExtractionEvent(const WRenderWorldExtractionEvent& e)
  {
    if (e.m_Type != WRenderWorldExtractionEvent::Type::EndExtraction)
      return;

    W_LOCK(s_ResourcesToUploadMutex);

    for (auto it : s_ResourcesToUpload)
    {
      it->UploadChangesForNextFrame();
    }

    s_ResourcesToUpload.Clear();
  }
};

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DynamicMeshBufferManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WRenderWorld::GetExtractionEvent().AddEventHandler(WDynamicMeshBufferManager::OnExtractionEvent);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WRenderWorld::GetExtractionEvent().RemoveEventHandler(WDynamicMeshBufferManager::OnExtractionEvent);
  }
W_END_SUBSYSTEM_DECLARATION;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicMeshBufferResource, 1, WRTTIDefaultAllocator<WDynamicMeshBufferResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WDynamicMeshBufferResource);
// clang-format on

WDynamicMeshBufferResource::WDynamicMeshBufferResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

WDynamicMeshBufferResource::~WDynamicMeshBufferResource()
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    W_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  W_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

WResourceLoadDesc WDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  WDynamicMeshBufferManager::RemoveResourceToUpload(this);

  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    WGALDevice::GetDefaultDevice()->DestroyBuffer(hVertexBuffer);
  }

  WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WDynamicMeshBufferResource::UpdateContent(WStreamReader* Stream)
{
  W_REPORT_FAILURE("This resource type does not support loading data from file.");

  return WResourceLoadDesc();
}

void WDynamicMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WDynamicMeshBufferResource) + m_PositionData.GetHeapMemoryUsage() + m_NTTData.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage() + m_IndexData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WDynamicMeshBufferResource, WDynamicMeshBufferResourceDescriptor)
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    W_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  W_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  WMeshVertexStreamConfig config;
  {
    config.m_bUseHighPrecision = true;
    config.AddStream(WMeshVertexStreamType::Position);
    config.AddStream(WMeshVertexStreamType::NormalTangentAndTexCoord0);
    if (m_Descriptor.m_bColorStream)
    {
      config.AddStream(WMeshVertexStreamType::Color0);
    }

    W_ASSERT_DEBUG(config.GetNormalFormat() == WGALResourceFormat::RGBAUShortNormalized, "Unexpected normal format");
    W_ASSERT_DEBUG(config.GetTangentFormat() == WGALResourceFormat::RGBAUShortNormalized, "Unexpected tangent format");
    W_ASSERT_DEBUG(config.GetTexCoordFormat() == WGALResourceFormat::XYFloat, "Unexpected texcoord format");
    W_ASSERT_DEBUG(config.GetColorFormat() == WGALResourceFormat::RGBAHalf, "Unexpected color format");

    W_ASSERT_DEBUG(config.GetNormalDataOffset() == offsetof(WDynamicMeshVertexNTT, m_vEncodedNormal), "Unexpected normal offset");
    W_ASSERT_DEBUG(config.GetTangentDataOffset() == offsetof(WDynamicMeshVertexNTT, m_vEncodedTangent), "Unexpected tangent offset");
    W_ASSERT_DEBUG(config.GetTexCoord0DataOffset() == offsetof(WDynamicMeshVertexNTT, m_vTexCoord), "Unexpected texcoord offset");

    config.FillVertexAttributes(m_VertexAttributes);
  }

  const WUInt32 uiVertexCount = WMath::Max(1u, m_Descriptor.m_uiMaxVertices);
  const WUInt32 uiIndexCount = WGALPrimitiveTopology::GetIndexCount(m_Descriptor.m_Topology, m_Descriptor.m_uiMaxPrimitives);
  const bool bUseIndices = uiIndexCount > 0 && descriptor.m_IndexType != WGALIndexType::None;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WStringBuilder sName;

  for (WUInt32 uiIndex : WIterateBitIndices(config.m_uiTypesMask))
  {
    auto type = static_cast<WMeshVertexStreamType::Enum>(uiIndex);
    const WUInt32 uiElementSize = config.GetStreamElementSize(type);

    m_hVertexBuffers[uiIndex] = pDevice->CreateVertexBuffer(uiElementSize, uiVertexCount, WConstByteArrayPtr(), true);

    sName.SetFormat("{0} Dynamic Vertex Buffer {1}", GetResourceIdOrDescription(), WMeshVertexStreamType::GetName(type));
    pDevice->GetBuffer(m_hVertexBuffers[uiIndex])->SetDebugName(sName);
  }

  if (bUseIndices)
  {
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.m_IndexType, uiIndexCount, WConstByteArrayPtr(), true);

    sName.SetFormat("{0} Dynamic Index Buffer", GetResourceIdOrDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }


  m_PositionData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  m_NTTData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  }

  if (bUseIndices)
  {
    m_IndexData.SetCountUninitialized(uiIndexCount * WGALIndexType::GetSize(descriptor.m_IndexType));
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_PositionData.GetHeapMemoryUsage() + m_NTTData.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage() + m_IndexData.GetHeapMemoryUsage();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

void WDynamicMeshBufferResource::MarkAsDirty()
{
  WDynamicMeshBufferManager::AddResourceToUpload(this);
}

void WDynamicMeshBufferResource::UploadChangesForNextFrame()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

  if (m_ModifiedPositionDataRange.IsValid())
  {
    auto data = m_PositionData.GetArrayPtr().GetSubArray(m_ModifiedPositionDataRange.m_uiMin, m_ModifiedPositionDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[WMeshVertexStreamType::Position], data.ToByteArray(), m_ModifiedPositionDataRange.m_uiMin);

    m_ModifiedPositionDataRange.Reset();
  }

  if (m_ModifiedNTTDataRange.IsValid())
  {
    auto data = m_NTTData.GetArrayPtr().GetSubArray(m_ModifiedNTTDataRange.m_uiMin, m_ModifiedNTTDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[WMeshVertexStreamType::NormalTangentAndTexCoord0], data.ToByteArray(), m_ModifiedNTTDataRange.m_uiMin);

    m_ModifiedNTTDataRange.Reset();
  }

  if (m_ModifiedColorDataRange.IsValid())
  {
    auto data = m_ColorData.GetArrayPtr().GetSubArray(m_ModifiedColorDataRange.m_uiMin, m_ModifiedColorDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[WMeshVertexStreamType::Color0], data.ToByteArray(), m_ModifiedColorDataRange.m_uiMin);

    m_ModifiedColorDataRange.Reset();
  }

  if (m_ModifiedIndexDataRange.IsValid())
  {
    auto data = m_IndexData.GetArrayPtr().GetSubArray(m_ModifiedIndexDataRange.m_uiMin, m_ModifiedIndexDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hIndexBuffer, data, m_ModifiedIndexDataRange.m_uiMin);

    m_ModifiedIndexDataRange.Reset();
  }
}

// static
void WDynamicMeshBufferResource::CreateGridXY(WDynamicMeshBufferResource* pDynamicMeshBuffer, const WVec2& vSize, const WVec2U32& vNumVertices, const WVec2& vTextureScale)
{
  W_ASSERT_DEV(vNumVertices.x > 1 && vNumVertices.y > 1, "Invalid number of vertices");

  const WUInt32 uiNumVertsX = vNumVertices.x;
  const WUInt32 uiNumVertsY = vNumVertices.y;
  const WUInt32 uiNumSegmentsX = uiNumVertsX - 1;
  const WUInt32 uiNumSegmentsY = uiNumVertsY - 1;

  W_ASSERT_DEV(pDynamicMeshBuffer->m_Descriptor.m_uiMaxVertices == uiNumVertsX * uiNumVertsY, "Invalid number of vertices");
  W_ASSERT_DEV(pDynamicMeshBuffer->m_Descriptor.m_uiMaxPrimitives = uiNumSegmentsX * uiNumSegmentsY * 2, "Invalid number of primitives");

  {
    const WVec3 dirX = WVec3(1, 0, 0);
    const WVec3 dirY = WVec3(0, 1, 0);

    WDynamicMeshVertexNTT v;
    v.EncodeNormal(WVec3(0, 0, 1));
    v.EncodeTangent(dirX, 1.0f);

    WVec2 dist = vSize;
    dist.x /= (float)uiNumSegmentsX;
    dist.y /= (float)uiNumSegmentsY;

    const float fDivU = (1.0f / uiNumSegmentsX) * vTextureScale.x;
    const float fDivV = (1.0f / uiNumSegmentsY) * vTextureScale.y;

    auto positions = pDynamicMeshBuffer->AccessPositionData();
    auto ntt = pDynamicMeshBuffer->AccessNormalTangentTexCoord0Data();

    for (WUInt32 y = 0; y < uiNumVertsY; ++y)
    {
      for (WUInt32 x = 0; x < uiNumVertsX; ++x)
      {
        const WUInt32 idx = (y * uiNumVertsX) + x;

        positions[idx] = dirX * (x * dist.x) + dirY * (y * dist.y);
        ntt[idx].m_vEncodedNormal = v.m_vEncodedNormal;
        ntt[idx].m_vEncodedTangent = v.m_vEncodedTangent;
        ntt[idx].m_vTexCoord = WVec2(x * fDivU, y * fDivV);
      }
    }
  }

  {
    auto indices = pDynamicMeshBuffer->AccessIndex16Data();

    WUInt32 tidx = 0;
    WUInt32 vidx = 0;
    for (WUInt32 y = 0; y < uiNumSegmentsY; ++y)
    {
      for (WUInt32 x = 0; x < uiNumSegmentsX; ++x, ++vidx)
      {
        indices[tidx++] = vidx;
        indices[tidx++] = vidx + 1;
        indices[tidx++] = vidx + uiNumVertsX;

        indices[tidx++] = vidx + 1;
        indices[tidx++] = vidx + uiNumVertsX + 1;
        indices[tidx++] = vidx + uiNumVertsX;
      }

      ++vidx;
    }
  }
}

void WDynamicMeshBufferResource::CalculateGridNormalAndTangents(WDynamicMeshBufferResource* pDynamicMeshBuffer, const WVec2U32& vNumVertices)
{
  auto positions = pDynamicMeshBuffer->AccessPositionData();
  auto ntt = pDynamicMeshBuffer->AccessNormalTangentTexCoord0Data();

  const WUInt32 width = vNumVertices.x;
  const WUInt32 height = vNumVertices.y;
  const WUInt32 widthM1 = width - 1;
  const WUInt32 heightM1 = height - 1;

  WUInt32 topIdx = 0;

  WUInt32 vidx = 0;
  for (WUInt32 y = 0; y < height; ++y)
  {
    WUInt32 leftIdx = 0;
    const WUInt32 bottomIdx = WMath::Min<WUInt32>(y + 1, heightM1);

    const WUInt32 yOff = y * width;
    const WUInt32 yOffTop = topIdx * width;
    const WUInt32 yOffBottom = bottomIdx * width;

    for (WUInt32 x = 0; x < width; ++x, ++vidx)
    {
      const WUInt32 rightIdx = WMath::Min<WUInt32>(x + 1, widthM1);

      const WSimdVec4f leftPos = WSimdConversion::ToVec3(positions[yOff + leftIdx]);
      const WSimdVec4f rightPos = WSimdConversion::ToVec3(positions[yOff + rightIdx]);
      const WSimdVec4f topPos = WSimdConversion::ToVec3(positions[yOffTop + x]);
      const WSimdVec4f bottomPos = WSimdConversion::ToVec3(positions[yOffBottom + x]);

      const WSimdVec4f leftToRight = rightPos - leftPos;
      const WSimdVec4f bottomToTop = topPos - bottomPos;
      WSimdVec4f normal = -leftToRight.CrossRH(bottomToTop);
      normal.NormalizeIfNotZero<3>(WSimdVec4f(0, 0, 1));

      WSimdVec4f tangent = leftToRight;
      tangent.NormalizeIfNotZero<3>(WSimdVec4f(1, 0, 0));

      ntt[vidx].EncodeNormal(WSimdConversion::ToVec3(normal));
      ntt[vidx].EncodeTangent(WSimdConversion::ToVec3(tangent), 1.0f);

      leftIdx = x;
    }

    topIdx = y;
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
