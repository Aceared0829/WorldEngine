#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/InputContexts/EditorInputContext.h>

/// Input context for the mesh asset editor that handles Ctrl+Middle click to open the material at the cursor.
///
/// Ctrl+Middle click fires a picking query to determine the material slot under the cursor,
/// then opens the corresponding material document. Used only in the mesh asset preview.
class W_EDITORPLUGINASSETS_DLL WMeshEditorInputContext : public WEditorInputContext
{
  W_ADD_DYNAMIC_REFLECTION(WMeshEditorInputContext, WEditorInputContext);

public:
  WMeshEditorInputContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView);

protected:
  virtual void OnSetOwner(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView) override {}
  virtual WEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
};
