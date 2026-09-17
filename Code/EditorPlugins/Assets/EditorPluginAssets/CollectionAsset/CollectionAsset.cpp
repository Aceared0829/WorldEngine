#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCollectionAssetEntry, 1, WRTTIDefaultAllocator<WCollectionAssetEntry>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sLookupName),
    W_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new WAssetBrowserAttribute("", "*", WDependencyFlags::Package), new WRequiredAttribute())
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCollectionAssetData, 1, WRTTIDefaultAllocator<WCollectionAssetData>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCollectionAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCollectionAssetDocument::WCollectionAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WCollectionAssetData>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

static bool InsertEntry(WStringView sID, WStringView sLookupName, WMap<WString, WCollectionEntry>& inout_found)
{
  auto it = inout_found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return true;
  }

  WStringBuilder tmp;
  WAssetCurator::WLockedSubAsset pInfo = WAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    // this happens for non-asset types (e.g. 'xyz.color' and other non-asset file types)
    // these are benign and can just be skipped
    return false;
  }

  // insert item itself
  {
    WCollectionEntry& entry = inout_found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID = sID;
    entry.m_sAssetTypeName = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // insert dependencies
  {
    const WAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const WString& doc : pDocInfo->m_PackageDependencies)
    {
      // ignore return value, we are only interested in top-level information
      InsertEntry(doc, {}, inout_found);
    }
  }

  return true;
}

WTransformStatus WCollectionAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  const WCollectionAssetData* pProp = GetProperties();

  WMap<WString, WCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    if (!InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries))
    {
      // this should be treated as an error for top-level references, since they are manually added (in contrast to the transitive dependencies)
      return WStatus(WFmt("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset));
    }
  }

  WCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return WStatus(W_SUCCESS);
}
