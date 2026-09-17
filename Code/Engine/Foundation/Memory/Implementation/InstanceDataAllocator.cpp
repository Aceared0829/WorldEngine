#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/InstanceDataAllocator.h>

WUInt32 WInstanceDataAllocator::AddDesc(const WInstanceDataDesc& desc)
{
  m_Descs.PushBack(desc);

  const WUInt32 uiOffset = WMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
  m_uiTotalDataSize = uiOffset + desc.m_uiTypeSize;

  return uiOffset;
}

void WInstanceDataAllocator::ClearDescs()
{
  m_Descs.Clear();
  m_uiTotalDataSize = 0;
}

WBlob WInstanceDataAllocator::AllocateAndConstruct() const
{
  WBlob blob;
  if (m_uiTotalDataSize > 0)
  {
    blob.SetCountUninitialized(m_uiTotalDataSize);
    blob.ZeroFill();

    Construct(blob.GetByteBlobPtr());
  }

  return blob;
}

void WInstanceDataAllocator::DestructAndDeallocate(WBlob& ref_blob) const
{
  W_ASSERT_DEV(ref_blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");
  Destruct(ref_blob.GetByteBlobPtr());

  ref_blob.Clear();
}

void WInstanceDataAllocator::Construct(WByteBlobPtr blobPtr) const
{
  WUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = WMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_ConstructorFunction != nullptr)
    {
      desc.m_ConstructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}

void WInstanceDataAllocator::Destruct(WByteBlobPtr blobPtr) const
{
  WUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = WMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_DestructorFunction != nullptr)
    {
      desc.m_DestructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}
