#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/Blob.h>

#include <Foundation/Memory/AllocatorWithPolicy.h>

WBlob::WBlob() = default;

WBlob::WBlob(WBlob&& other)
{
  m_pStorage = other.m_pStorage;
  m_uiSize = other.m_uiSize;

  other.m_pStorage = nullptr;
  other.m_uiSize = 0;
}

void WBlob::operator=(WBlob&& rhs)
{
  Clear();

  m_pStorage = rhs.m_pStorage;
  m_uiSize = rhs.m_uiSize;

  rhs.m_pStorage = nullptr;
  rhs.m_uiSize = 0;
}

WBlob::~WBlob()
{
  Clear();
}

void WBlob::SetFrom(const void* pSource, WUInt64 uiSize)
{
  SetCountUninitialized(uiSize);
  WMemoryUtils::Copy(static_cast<WUInt8*>(m_pStorage), static_cast<const WUInt8*>(pSource), static_cast<size_t>(uiSize));
}

void WBlob::Clear()
{
  if (m_pStorage)
  {
    WFoundation::GetAlignedAllocator()->Deallocate(m_pStorage);
    m_pStorage = nullptr;
    m_uiSize = 0;
  }
}

void WBlob::SetCountUninitialized(WUInt64 uiCount)
{
  if (m_uiSize != uiCount)
  {
    Clear();

    m_pStorage = WFoundation::GetAlignedAllocator()->Allocate(WMath::SafeConvertToSizeT(uiCount), 64u);
    m_uiSize = uiCount;
  }
}

void WBlob::ZeroFill()
{
  if (m_pStorage)
  {
    WMemoryUtils::ZeroFill(static_cast<WUInt8*>(m_pStorage), static_cast<size_t>(m_uiSize));
  }
}

bool WBlob::IsEmpty() const
{
  return 0 == m_uiSize;
}
