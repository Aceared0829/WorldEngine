#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

/// Data returned by WResourceTypeLoader implementations.
struct W_CORE_DLL WResourceLoadData
{
  /// Additional (optional) description that can help during debugging (e.g. the final file path).
  WString m_sResourceDescription;

  /// Used to keep track when the loaded file was modified last and thus when reloading of the resource might be necessary.
  WTimestamp m_LoadedFileModificationDate;

  /// All loaded data should be stored in a memory stream. This stream reader allows the resource to read the memory stream.
  WStreamReader* m_pDataStream = nullptr;

  /// Custom loader data, e.g. a pointer to a custom memory block, that needs to be freed when the resource is done updating.
  void* m_pCustomLoaderData = nullptr;
};

/// Base class for all resource loaders.
///
/// A resource loader handles preparing the data before the resource is updated with the data.
/// Resource loaders are always executed on a separate thread.
class W_CORE_DLL WResourceTypeLoader
{
public:
  WResourceTypeLoader() = default;
  virtual ~WResourceTypeLoader() = default;

  /// Override this function to implement the resource loading.
  ///
  /// This function should take the information from \a pResource, e.g. which file to load, and do the loading work.
  /// It should allocate temporary storage for the loaded data and encode it in a memory stream, such that the
  /// resource can read all necessary information from the stream.
  ///
  /// \sa WResourceLoadData
  virtual WResourceLoadData OpenDataStream(const WResource* pResource) = 0;

  /// This function is called when the resource has been updated with the data from the resource loader and the loader can deallocate
  /// any temporary memory.
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) = 0;

  /// If this function returns true, a resource is unloaded and loaded again to update its content.
  ///
  /// Call WResource::GetLoadedFileModificationTime() to query the file modification time that was returned
  /// through WResourceLoadData::m_LoadedFileModificationDate.
  virtual bool IsResourceOutdated(const WResource* pResource) const
  {
    W_IGNORE_UNUSED(pResource);
    return false;
  }
};

/// A default implementation of WResourceTypeLoader for standard file loading.
///
/// The loader will interpret the WResource 'resource ID' as a path, read that full file into a memory stream.
/// The file modification data is stored as well.
/// Resources that use this loader can update their data as if they were reading the file directly.
class W_CORE_DLL WResourceLoaderFromFile : public WResourceTypeLoader
{
public:
  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;
};


/// A resource loader that is mainly used to update a resource on the fly with custom data, e.g. in an editor
///
/// Use like this:
/// Allocate a WResourceLoaderFromMemory instance on the heap, using W_DEFAULT_NEW and store the result in a
/// WUniquePtr<WResourceTypeLoader>. Then set the description, the modification time (simply use WTimestamp::CurrentTimestamp()), and the
/// custom data. Use a WMemoryStreamWriter to write your custom data. Make sure to write EXACTLY the same format that the targeted resource
/// type would read, including all data that would typically be written by outside code, e.g. the default WResourceLoaderFromFile
/// additionally writes the path to the resource at the start of the stream. If such data is usually present in the stream, you must write
/// this yourself. Then call WResourceManager::UpdateResourceWithCustomLoader(), specify the target resource and std::move your created
/// loader in there.
class W_CORE_DLL WResourceLoaderFromMemory : public WResourceTypeLoader
{
public:
  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;

  WString m_sResourceDescription;
  WTimestamp m_ModificationTimestamp;
  WDefaultMemoryStreamStorage m_CustomData;

private:
  WMemoryStreamReader m_Reader;
};
