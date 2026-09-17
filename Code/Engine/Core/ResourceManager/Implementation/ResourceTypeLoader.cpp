#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

struct FileResourceLoadData
{
  WBlob m_Storage;
  WRawMemoryStreamReader m_Reader;
};

WResourceLoadData WResourceLoaderFromFile::OpenDataStream(const WResource* pResource)
{
  W_PROFILE_SCOPE("ReadResourceFile");

  WResourceLoadData res;

  WFileReader File;
  if (File.Open(pResource->GetResourceID()).Failed())
    return res;

  res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats stat;
  if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }

#endif

  FileResourceLoadData* pData = W_DEFAULT_NEW(FileResourceLoadData);

  const WUInt64 uiFileSize = File.GetFileSize();

  const WUInt64 uiBlobCapacity = uiFileSize + File.GetFilePathAbsolute().GetElementCount() + 8; // +8 for the string overhead
  pData->m_Storage.SetCountUninitialized(uiBlobCapacity);

  WUInt8* pBlobPtr = pData->m_Storage.GetBlobPtr<WUInt8>().GetPtr();

  WRawMemoryStreamWriter w(pBlobPtr, uiBlobCapacity);

  // write the absolute path to the read file into the memory stream
  w << File.GetFilePathAbsolute();

  const WUInt64 uiOffset = w.GetNumWrittenBytes();

  File.ReadBytes(pBlobPtr + uiOffset, uiFileSize);

  pData->m_Reader.Reset(pBlobPtr, w.GetNumWrittenBytes() + uiFileSize);
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void WResourceLoaderFromFile::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  W_IGNORE_UNUSED(pResource);

  FileResourceLoadData* pData = static_cast<FileResourceLoadData*>(loaderData.m_pCustomLoaderData);

  W_DEFAULT_DELETE(pData);
}

bool WResourceLoaderFromFile::IsResourceOutdated(const WResource* pResource) const
{
  // if we cannot find the target file, there is no point in trying to reload it -> claim it's up to date
  if (WFileSystem::ResolvePath(pResource->GetResourceID(), nullptr, nullptr).Failed())
    return false;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    WFileStats stat;
    if (WFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), WTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

//////////////////////////////////////////////////////////////////////////

WResourceLoadData WResourceLoaderFromMemory::OpenDataStream(const WResource* pResource)
{
  W_IGNORE_UNUSED(pResource);

  m_Reader.SetStorage(&m_CustomData);
  m_Reader.SetReadPosition(0);

  WResourceLoadData res;

  res.m_sResourceDescription = m_sResourceDescription;
  res.m_LoadedFileModificationDate = m_ModificationTimestamp;
  res.m_pDataStream = &m_Reader;
  res.m_pCustomLoaderData = nullptr;

  return res;
}

void WResourceLoaderFromMemory::CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData)
{
  W_IGNORE_UNUSED(pResource);
  W_IGNORE_UNUSED(loaderData);

  m_Reader.SetStorage(nullptr);
}

bool WResourceLoaderFromMemory::IsResourceOutdated(const WResource* pResource) const
{
  if (pResource->GetLoadedFileModificationTime().IsValid() && m_ModificationTimestamp.IsValid())
  {
    if (!m_ModificationTimestamp.Compare(pResource->GetLoadedFileModificationTime(), WTimestamp::CompareMode::FileTimeEqual))
      return true;

    return false;
  }

  return true;
}
