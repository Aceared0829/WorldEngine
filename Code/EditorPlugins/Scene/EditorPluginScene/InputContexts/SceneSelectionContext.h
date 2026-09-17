#pragma once

#include <EditorFramework/InputContexts/SelectionContext.h>

/// Custom selection context for the scene to allow switching the active layer if an object is clicked that is in a different layer then the active one.
class WSceneSelectionContext : public WSelectionContext
{
public:
  WSceneSelectionContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera);

protected:
  virtual void OpenDocumentForPickedObject(const WObjectPickingResult& res) const override;
  virtual void SelectPickedObject(const WObjectPickingResult& res, bool bToggle, bool bDirect) const override;

  WUuid FindLayerByObject(WUuid objectGuid, const WDocumentObject*& out_pObject) const;
};
