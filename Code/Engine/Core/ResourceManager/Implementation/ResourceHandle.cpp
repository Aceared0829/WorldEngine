#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>

WTypelessResourceHandle::WTypelessResourceHandle(WResource* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(m_pResource, this);
  }
}

void WTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
  {
    DecreaseResourceRefCount(m_pResource, this);
  }

  m_pResource = nullptr;
}

WUInt64 WTypelessResourceHandle::GetResourceIDHash() const
{
  return IsValid() ? m_pResource->GetResourceIDHash() : 0;
}

WStringView WTypelessResourceHandle::GetResourceID() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceID();
  }

  return {};
}

WStringView WTypelessResourceHandle::GetResourceIdOrDescription() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceIdOrDescription();
  }

  return {};
}

const WRTTI* WTypelessResourceHandle::GetResourceType() const
{
  return IsValid() ? m_pResource->GetDynamicRTTI() : nullptr;
}

void WTypelessResourceHandle::operator=(const WTypelessResourceHandle& rhs)
{
  W_ASSERT_DEBUG(this != &rhs, "Cannot assign a resource handle to itself! This would invalidate the handle.");

  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(reinterpret_cast<WResource*>(m_pResource), this);
  }
}

void WTypelessResourceHandle::operator=(WTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;

  if (m_pResource)
  {
    MigrateResourceRefCount(m_pResource, &rhs, this);
  }
}

// static
void WResourceHandleStreamOperations::WriteHandle(WStreamWriter& Stream, const WResource* pResource)
{
  if (pResource != nullptr)
  {
    Stream << pResource->GetDynamicRTTI()->GetTypeName();
    Stream << pResource->GetResourceID();
  }
  else
  {
    const char* szEmpty = "";
    Stream << szEmpty;
  }
}

// static
void WResourceHandleStreamOperations::ReadHandle(WStreamReader& Stream, WTypelessResourceHandle& ResourceHandle)
{
  WStringBuilder sTemp;

  Stream >> sTemp;
  if (sTemp.IsEmpty())
  {
    ResourceHandle.Invalidate();
    return;
  }

  const WRTTI* pRtti = WResourceManager::FindResourceForAssetType(sTemp);

  if (pRtti == nullptr)
  {
    pRtti = WRTTI::FindTypeByName(sTemp);
  }

  if (pRtti == nullptr)
  {
    WLog::Error("Unknown resource type '{0}'", sTemp);
    ResourceHandle.Invalidate();
  }

  // read unique ID for restoring the resource (from file)
  Stream >> sTemp;

  if (pRtti != nullptr)
  {
    ResourceHandle = WResourceManager::LoadResourceByType(pRtti, sTemp);
  }
}
