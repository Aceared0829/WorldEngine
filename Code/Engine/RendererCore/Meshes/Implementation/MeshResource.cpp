#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMeshResource, 1, WRTTIDefaultAllocator<WMeshResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WMeshResource);
// clang-format on

WUInt32 WMeshResource::s_uiMeshBufferNameSuffix = 0;

WMeshResource::WMeshResource()
  : WResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
  m_Bounds = WBoundingBoxSphere::MakeInvalid();
}

WResourceLoadDesc WMeshResource::UnloadData(Unload WhatToUnload)
{
  WResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_SubMeshes.Compact();
    m_Materials.Clear();
    m_Materials.Compact();
    m_Bones.Clear();
    m_Bones.Compact();

    m_hMeshBuffer.Invalidate();
    m_hDefaultSkeleton.Invalidate();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = WResourceState::Unloaded;
  }

  return res;
}

WResourceLoadDesc WMeshResource::UpdateContent(WStreamReader* Stream)
{
  WMeshResourceDescriptor desc;
  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  WStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = WResourceState::LoadedResourceMissing;
    return res;
  }

  m_uiAssetHash = AssetHash.GetFileHash();

  return CreateResource(std::move(desc));
}

void WMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WMeshResource) + (WUInt32)m_SubMeshes.GetHeapMemoryUsage() + (WUInt32)m_Materials.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

W_RESOURCE_IMPLEMENT_CREATEABLE(WMeshResource, WMeshResourceDescriptor)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = descriptor.GetExistingMeshBuffer();

  m_hDefaultSkeleton = descriptor.m_hDefaultSkeleton;
  m_Bones = descriptor.m_Bones;
  m_fMaxBoneVertexOffset = descriptor.m_fMaxBoneVertexOffset;

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_uiMeshBufferNameSuffix++;
    WStringBuilder sMbName;
    sMbName.SetFormat("{0}  [MeshBuffer {1}]", GetResourceID(), WArgU(s_uiMeshBufferNameSuffix, 4, true, 16, true));

    // note: this gets move'd, might be invalid afterwards
    WMeshBufferResourceDescriptor& mb = descriptor.MeshBufferDesc();

    m_hMeshBuffer = WResourceManager::CreateResource<WMeshBufferResource>(sMbName, std::move(mb), GetResourceDescription());
  }

  m_SubMeshes = descriptor.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(descriptor.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : descriptor.GetMaterials())
  {
    WMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = WResourceManager::LoadResource<WMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = descriptor.GetBounds();
  W_ASSERT_DEV(m_Bounds.IsValid(), "The mesh bounds are invalid. Make sure to call WMeshResourceDescriptor::ComputeBounds()");

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);
