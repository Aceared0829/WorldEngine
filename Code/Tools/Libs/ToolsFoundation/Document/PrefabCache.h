#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WAbstractObjectGraph;

class W_TOOLSFOUNDATION_DLL WPrefabCache
{
  W_DECLARE_SINGLETON(WPrefabCache);

public:
  WPrefabCache();

  const WStringBuilder& GetCachedPrefabDocument(const WUuid& documentGuid);
  const WAbstractObjectGraph* GetCachedPrefabGraph(const WUuid& documentGuid);
  void LoadGraph(WAbstractObjectGraph& out_graph, WStringView sGraph);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, WPrefabCache);

  struct PrefabData
  {
    PrefabData() = default;

    WUuid m_documentGuid;
    WString m_sAbsPath;

    WAbstractObjectGraph m_Graph;
    WStringBuilder m_sDocContent;
    WTimestamp m_fileModifiedTime;
  };
  PrefabData& GetOrCreatePrefabCache(const WUuid& documentGuid);
  void UpdatePrefabData(PrefabData& data);

  WMap<WUInt64, WUniquePtr<WAbstractObjectGraph>> m_CachedGraphs;
  WMap<WUuid, WUniquePtr<PrefabData>> m_PrefabData;
};
