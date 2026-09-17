#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonResource, 1, WRTTIDefaultAllocator<WSkeletonResource>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_RESOURCE_IMPLEMENT_COMMON_CODE(WSkeletonResource);
// clang-format on

WSkeletonResource::WSkeletonResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WSkeletonResource::~WSkeletonResource() = default;

W_RESOURCE_IMPLEMENT_CREATEABLE(WSkeletonResource, WSkeletonResourceDescriptor)
{
  m_pDescriptor = W_DEFAULT_NEW(WSkeletonResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Loaded;

  return res;
}

WResourceLoadDesc WSkeletonResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  WResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = WResourceState::Unloaded;

  return res;
}

WResourceLoadDesc WSkeletonResource::UpdateContent(WStreamReader* Stream)
{
  W_LOG_BLOCK("WSkeletonResource::UpdateContent", GetResourceIdOrDescription());

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

  // skip the asset file header at the start of the file
  WAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = W_DEFAULT_NEW(WSkeletonResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = WResourceState::Loaded;
  return res;
}

void WSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WSkeletonResource); // TODO
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WSkeletonResourceDescriptor::WSkeletonResourceDescriptor() = default;
WSkeletonResourceDescriptor::~WSkeletonResourceDescriptor() = default;
WSkeletonResourceDescriptor::WSkeletonResourceDescriptor(WSkeletonResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

void WSkeletonResourceDescriptor::operator=(WSkeletonResourceDescriptor&& rhs)
{
  m_Skeleton = std::move(rhs.m_Skeleton);
  m_Geometry = std::move(rhs.m_Geometry);
}

WUInt64 WSkeletonResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Geometry.GetHeapMemoryUsage() + m_Skeleton.GetHeapMemoryUsage();
}

WResult WSkeletonResourceDescriptor::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(8);

  m_Skeleton.Save(inout_stream);
  inout_stream << m_RootTransform;
  inout_stream << m_fMaxImpulse;

  const WUInt16 uiNumGeom = static_cast<WUInt16>(m_Geometry.GetCount());
  inout_stream << uiNumGeom;

  for (WUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    inout_stream << geo.m_uiAttachedToJoint;
    inout_stream << geo.m_Type;
    inout_stream << geo.m_Transform;

    W_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_VertexPositions));
    W_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_TriangleIndices));
  }

  // version 8
  inout_stream << m_uiLeftFootJoint;
  inout_stream << m_uiRightFootJoint;

  return W_SUCCESS;
}

WResult WSkeletonResourceDescriptor::Deserialize(WStreamReader& inout_stream)
{
  const WTypeVersion version = inout_stream.ReadVersion(8);

  if (version < 6)
    return W_FAILURE;

  m_Skeleton.Load(inout_stream);

  inout_stream >> m_RootTransform;

  if (version >= 7)
  {
    inout_stream >> m_fMaxImpulse;
  }

  m_Geometry.Clear();

  WUInt16 uiNumGeom = 0;
  inout_stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (WUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    inout_stream >> geo.m_uiAttachedToJoint;
    inout_stream >> geo.m_Type;
    inout_stream >> geo.m_Transform;

    if (version <= 6)
    {
      WStringBuilder sName;
      WSurfaceResourceHandle hSurface;
      WUInt8 uiCollisionLayer;

      inout_stream >> sName;
      inout_stream >> hSurface;
      inout_stream >> uiCollisionLayer;
    }

    if (version >= 7)
    {
      W_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_VertexPositions));
      W_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_TriangleIndices));
    }
  }

  if (version >= 8)
  {
    inout_stream >> m_uiLeftFootJoint;
    inout_stream >> m_uiRightFootJoint;
  }

  // make sure the geometry is sorted by bones
  // this allows to make the algorithm for creating the bone geometry more efficient
  m_Geometry.Sort([](const WSkeletonResourceGeometry& lhs, const WSkeletonResourceGeometry& rhs) -> bool
    { return lhs.m_uiAttachedToJoint < rhs.m_uiAttachedToJoint; });

  return W_SUCCESS;
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonResource);
