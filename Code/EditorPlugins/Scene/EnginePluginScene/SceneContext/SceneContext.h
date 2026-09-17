#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class WObjectSelectionMsgToEngine;
class WRenderContext;
class WGameStateBase;
class WGameModeMsgToEngine;
class WWorldSettingsMsgToEngine;
class WObjectsForDebugVisMsgToEngine;
class WGridSettingsMsgToEngine;
class WSimulationSettingsMsgToEngine;
struct WResourceManagerEvent;
class WExposedDocumentObjectPropertiesMsgToEngine;
class WSyncChildOrderMsgToEngine;
class WViewRedrawMsgToEngine;
class WWorldWriter;
class WDeferredFileWriter;
class WLayerContext;
struct WGameApplicationExecutionEvent;

class W_ENGINEPLUGINSCENE_DLL WSceneContext : public WEngineProcessDocumentContext
{
  W_ADD_DYNAMIC_REFLECTION(WSceneContext, WEngineProcessDocumentContext);

public:
  WSceneContext();
  ~WSceneContext();

  virtual void HandleMessage(const WEditorEngineDocumentMsg* pMsg) override;

  const WDeque<WGameObjectHandle>& GetSelection() const { return m_Selection; }
  const WDeque<WGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  bool GetRenderSelectionBoxes() const { return m_bRenderSelectionBoxes; }
  float GetGridDensity() const { return WMath::Abs(m_fGridDensity); }
  bool IsGridInGlobalSpace() const { return m_fGridDensity >= 0.0f; }
  const WTransform& GetGridTransform() const { return m_GridTransform; }

  WGameStateBase* GetGameState() const;
  bool IsPlayTheGameActive() const { return GetGameState() != nullptr; }

  WUInt32 RegisterLayer(WLayerContext* pLayer);
  void UnregisterLayer(WLayerContext* pLayer);
  void AddLayerIndexTag(const WEntityMsgToEngine& msg, WWorldRttiConverterContext& ref_context, const WTag& layerTag);
  const WArrayPtr<const WTag> GetInvisibleLayerTags() const;

  WEngineProcessDocumentContext* GetActiveDocumentContext();
  const WEngineProcessDocumentContext* GetActiveDocumentContext() const;
  WWorldRttiConverterContext& GetActiveContext();
  const WWorldRttiConverterContext& GetActiveContext() const;
  WWorldRttiConverterContext* GetContextForLayer(const WUuid& layerGuid);
  WArrayPtr<WWorldRttiConverterContext*> GetAllContexts();

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual WEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(WEngineProcessViewContext* pContext) override;
  virtual WStatus ExportDocument(const WExportDocumentMsgToEngine* pMsg) override;
  void ExportExposedParameters(const WWorldWriter& ww, WDeferredFileWriter& file) const;

  virtual bool UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void OnThumbnailViewContextCreated() override;
  virtual void OnDestroyThumbnailViewContext() override;
  virtual void UpdateDocumentContext() override;
  virtual WGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, WComponentHandle hThis, WStringView sProperty) const override;

private:
  struct TagGameObject
  {
    WGameObjectHandle m_hObject;
    WTag m_Tag;
  };

  void AddAmbientLight(bool bSetEditorTag, bool bForce);
  void RemoveAmbientLight();

  void HandleViewRedrawMsg(const WViewRedrawMsgToEngine* pMsg);
  void HandleSelectionMsg(const WObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const WGameModeMsgToEngine* pMsg);
  void HandleSimulationSettingsMsg(const WSimulationSettingsMsgToEngine* msg);
  void HandleGridSettingsMsg(const WGridSettingsMsgToEngine* msg);
  void HandleWorldSettingsMsg(const WWorldSettingsMsgToEngine* msg);
  void HandleObjectsForDebugVisMsg(const WObjectsForDebugVisMsgToEngine* pMsg);
  void ComputeHierarchyBounds(WGameObject* pObj, WBoundingBoxSphere& bounds);
  void HandleExposedPropertiesMsg(const WExposedDocumentObjectPropertiesMsgToEngine* pMsg);
  void HandleSceneGeometryMsg(const WExportSceneGeometryMsgToEngine* pMsg);
  void HandlePullObjectStateMsg(const WPullObjectStateMsgToEngine* pMsg);
  void AnswerObjectStatePullRequest(const WViewRedrawMsgToEngine* pMsg);
  void HandleActiveLayerChangedMsg(const WActiveLayerChangedMsgToEngine* pMsg);
  void HandleTagMsgToEngineMsg(const WObjectTagMsgToEngine* pMsg);
  void HandleLayerVisibilityChangedMsgToEngineMsg(const WLayerVisibilityChangedMsgToEngine* pMsg);
  void HandleSyncChildOrderMsg(const WSyncChildOrderMsgToEngine* pMsg);

  void DrawSelectionBounds(const WViewHandle& hView);

  void UpdateInvisibleLayerTags();
  void InsertSelectedChildren(const WGameObject* pObject);
  void QuerySelectionBBox(const WEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted(WStringView sStartPosition, const WTransform& startPositionOffset);

  void OnResourceManagerEvent(const WResourceManagerEvent& e);
  void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);

  bool m_bUpdateAllLocalBounds = false;
  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;
  float m_fGridDensity;
  WTransform m_GridTransform;

  WDeque<WGameObjectHandle> m_Selection;
  WDeque<WGameObjectHandle> m_SelectionWithChildren;
  WSet<WGameObjectHandle> m_SelectionWithChildrenSet;
  WGameObjectHandle m_hSkyLight;
  WGameObjectHandle m_hDirectionalLight;
  WDynamicArray<WExposedSceneProperty> m_ExposedSceneProperties;

  WPushObjectStateMsgToEditor m_PushObjectStateMsg;

  WUuid m_ActiveLayer;
  WDynamicArray<WLayerContext*> m_Layers;
  WDynamicArray<WWorldRttiConverterContext*> m_Contexts;

  // We use tags in the form of Layer_4 (Layer_Scene for the scene itself) to not pollute the tag registry with hundreds of unique tags. The tags do not need to be unique across documents so we can just use the layer index but that requires the Tags to be recomputed whenever we remove / add layers.
  // By caching the guids we do not need to send another message each time a layer is loaded as we send also guids of unloaded layers.
  WTag m_LayerTag;
  WHybridArray<WUuid, 1> m_InvisibleLayers;
  bool m_bInvisibleLayersDirty = true;
  WHybridArray<WTag, 1> m_InvisibleLayerTags;

  WDynamicArray<TagGameObject> m_ObjectsToTag;

  static WWorld* s_pWorldLinkedWithGameState;
};
