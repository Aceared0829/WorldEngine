#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;

/// Engine-process document context for a Kraut tree asset.
///
/// Runs inside the editor's engine process and owns the game world used for the asset preview.
/// It creates an WKrautTreeComponent driven by the generator resource, handles messages from
/// the editor (seed changes, LOD selection, bounding-box queries), and periodically sends
/// LOD triangle statistics back so the editor window can display them.
class W_ENGINEPLUGINKRAUT_DLL WKrautTreeContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WKrautTreeContext, WEngineProcessDocumentContext);

public:
  WKrautTreeContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;
  const WKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }
  WComponentHandle GetKrautComponentHandle() const { return m_hKrautComponent; }

protected:
  virtual void OnInitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);
  /// Sends per-LOD triangle and bone counts to the editor document window.
  void SendLodStats(const WUuid& documentGuid);

  /// Triangle and bone statistics for one LOD, sent to the editor for display in the stats labels.
  struct LodStats
  {
    WInt8 m_iLodIndex = -2; ///< Index of the LOD these stats describe; -2 = not yet initialized.
    WUInt16 m_uiNumBones = 0;
    WUInt32 m_uiNumTrianglesTotal = 0;
    WUInt32 m_uiNumTrianglesBranch = 0;
    WUInt32 m_uiNumTrianglesFrond = 0;
    WUInt32 m_uiNumTrianglesLeaf = 0;
  };

  WGameObject* m_pMainObject;
  WComponentHandle m_hKrautComponent;
  WComponentHandle m_hWindComponent;
  WKrautGeneratorResourceHandle m_hMainResource;
  WMeshResourceHandle m_hPreviewMeshResource;
  WUInt32 m_uiDisplayRandomSeed = 0xFFFFFFFF; ///< The seed currently shown in the preview; 0xFFFFFFFF means not yet set.
  LodStats m_LastSentLodStats;                 ///< Cached stats from the last SendLodStats() call, used to avoid redundant messages.
};
