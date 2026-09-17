#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct WVisualizerManagerEvent;
class WDocument;

class W_EDITORFRAMEWORK_DLL WVisualizerAdapterRegistry
{
  W_DECLARE_SINGLETON(WVisualizerAdapterRegistry);

public:
  WVisualizerAdapterRegistry();
  ~WVisualizerAdapterRegistry();

  WRttiMappedObjectFactory<WVisualizerAdapter> m_Factory;

private:
  void VisualizerManagerEventHandler(const WVisualizerManagerEvent& e);
  void ClearAdapters(const WDocument* pDocument);
  void CreateAdapters(const WDocument* pDocument, const WDocumentObject* pObject);

  struct Data
  {
    WHybridArray<WVisualizerAdapter*, 8> m_Adapters;
  };

  WMap<const WDocument*, Data> m_DocumentAdapters;
};
