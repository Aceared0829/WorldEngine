#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Uuid.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WEditorEngineSyncObjectMsg;
class WEditorEngineSyncObject;
class WEditorEngineDocumentMsg;
class WEngineProcessViewContext;
class WEngineProcessCommunicationChannel;
class WProcessMessage;
class WExportDocumentMsgToEngine;
class WCreateThumbnailMsgToEngine;
struct WResourceEvent;
class WRenderGraph;
struct WGALDeviceEvent;

struct WEngineProcessDocumentContextFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,
    CreateWorld = W_BIT(0),
    Default = None
  };

  struct Bits
  {
    StorageType CreateWorld : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WEngineProcessDocumentContextFlags);

/// A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a WEngineProcessDocumentContext
/// created in the engine process.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineProcessDocumentContext : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEngineProcessDocumentContext, WReflectedClass);

public:
  WEngineProcessDocumentContext(WBitflags<WEngineProcessDocumentContextFlags> flags);
  virtual ~WEngineProcessDocumentContext();

  virtual void Initialize(const WUuid& documentGuid, const WVariant& metaData, WEngineProcessCommunicationChannel* pIPC, WStringView sDocumentType);
  void Deinitialize();

  /// Returns the document type for which this context was created. Useful in case a context may be used for multiple document types.
  WStringView GetDocumentType() const { return m_sDocumentType; }

  void SendProcessMessage(WProcessMessage* pMsg = nullptr);
  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg);

  static WEngineProcessDocumentContext* GetDocumentContext(WUuid guid);
  static void AddDocumentContext(WUuid guid, const WVariant& metaData, WEngineProcessDocumentContext* pView, WEngineProcessCommunicationChannel* pIPC, WStringView sDocumentType);
  static bool PendingOperationsInProgress();
  static void UpdateDocumentContexts();
  static void DestroyDocumentContext(WUuid guid);

  /// Replaces handle based log links (see WArgGameObject / WArgComponent) with links that the editor can navigate to.
  ///
  /// Engine side code can only log object and component handles, since it doesn't know the document and object GUIDs
  /// that the editor uses. This function looks the handles up in the document contexts and rewrites the link targets
  /// to the 'asset:<doc-guid>#<obj-guid>' form. Links whose handle can't be resolved (e.g. because the object was
  /// deleted, or the message came from a world that isn't an editor document) are replaced by their display text.
  ///
  /// Returns true if ref_sMessage was modified.
  static bool ResolveLogLinks(WStringBuilder& ref_sMessage);

  /// Returns the bounding box of the objects in the world.
  WBoundingBoxSphere GetWorldBounds(WWorld* pWorld);

  void ProcessEditorEngineSyncObjectMsg(const WEditorEngineSyncObjectMsg& msg);

  const WUuid& GetDocumentGuid() const { return m_DocumentGuid; }

  virtual void Reset();
  void ClearExistingObjects();

  WIPCObjectMirrorEngine m_Mirror;
  WWorldRttiConverterContext m_Context; // TODO: Move actual context into the EngineProcessDocumentContext
  virtual WWorldRttiConverterContext& GetContext() { return m_Context; }
  virtual const WWorldRttiConverterContext& GetContext() const { return m_Context; }

  WWorld* GetWorld() const { return m_pWorld; }

  /// Tries to resolve a 'reference' (given in pData) to an WGameObject.
  virtual WGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, WComponentHandle hThis, WStringView sProperty) const;

protected:
  virtual void OnInitialize();
  virtual void OnDeinitialize();

  /// Needs to be implemented to create a view context used for windows and thumbnails rendering.
  virtual WEngineProcessViewContext* CreateViewContext() = 0;
  /// Needs to be implemented to destroy the view context created in CreateViewContext.
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) = 0;

  /// Should return true if this context has any operation in progress like thumbnail rendering
  /// and thus needs to continue rendering even if no new messages from the editor come in.
  virtual bool PendingOperationInProgress() const;

  /// A tick functions that allows each document context to do processing that continues
  /// over multiple frames and can't be handled in HandleMessage directly.
  ///
  /// Make sure to call the base implementation when overwriting as this handles the thumbnail
  /// rendering that takes multiple frames to complete.
  virtual void UpdateDocumentContext();

  /// Exports to current document resource to file. Make sure to write WAssetFileHeader at the start of it.
  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg);
  void UpdateSyncObjects();

  /// Creates the thumbnail view context. It uses 'CreateViewContext' in combination with an off-screen render target.
  void CreateThumbnailViewContext(const WCreateThumbnailMsgToEngine* pMsg);

  /// Once a thumbnail is successfully rendered, the thumbnail view context is destroyed again.
  void DestroyThumbnailViewContext();

  /// Overwrite this function to apply the thumbnail render settings to the given context.
  ///
  /// Return false if you need more frames to be rendered to setup everything correctly.
  /// If true is returned for 'ThumbnailConvergenceFramesTarget' frames in a row the thumbnail image is taken.
  /// This is to allow e.g. camera updates after more resources have been streamed in. The frame counter
  /// will start over to count to 'ThumbnailConvergenceFramesTarget' when a new resource is being loaded
  /// to make sure we do not make an image of half-streamed in data.
  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext);

  /// Called before a thumbnail context is created.
  virtual void OnThumbnailViewContextRequested() {}
  /// Called after a thumbnail context was created. Allows to insert code before the thumbnail is generated.
  virtual void OnThumbnailViewContextCreated();
  /// Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext();

  WWorld* m_pWorld = nullptr;

  /// Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const WUuid& object, const char* szTag, bool bSet, bool recursive);

  /// Sets the given tag on the object and all children.
  void SetTagRecursive(WGameObject* pObject, const WTag& tag);
  /// Clears the given tag on the object and all children.
  void ClearTagRecursive(WGameObject* pObject, const WTag& tag);

protected:
  const WEngineProcessViewContext* GetViewContext(WUInt32 uiView) const
  {
    return uiView >= m_ViewContexts.GetCount() ? nullptr : m_ViewContexts[uiView];
  }

private:
  friend class WEditorEngineSyncObject;

  void AddSyncObject(WEditorEngineSyncObject* pSync);
  void RemoveSyncObject(WEditorEngineSyncObject* pSync);
  WEditorEngineSyncObject* FindSyncObject(const WUuid& guid);


private:
  void ClearViewContexts();

  // Maps a document guid to the corresponding context that handles that document on the engine side
  static WHashTable<WUuid, WEngineProcessDocumentContext*> s_DocumentContexts;

  /// Removes all sync objects that are tied to this context
  void CleanUpContextSyncObjects();

protected:
  WBitflags<WEngineProcessDocumentContextFlags> m_Flags;
  WUuid m_DocumentGuid;
  WVariant m_MetaData;

  WEngineProcessCommunicationChannel* m_pIPC = nullptr;
  WHybridArray<WEngineProcessViewContext*, 4> m_ViewContexts;

  WMap<WUuid, WEditorEngineSyncObject*> m_SyncObjects;

private:
  enum Constants
  {
    ThumbnailSuperscaleFactor =
      2,                                 ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be power-of-two.
    ThumbnailConvergenceFramesTarget = 4 ///< Due to multi-threaded rendering, this must be at least 4
  };

  WUInt8 m_uiThumbnailConvergenceFrames = 0;
  WUInt16 m_uiThumbnailWidth = 0;
  WUInt16 m_uiThumbnailHeight = 0;
  WEngineProcessViewContext* m_pThumbnailViewContext = nullptr;
  WGALRenderTargets m_ThumbnailRenderTargets;
  WGALTextureHandle m_hThumbnailColorRT;
  WGALTextureHandle m_hThumbnailDepthRT;
  WGALReadbackTextureHelper m_ThumbnailReadback;

  bool m_bThumbnailReadbackRequested = false;
  bool m_bThumbnailReadbackInFlight = false;
  WGALTextureCreationDescription m_ThumbnailColorDesc;

  bool m_bWorldSimStateBeforeThumbnail = false;
  WString m_sDocumentType;

  WSharedPtr<WRenderGraph> m_pRenderGraph;

  void OnGALEvent(const WGALDeviceEvent& e);

  //////////////////////////////////////////////////////////////////////////
  // GameObject reference resolution
private:
  struct GoReferenceTo
  {
    WStringView m_sComponentProperty;
    WUuid m_ReferenceToGameObject;
  };

  struct GoReferencedBy
  {
    WStringView m_sComponentProperty;
    WUuid m_ReferencedByComponent;
  };

  // Components reference GameObjects
  mutable WMap<WUuid, WHybridArray<GoReferenceTo, 4>> m_GoRef_ReferencesTo;

  // GameObjects referenced by Components
  mutable WMap<WUuid, WHybridArray<GoReferencedBy, 4>> m_GoRef_ReferencedBy;

  void WorldRttiConverterContextEventHandler(const WWorldRttiConverterContext::Event& e);
};
