#pragma once

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class WScene2Document;

class WSceneLayerBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSceneLayerBase, WReflectedClass);

public:
  WSceneLayerBase();
  ~WSceneLayerBase();

public:
  mutable WScene2Document* m_pDocument = nullptr;
};

class WSceneLayer : public WSceneLayerBase
{
  W_ADD_DYNAMIC_REFLECTION(WSceneLayer, WSceneLayerBase);

public:
  WSceneLayer();
  ~WSceneLayer();

public:
  WUuid m_Layer;
};

class WSceneDocumentSettings : public WSceneDocumentSettingsBase
{
  W_ADD_DYNAMIC_REFLECTION(WSceneDocumentSettings, WSceneDocumentSettingsBase);

public:
  WSceneDocumentSettings();
  ~WSceneDocumentSettings();

public:
  WDynamicArray<WSceneLayerBase*> m_Layers;
  mutable WScene2Document* m_pDocument = nullptr;
};

struct WScene2LayerEvent
{
  enum class Type
  {
    LayerAdded,
    LayerRemoved,
    LayerLoaded,
    LayerUnloaded,
    LayerVisible,
    LayerInvisible,
    ActiveLayerChanged,
    SettingsChanged,
  };

  Type m_Type;
  WUuid m_layerGuid;
};

class W_EDITORPLUGINSCENE_DLL WScene2Document : public WSceneDocument
{
  W_ADD_DYNAMIC_REFLECTION(WScene2Document, WSceneDocument);

public:
  WScene2Document(WStringView sDocumentPath);
  ~WScene2Document();

  /// \name Scene Data Accessors
  ///@{

  const WDocumentObjectManager* GetSceneObjectManager() const { return m_pSceneObjectManager.Borrow(); }
  WDocumentObjectManager* GetSceneObjectManager() { return m_pSceneObjectManager.Borrow(); }
  WSelectionManager* GetSceneSelectionManager() const { return m_pSceneSelectionManager.Borrow(); }
  WCommandHistory* GetSceneCommandHistory() const { return m_pSceneCommandHistory.Borrow(); }
  WObjectAccessorBase* GetSceneObjectAccessor() const { return m_pSceneObjectAccessor.Borrow(); }
  const WObjectMetaData<WUuid, WDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() const { return m_pSceneDocumentObjectMetaData.Borrow(); }
  WObjectMetaData<WUuid, WDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() { return m_pSceneDocumentObjectMetaData.Borrow(); }
  const WObjectMetaData<WUuid, WGameObjectMetaData>* GetSceneGameObjectMetaData() const { return m_pSceneGameObjectMetaData.Borrow(); }
  WObjectMetaData<WUuid, WGameObjectMetaData>* GetSceneGameObjectMetaData() { return m_pSceneGameObjectMetaData.Borrow(); }

  ///@}
  /// \name Layer Functions
  ///@{

  WSelectionManager* GetLayerSelectionManager() const { return m_pLayerSelection.Borrow(); }

  WStatus CreateLayer(const char* szName, WUuid& out_layerGuid);
  WStatus DeleteLayer(const WUuid& layerGuid);

  const WUuid& GetActiveLayer() const;
  WStatus SetActiveLayer(const WUuid& layerGuid);

  bool IsLayerLoaded(const WUuid& layerGuid) const;
  WStatus SetLayerLoaded(const WUuid& layerGuid, bool bLoaded);
  void GetAllLayers(WDynamicArray<WUuid>& out_layerGuids);
  void GetLoadedLayers(WDynamicArray<WSceneDocument*>& out_layers) const;

  bool IsLayerVisible(const WUuid& layerGuid) const;
  WStatus SetLayerVisible(const WUuid& layerGuid, bool bVisible);

  const WDocumentObject* GetLayerObject(const WUuid& layerGuid) const;
  WSceneDocument* GetLayerDocument(const WUuid& layerGuid) const;

  virtual WGameObjectDocument* GetRedirectedGameObjectDoc() override;

  bool IsAnyLayerModified() const;

  bool GetSwitchLayerToSelection() const { return m_bSwitchLayerToSelection; }
  void SetSwitchLayerToSelection(bool bEnable);

  ///@}
  /// \name Base Class Functions
  ///@{

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;
  virtual const WDocumentObject* GetSettingsObject() const override;
  virtual void HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg) override;
  virtual void SyncAllChildOrders() override;
  virtual WTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;
  virtual void SendGameWorldToEngine() override;
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& assetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;

  ///@}
  /// \name Selection Specific Functions
  ///@{

  void PreventDoubleSelectionChange(bool b);
  virtual void UndoSelection() override;

  ///@}


public:
  mutable WEvent<const WScene2LayerEvent&> m_LayerEvents;

private:
  void LayerSelectionEventHandler(const WSelectionManagerEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);
  void HandleObjectStateFromEngineMsg2(const WPushObjectStateMsgToEditor* pMsg);

  void UpdateLayers();
  void SendLayerVisibility();
  void LayerAdded(const WUuid& layerGuid, const WUuid& layerObjectGuid);
  void LayerRemoved(const WUuid& layerGuid);

private:
  friend class WSceneLayer;
  WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventSubscriber;
  WCopyOnBroadcastEvent<const WSelectionManagerEvent&>::Unsubscriber m_LayerSelectionEventSubscriber;
  WEvent<const WCommandHistoryEvent&, WMutex>::Unsubscriber m_CommandHistoryEventSubscriber;
  WCopyOnBroadcastEvent<const WDocumentManager::Event&>::Unsubscriber m_DocumentManagerEventSubscriber;

  // This is used for a flattened list of the WSceneDocumentSettings hierarchy
  struct LayerInfo
  {
    WSceneDocument* m_pLayer = nullptr;
    WUuid m_objectGuid;
    bool m_bVisible = true;
  };

  // Scene document cache
  WUniquePtr<WDocumentObjectManager> m_pSceneObjectManager;
  mutable WUniquePtr<WCommandHistory> m_pSceneCommandHistory;
  mutable WUniquePtr<WSelectionManager> m_pSceneSelectionManager;
  mutable WUniquePtr<WObjectCommandAccessor> m_pSceneObjectAccessor;
  WUniquePtr<WObjectMetaData<WUuid, WDocumentObjectMetaData>> m_pSceneDocumentObjectMetaData;
  WUniquePtr<WObjectMetaData<WUuid, WGameObjectMetaData>> m_pSceneGameObjectMetaData;

  // Layer state
  mutable WUniquePtr<WSelectionManager> m_pLayerSelection;
  WUuid m_ActiveLayerGuid;
  WHashTable<WUuid, LayerInfo> m_Layers;
  bool m_bSwitchLayerToSelection = true;

  void ActiveLayerGameObjectEventHandler(const WGameObjectEvent& e);

  WEvent<const WGameObjectEvent&>::Unsubscriber m_ActiveLayerGoEvUnsubscriber;
};
