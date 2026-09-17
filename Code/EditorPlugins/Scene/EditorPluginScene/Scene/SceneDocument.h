#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

class WExposedSceneProperty;
class WSceneDocumentSettingsBase;
class WPushObjectStateMsgToEditor;

struct GameMode
{
  enum Enum
  {
    Off,
    Simulate,
    Play,
  };
};

class W_EDITORPLUGINSCENE_DLL WSceneDocument : public WGameObjectDocument
{
  W_ADD_DYNAMIC_REFLECTION(WSceneDocument, WGameObjectDocument);

public:
  enum class DocumentType
  {
    Scene,
    Prefab,
    Layer
  };

public:
  WSceneDocument(WStringView sDocumentPath, DocumentType documentType);
  ~WSceneDocument();

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  /// Creates a new object and attaches all currently selected objects to it.
  void GroupSelection();

  /// Changes the selection to the parent object.
  void SelectParentObject();

  /// Sets the last selected object as the 'active parent'.
  void SetSelectedAsActiveParent();
  /// Clears the 'active parent' object.
  void ClearActiveParent();

  /// Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// Opens the 'Delta Transform' dialog.
  void DeltaTransform();


  /// Moves all selected objects to the editor camera position
  void SnapObjectToCamera();


  /// Attaches all selected objects to the selected object
  void AttachToObject();

  /// Detaches all selected objects from their current parent
  void DetachFromParent();

  /// Sends the ordered child object GUIDs to all selected objects that have a component with WSyncChildOrderAttribute.
  void SyncChildOrderForSelection();

  /// Iterates all objects in this document and sends child order sync for those that require it.
  virtual void SyncAllChildOrders();

  /// Puts the GUID of the single selected object into the clipboard
  void CopyReference();

  /// Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  WStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition, bool bComponentSelectionMenu);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_DocumentType == DocumentType::Prefab; }

  /// Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const WUuid& object, WUuid* out_pPrefabAssetGuid = nullptr) const;

  /// Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const WUuid& object, WUuid* out_pPrefabAssetGuid = nullptr) const;

  /// Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const override { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_sMimeType) const override;
  virtual bool Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;
  bool DuplicateSelectedObjects(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool CopySelectedObjects(WAbstractObjectGraph& ref_graph, WMap<WUuid, WUuid>* out_pParents) const;
  bool PasteAt(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, const WVec3& vPos);
  bool PasteAtOrignalPosition(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph);

  virtual void UpdatePrefabs() override;

  /// Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(WArrayPtr<const WDocumentObject*> selection) override;

  virtual WUuid ReplaceByPrefab(const WDocumentObject* pRootObject, WStringView sPrefabFile, const WUuid& prefabAsset, const WUuid& prefabSeed, bool bEnginePrefab) override;

  /// Reverts all selected editor prefabs to their original template state
  virtual WUuid RevertPrefab(const WDocumentObject* pObject) override;

  /// Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(WArrayPtr<const WDocumentObject*> selection);
  /// Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(WArrayPtr<const WDocumentObject*> selection);

  virtual WStatus CreatePrefabDocumentFromSelection(WStringView sFile, const WRTTI* pRootType, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB = {}, WDelegate<void(WDocumentObject*)> adjustNewNodesCB = {}, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {}) override;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  virtual bool CanEngineProcessBeRestarted() const override;

  void StartSimulateWorld();
  void TriggerGameModePlay(bool bUsePickedPositionAsStart);

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  void StepSimulation();
  void PauseSimulation();

  WTransformStatus ExportScene(bool bCreateThumbnail);
  void ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode /* WWorldGeoExtractionUtil::ExtractionMode */, const WMat3& mTransform);

  virtual void HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const WGameModeMsgToEditor* pMsg);
  void HandleObjectStateFromEngineMsg(const WPushObjectStateMsgToEditor* pMsg);

  void SendObjectMsg(const WDocumentObject* pObj, WObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const WDocumentObject* pObj, WObjectTagMsgToEngine* pMsg);

  /// \name Scene Settings
  ///@{

  virtual const WDocumentObject* GetSettingsObject() const;
  const WSceneDocumentSettingsBase* GetSettingsBase() const;
  template <typename T>
  const T* GetSettings() const
  {
    return WDynamicCast<const T*>(GetSettingsBase());
  }

  WStatus CreateExposedProperty(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index, WExposedSceneProperty& out_key) const;
  WStatus AddExposedParameter(const char* szName, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index);
  WInt32 FindExposedParameter(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index);
  WStatus RemoveExposedParameter(WInt32 iIndex);
  ///@}

  /// \name Editor Camera
  ///@{

  /// Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavoriteCamera(WUInt8 uiSlot);

  /// Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavoriteCamera(WUInt8 uiSlot);

  /// Searches for an WCameraComponent with the 'EditorShortcut' property set to \a uiSlot and moves the editor camera to that position.
  WResult JumpToLevelCamera(WUInt8 uiSlot, bool bImmediate);

  /// Creates an object with an WCameraComponent at the current editor camera position and sets the 'EditorShortcut' property to \a uiSlot.
  WResult CreateLevelCamera(WUInt8 uiSlot);

  virtual WManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return WManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

  ///@}

  bool CanUndoSelection() const;
  virtual void UndoSelection();

protected:
  void SetGameMode(GameMode::Enum mode);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab) override;
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;

  template <typename Func>
  void ApplyRecursive(const WDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

protected:
  void EnsureSettingsObjectExist();
  void DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const WEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const WToolsProjectEvent& e);

  WStatus RequestExportScene(const char* szTargetFile, const WAssetFileHeader& header);

  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(WDocumentObject* pObject);

  /// Sends the child order sync message if pObj has a component with WSyncChildOrderAttribute.
  void SyncChildOrderForObject(const WDocumentObject* pObj);

  /// Flushes m_PendingChildOrderSync, sending sync messages for all collected parents.
  void SendPendingChildOrderSyncs();

  /// Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in
  /// the runtime.
  void UpdateObjectDebugTargets();

  DocumentType m_DocumentType = DocumentType::Scene;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  WDocumentObjectMirror m_ObjectMirror;
  WRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
protected:
  bool m_bStoreSelectionChange = true;
  WInt8 m_iAllowSelectionChanges = -1;
  WCopyOnBroadcastEvent<const WSelectionManagerEvent&>::Unsubscriber m_SelectionHandlerUnsubscriber;
  WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&>::Unsubscriber m_ChildOrderStructureEventUnsubscriber;
  WEvent<const WCommandHistoryEvent&, WMutex>::Unsubscriber m_ChildOrderCommandHistoryUnsubscriber;
  void SelectionManagerEventHandler(const WSelectionManagerEvent& e);
  void ChildOrderStructureEventHandler(const WDocumentObjectStructureEvent& e);
  void ChildOrderCommandHistoryEventHandler(const WCommandHistoryEvent& e);

  struct SelectionHistory
  {
    WDynamicArray<WUuid> m_Objects;
    WUuid m_documentGuid;
  };

  WDeque<SelectionHistory> m_SelectionStack;

  //////////////////////////////////////////////////////////////////////////
private:
  WSet<WUuid> m_PendingChildOrderSync;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(WReflectedClass* pMessage, WDocument* pSender) override;
  void GatherObjectsOfType(WDocumentObject* pRoot, WGatherObjectsOfTypeMsgInterDoc* pMsg) const;
};
