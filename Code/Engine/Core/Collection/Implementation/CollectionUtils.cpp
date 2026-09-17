#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void WCollectionUtils::AddFiles(WCollectionResourceDescriptor& ref_collection, WStringView sAssetTypeNameView, WStringView sAbsPathToFolder, WStringView sFileExtension, WStringView sStripPrefix, WStringView sPrependPrefix)
{
#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

  const WUInt32 uiStripPrefixLength = WStringUtils::GetCharacterCount(sStripPrefix.GetStartPointer(), sStripPrefix.GetEndPointer());

  WFileSystemIterator fsIt;
  fsIt.StartSearch(sAbsPathToFolder, WFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  WStringBuilder sFullPath;
  WHashedString sAssetTypeName;
  sAssetTypeName.Assign(sAssetTypeNameView);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (WPathUtils::HasExtension(stats.m_sName, sFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(sPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry = ref_collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }

#else
  W_IGNORE_UNUSED(ref_collection);
  W_IGNORE_UNUSED(sAssetTypeNameView);
  W_IGNORE_UNUSED(sAbsPathToFolder);
  W_IGNORE_UNUSED(sFileExtension);
  W_IGNORE_UNUSED(sStripPrefix);
  W_IGNORE_UNUSED(sPrependPrefix);
  W_ASSERT_NOT_IMPLEMENTED;
#endif
}


W_CORE_DLL void WCollectionUtils::MergeCollections(WCollectionResourceDescriptor& ref_result, WArrayPtr<const WCollectionResourceDescriptor*> inputCollections)
{
  WMap<WString, const WCollectionEntry*> firstEntryOfID;

  for (const WCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const WCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        ref_result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


W_CORE_DLL void WCollectionUtils::DeDuplicateEntries(WCollectionResourceDescriptor& ref_result, const WCollectionResourceDescriptor& input)
{
  const WCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(ref_result, WArrayPtr<const WCollectionResourceDescriptor*>(&firstInput, 1));
}

void WCollectionUtils::AddResourceHandle(WCollectionResourceDescriptor& ref_collection, WTypelessResourceHandle hHandle, WStringView sAssetTypeName, WStringView sAbsFolderpath)
{
  if (!hHandle.IsValid())
    return;

  const WStringView resID = hHandle.GetResourceID();

  auto& entry = ref_collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(sAssetTypeName);
  entry.m_sResourceID = resID;

  WStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (!sAbsFolderpath.IsEmpty())
  {
    WStringView root, relFile;
    WPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = sAbsFolderpath;
    absFilename.AppendPath(relFile);
    absFilename.MakeCleanPath();

    WFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && WFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}
