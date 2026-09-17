#pragma once

#include <Core/Collection/CollectionResource.h>

class WHashedString;

namespace WCollectionUtils
{
  /// Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see WResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  W_CORE_DLL void AddFiles(WCollectionResourceDescriptor& ref_collection, WStringView sAssetTypeName, WStringView sAbsPathToFolder,
    WStringView sFileExtension, WStringView sStripPrefix, WStringView sPrependPrefix);

  /// Merges all collections from the input array into the target result collection. Resource entries will be de-duplicated by resource ID
  /// string.
  W_CORE_DLL void MergeCollections(WCollectionResourceDescriptor& ref_result, WArrayPtr<const WCollectionResourceDescriptor*> inputCollections);

  /// Special case of WCollectionUtils::MergeCollections which outputs unique entries from input collection into the result collection
  W_CORE_DLL void DeDuplicateEntries(WCollectionResourceDescriptor& ref_result, const WCollectionResourceDescriptor& input);

  /// Extracts info (i.e. resource ID as file path) from the passed handle and adds it as a new resource entry. Does not add an entry if the
  /// resource handle is not valid.
  ///
  /// The resource type identifier must be passed explicity as szAssetTypeName (see WResourceManager::RegisterResourceForAssetType). To determine the
  /// file size, the resource ID is used as a filename passed to WFileSystem::GetFileStats. In case the resource's path root is not mounted, the path
  /// root can be replaced by passing non-NULL string to szAbsFolderpath, which will replace the root, e.g. with an absolute file path. This is just
  /// for the file size check within the scope of the function, it will not modify the resource Id.
  W_CORE_DLL void AddResourceHandle(WCollectionResourceDescriptor& ref_collection, WTypelessResourceHandle hHandle, WStringView sAssetTypeName, WStringView sAbsFolderpath);

}; // namespace WCollectionUtils
