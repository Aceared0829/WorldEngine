#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct WSelectionManagerEvent;
class WDocumentObject;
class WVisualizerAttribute;

struct W_GUIFOUNDATION_DLL WVisualizerManagerEvent
{
  const WDocument* m_pDocument;
  const WDeque<const WDocumentObject*>* m_pSelection;
};

class W_GUIFOUNDATION_DLL WVisualizerManager
{
  W_DECLARE_SINGLETON(WVisualizerManager);

public:
  WVisualizerManager();
  ~WVisualizerManager();

  void SetVisualizersActive(const WDocument* pDoc, bool bActive);
  bool GetVisualizersActive(const WDocument* pDoc);

  WEvent<const WVisualizerManagerEvent&> m_Events;

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void SendEventToRecreateVisualizers(const WDocument* pDoc);

  struct DocData
  {
    bool m_bActivated;

    DocData() { m_bActivated = true; }
  };

  WMap<const WDocument*, DocData> m_DocsSubscribed;
};
