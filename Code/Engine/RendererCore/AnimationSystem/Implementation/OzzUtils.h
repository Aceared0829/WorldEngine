#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/io/stream.h>

namespace ozz::animation
{
  class Skeleton;
  class Animation;
}; // namespace ozz::animation

/// Stores or gather the data for an ozz file, for random access operations (seek / tell).
///
/// Since ozz::io::Stream requires seek/tell functionality, it cannot be implemented with basic WStreamReader / WStreamWriter.
/// Instead, we must have the entire ozz archive data in memory, to be able to jump around arbitrarily.
class W_RENDERERCORE_DLL WOzzArchiveData
{
  W_DISALLOW_COPY_AND_ASSIGN(WOzzArchiveData);

public:
  WOzzArchiveData();
  ~WOzzArchiveData();

  WResult FetchRegularFile(const char* szFile);
  WResult FetchEmbeddedArchive(WStreamReader& inout_stream);
  WResult StoreEmbeddedArchive(WStreamWriter& inout_stream) const;

  WDefaultMemoryStreamStorage m_Storage;
};

/// Implements the ozz::io::Stream interface for reading. The data has to be present in an WOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class W_RENDERERCORE_DLL WOzzStreamReader : public ozz::io::Stream
{
public:
  WOzzStreamReader(const WOzzArchiveData& data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  WMemoryStreamReader m_Reader;
};

/// Implements the ozz::io::Stream interface for writing. The data is gathered in an WOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class W_RENDERERCORE_DLL WOzzStreamWriter : public ozz::io::Stream
{
public:
  WOzzStreamWriter(WOzzArchiveData& ref_data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  WMemoryStreamWriter m_Writer;
};

namespace WOzzUtils
{
  W_RENDERERCORE_DLL void CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc);
  W_RENDERERCORE_DLL void CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc);
} // namespace WOzzUtils
