#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

W_IMPLEMENT_SINGLETON(WPrefabCache);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, WPrefabCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WPrefabCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPrefabCache* pDummy = WPrefabCache::GetSingleton();
    W_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WPrefabCache::WPrefabCache()
  : m_SingletonRegistrar(this)
{
}

const WStringBuilder& WPrefabCache::GetCachedPrefabDocument(const WUuid& documentGuid)
{
  PrefabData& data = WPrefabCache::GetOrCreatePrefabCache(documentGuid);
  return data.m_sDocContent;
}

const WAbstractObjectGraph* WPrefabCache::GetCachedPrefabGraph(const WUuid& documentGuid)
{
  PrefabData& data = WPrefabCache::GetOrCreatePrefabCache(documentGuid);
  if (data.m_sAbsPath.IsEmpty())
    return nullptr;
  return &data.m_Graph;
}

void WPrefabCache::LoadGraph(WAbstractObjectGraph& out_graph, WStringView sGraph)
{
  WUInt64 uiHash = WHashingUtils::xxHash64(sGraph.GetStartPointer(), sGraph.GetElementCount());
  auto it = m_CachedGraphs.Find(uiHash);
  if (!it.IsValid())
  {
    it = m_CachedGraphs.Insert(uiHash, WUniquePtr<WAbstractObjectGraph>(W_DEFAULT_NEW(WAbstractObjectGraph)));

    WRawMemoryStreamReader stringReader(sGraph.GetStartPointer(), sGraph.GetElementCount());
    WUniquePtr<WAbstractObjectGraph> header;
    WUniquePtr<WAbstractObjectGraph> types;
    WAbstractGraphDdlSerializer::ReadDocument(stringReader, header, it.Value(), types, true).IgnoreResult();
  }

  it.Value()->Clone(out_graph);
}

WPrefabCache::PrefabData& WPrefabCache::GetOrCreatePrefabCache(const WUuid& documentGuid)
{
  auto it = m_PrefabData.Find(documentGuid);

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  if (it.IsValid())
  {
    WFileStats Stats;
    if (WOSFile::GetFileStats(it.Value()->m_sAbsPath, Stats).Succeeded() && !Stats.m_LastModificationTime.Compare(it.Value()->m_fileModifiedTime, WTimestamp::CompareMode::FileTimeEqual))
    {
      UpdatePrefabData(*it.Value().Borrow());
    }
  }
  else
  {
    it = m_PrefabData.Insert(documentGuid, WUniquePtr<PrefabData>(W_DEFAULT_NEW(PrefabData)));

    it.Value()->m_documentGuid = documentGuid;
    it.Value()->m_sAbsPath = WToolsProject::GetSingleton()->GetPathForDocumentGuid(documentGuid);
    if (it.Value()->m_sAbsPath.IsEmpty())
    {
      WStringBuilder sGuid;
      WConversionUtils::ToString(documentGuid, sGuid);
      WLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
    }
    else
      UpdatePrefabData(*it.Value().Borrow());
  }
#else
  W_ASSERT_NOT_IMPLEMENTED;
#endif

  return *it.Value().Borrow();
}

void WPrefabCache::UpdatePrefabData(PrefabData& data)
{
#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  if (data.m_sAbsPath.IsEmpty())
  {
    data.m_sAbsPath = WToolsProject::GetSingleton()->GetPathForDocumentGuid(data.m_documentGuid);
    if (data.m_sAbsPath.IsEmpty())
    {
      WStringBuilder sGuid;
      WConversionUtils::ToString(data.m_documentGuid, sGuid);
      WLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
      return;
    }
  }

  WFileStats Stats;
  bool bStat = WOSFile::GetFileStats(data.m_sAbsPath, Stats).Succeeded();

  if (!bStat)
  {
    WLog::Error("Can't update prefab file '{0}', the file can't be opened.", data.m_sAbsPath);
    return;
  }

  data.m_sDocContent = WPrefabUtils::ReadDocumentAsString(data.m_sAbsPath);

  if (data.m_sDocContent.IsEmpty())
    return;

  data.m_fileModifiedTime = Stats.m_LastModificationTime;
  data.m_Graph.Clear();
  WPrefabUtils::LoadGraph(data.m_Graph, data.m_sDocContent);
#else
  W_ASSERT_NOT_IMPLEMENTED;
#endif
}
