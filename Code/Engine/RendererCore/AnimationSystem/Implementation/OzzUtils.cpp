#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

WOzzArchiveData::WOzzArchiveData() = default;
WOzzArchiveData::~WOzzArchiveData() = default;

WResult WOzzArchiveData::FetchRegularFile(const char* szFile)
{
  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(szFile));

  m_Storage.Clear();
  m_Storage.Reserve(file.GetFileSize());
  m_Storage.ReadAll(file);

  return W_SUCCESS;
}

WResult WOzzArchiveData::FetchEmbeddedArchive(WStreamReader& inout_stream)
{
  char szTag[8] = "";

  inout_stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!WStringUtils::IsEqual(szTag, "WOzzAr"))
    return W_FAILURE;

  /*const WTypeVersion version =*/inout_stream.ReadVersion(1);

  WUInt64 uiArchiveSize = 0;
  inout_stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(inout_stream, uiArchiveSize);

  if (m_Storage.GetStorageSize64() != uiArchiveSize)
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WOzzArchiveData::StoreEmbeddedArchive(WStreamWriter& inout_stream) const
{
  const char szTag[8] = "WOzzAr";

  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 8));

  inout_stream.WriteVersion(1);

  const WUInt64 uiArchiveSize = m_Storage.GetStorageSize64();

  inout_stream << uiArchiveSize;

  return m_Storage.CopyToStream(inout_stream);
}

WOzzStreamReader::WOzzStreamReader(const WOzzArchiveData& data)
  : m_Reader(&data.m_Storage)
{
}

bool WOzzStreamReader::opened() const
{
  return true;
}

size_t WOzzStreamReader::Read(void* pBuffer, size_t uiSize)
{
  return static_cast<size_t>(m_Reader.ReadBytes(pBuffer, uiSize));
}

size_t WOzzStreamReader::Write(const void* pBuffer, size_t uiSize)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int WOzzStreamReader::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(iOffset);
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int WOzzStreamReader::Tell() const
{
  return static_cast<int>(m_Reader.GetReadPosition());
}

size_t WOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount64());
}

WOzzStreamWriter::WOzzStreamWriter(WOzzArchiveData& ref_data)
  : m_Writer(&ref_data.m_Storage)
{
}

bool WOzzStreamWriter::opened() const
{
  return true;
}

size_t WOzzStreamWriter::Read(void* pBuffer, size_t uiSize)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t WOzzStreamWriter::Write(const void* pBuffer, size_t uiSize)
{
  if (m_Writer.WriteBytes(pBuffer, uiSize).Failed())
    return 0;

  return uiSize;
}

int WOzzStreamWriter::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(iOffset);
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int WOzzStreamWriter::Tell() const
{
  return static_cast<int>(m_Writer.GetWritePosition());
}

size_t WOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount64());
}

void WOzzUtils::CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc)
{
  WOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    WOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    WOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}

W_RENDERERCORE_DLL void WOzzUtils::CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc)
{
  WOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    WOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    WOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}
