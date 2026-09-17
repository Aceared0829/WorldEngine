#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WObjectAccessorBase;
class WObjectCommandAccessor;
class WEditorInputContext;
class WAbstractObjectNode;

struct W_TOOLSFOUNDATION_DLL WObjectAccessorChangeEvent
{
  WDocument* m_pDocument = nullptr; ///< The document in which the accessor change occurred.
  WObjectAccessorBase* m_pOldObjectAccessor = nullptr;
  WObjectAccessorBase* m_pNewObjectAccessor = nullptr;
};

/// Stores meta data for document objects, such as prefab information and visibility in the editor.
class W_TOOLSFOUNDATION_DLL WDocumentObjectMetaData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentObjectMetaData, WReflectedClass);

public:
  enum ModifiedFlags : unsigned int
  {
    HiddenFlag = W_BIT(0),
    PrefabFlag = W_BIT(1),
    ActiveParentFlag = W_BIT(2), ///< This flag is used to update an entry, even though there is no meta data for it.

    AllFlags = 0xFFFFFFFF
  };

  bool m_bHidden = false;    ///< Whether the object should be rendered in the editor view (no effect on the runtime)
  WUuid m_CreateFromPrefab; ///< The asset GUID of the prefab from which this object was created. Invalid GUID, if this is not a prefab instance.
  WUuid m_PrefabSeedGuid;   ///< The seed GUID used to remap the object GUIDs from the prefab asset into this instance.
  WString m_sBasePrefab;    ///< The prefab from which this instance was created as complete DDL text (this describes the entire object!). Necessary for
                             ///< three-way-merging the prefab instances.
};

/// Strategies for searching for manipulators in the document.
enum class WManipulatorSearchStrategy
{
  None,                    ///< No manipulator search.
  SelectedObject,          ///< Search for manipulators on the selected document object.
  ChildrenOfSelectedObject ///< Search for manipulators on the children of the selected document object (e.g. on components of game objects).
};

/// Base class for all editable documents in the editor. Handles state, object management, undo/redo, and more.
class W_TOOLSFOUNDATION_DLL WDocument : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocument, WReflectedClass);

public:
  WDocument(WStringView sPath, WDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~WDocument();

  /// \name Document State Functions
  ///@{

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }

  /// Returns when the document was last marked as modified. Invalid if the document is not modified.
  WTime GetModifiedTime() const { return m_ModifiedTime; }

  const WUuid GetGuid() const { return m_pDocumentInfo ? m_pDocumentInfo->m_DocumentID : WUuid(); }

  const WDocumentObjectManager* GetObjectManager() const { return m_pObjectManager.Borrow(); }
  WDocumentObjectManager* GetObjectManager() { return m_pObjectManager.Borrow(); }
  WSelectionManager* GetSelectionManager() const { return m_pSelectionManager.Borrow(); }
  WCommandHistory* GetCommandHistory() const { return m_pCommandHistory.Borrow(); }
  virtual WObjectAccessorBase* GetObjectAccessor() const;

  ///@}
  /// \name Main / Sub-Document Functions
  ///@{

  /// Returns whether this document is a main document, i.e. self contained.
  bool IsMainDocument() const { return m_pHostDocument == this; }
  /// Returns whether this document is a sub-document, i.e. is part of another document.
  bool IsSubDocument() const { return m_pHostDocument != this; }
  /// In case this is a sub-document, returns the main document this belongs to. Otherwise 'this' is returned.
  const WDocument* GetMainDocument() const { return m_pHostDocument; }
  /// At any given time, only the active sub-document can be edited. This returns the active sub-document which can also be this document itself. Changes to the active sub-document are generally triggered by WDocumentObjectStructureEvent::Type::AfterReset.
  const WDocument* GetActiveSubDocument() const { return m_pActiveSubDocument; }
  WDocument* GetMainDocument() { return m_pHostDocument; }
  WDocument* GetActiveSubDocument() { return m_pActiveSubDocument; }

protected:
  WDocument* m_pHostDocument = nullptr;      ///< Pointer to the main document if this is a sub-document, otherwise self.
  WDocument* m_pActiveSubDocument = nullptr; ///< Pointer to the currently active sub-document.

  ///@}
  /// \name Document Management Functions
  ///@{

public:
  /// Returns the absolute path to the document.
  WStringView GetDocumentPath() const { return m_sDocumentPath; }

  /// Saves the document, if it is modified.
  /// If bForce is true, the document will be written, even if it is not considered modified.
  WStatus SaveDocument(bool bForce = false);
  /// Callback type for asynchronous save operations.
  using AfterSaveCallback = WDelegate<void(WDocument*, WStatus)>;
  /// Saves the document asynchronously. Calls the callback when done.
  WTaskGroupID SaveDocumentAsync(AfterSaveCallback callback, bool bForce = false);
  /// Updates the document path after a rename operation.
  void DocumentRenamed(WStringView sNewDocumentPath);

  /// Reads a document from disk and parses its header, objects, and types.
  static WStatus ReadDocument(WStringView sDocumentPath, WUniquePtr<WAbstractObjectGraph>& ref_pHeader, WUniquePtr<WAbstractObjectGraph>& ref_pObjects,
    WUniquePtr<WAbstractObjectGraph>& ref_pTypes);
  /// Reads and registers types from the given object graph.
  static WStatus ReadAndRegisterTypes(const WAbstractObjectGraph& types);

  /// Loads the document from disk.
  WStatus LoadDocument() { return InternalLoadDocument(); }

  /// Brings the corresponding window to the front.
  void EnsureVisible();

  /// Returns the document manager that owns this document.
  WDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const WDocumentTypeDescriptor* GetDocumentTypeDescriptor() const { return m_pTypeDescriptor; }

  /// Returns the document's type name. Same as GetDocumentTypeDescriptor()->m_sDocumentTypeName.
  WStringView GetDocumentTypeName() const
  {
    if (m_pTypeDescriptor == nullptr)
    {
      // if this is a document without a type descriptor, use the RTTI type name as a fallback
      return GetDynamicRTTI()->GetTypeName();
    }

    return m_pTypeDescriptor->m_sDocumentTypeName;
  }

  const WDocumentInfo* GetDocumentInfo() const { return m_pDocumentInfo; }

  /// Asks the document whether a restart of the engine process is allowed at this time.
  ///
  /// Documents that are currently interacting with the engine process (active play-the-game mode) should return false.
  /// All others should return true.
  /// As long as any document returns false, automatic engine process reload is suppressed.
  virtual bool CanEngineProcessBeRestarted() const { return true; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  /// Information about a pasted object, including its parent and index.
  struct PasteInfo
  {
    W_DECLARE_POD_TYPE();

    WDocumentObject* m_pObject = nullptr; ///< The object being pasted.
    WDocumentObject* m_pParent = nullptr; ///< The parent object to paste into.
    WInt32 m_Index = -1;                  ///< The index at which to insert the object.
  };

  /// Whether this document supports pasting the given mime format into it
  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const {}
  /// Creates the abstract graph of data to be copied and returns the mime type for the clipboard to identify the data
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_sMimeType) const { return false; };
  /// Pastes objects from the given object graph into the document.
  virtual bool Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
  {
    return false;
  };

  ///@}
  /// \name Inter Document Communication
  ///@{

  /// This will deliver the message to all open documents. The documents may respond, e.g. by modifying the content of the message.
  void BroadcastInterDocumentMessage(WReflectedClass* pMessage, WDocument* pSender);

  /// Called on all documents when BroadcastInterDocumentMessage() is called.
  ///
  /// Use the RTTI information to identify whether the message is of interest.
  virtual void OnInterDocumentMessage(WReflectedClass* pMessage, WDocument* pSender) {}

  ///@}
  /// \name Editing Functionality
  ///@{

  /// Allows to return a single input context that currently overrides all others (in priority).
  ///
  /// Used to implement custom tools that need to have priority over selection and camera movement.
  virtual WEditorInputContext* GetEditorInputContextOverride() { return nullptr; }

  ///@}
  /// \name Misc Functions
  ///@{

  /// Deletes all currently selected objects in the document.
  virtual void DeleteSelectedObjects() const;

  /// Returns the set of unknown object types encountered during loading.
  const WSet<WString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  /// Returns the number of unknown object type instances encountered during loading.
  WUInt32 GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// Returns errors accumulated during loading (e.g. unresolvable connections).
  ///
  /// Non-empty after loading means the document is in a broken state. Transforms should fail,
  /// and the user should be informed when opening the document.
  const WDynamicArray<WString>& GetLoadingErrors() const { return m_LoadingErrors; }

  /// If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const WFormatString& msg) const;

  /// Tries to compute the position and rotation for an object in the document. Returns W_SUCCESS if it was possible.
  virtual WResult ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const;

  /// Needed by WManipulatorManager to know where to look for the manipulator attributes.
  ///
  /// Override this function for document types that use manipulators.
  /// The WManipulatorManager will assert that the document type doesn't return 'None' once it is in use.
  virtual WManipulatorSearchStrategy GetManipulatorSearchStrategy() const { return WManipulatorSearchStrategy::None; }

  ///@}
  /// \name Prefab Functions
  ///@{

  /// Whether the document allows to create prefabs in it. This may not be allowed for prefab documents themselves, to prevent nested prefabs.
  virtual bool ArePrefabsAllowed() const { return true; }

  /// Updates ALL prefabs in the document with the latest changes. Merges the current prefab templates with the instances in the document.
  virtual void UpdatePrefabs();

  /// Resets the given objects to their template prefab state, if they have local modifications.
  void RevertPrefabs(WArrayPtr<const WDocumentObject*> selection);

  /// Removes the link between a prefab instance and its template, turning the instance into a regular object.
  virtual void UnlinkPrefabs(WArrayPtr<const WDocumentObject*> selection);

  /// Creates a prefab document from the current selection.
  virtual WStatus CreatePrefabDocumentFromSelection(WStringView sFile, const WRTTI* pRootType, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB = {}, WDelegate<void(WDocumentObject*)> adjustNewNodesCB = {}, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});
  /// Creates a prefab document from the given root objects.
  virtual WStatus CreatePrefabDocument(WStringView sFile, WArrayPtr<const WDocumentObject*> rootObjects, const WUuid& invPrefabSeed, WUuid& out_newDocumentGuid, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB = {}, bool bKeepOpen = false, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});

  /// Replaces the given object by a prefab instance. Returns new guid of replaced object.
  virtual WUuid ReplaceByPrefab(const WDocumentObject* pRootObject, WStringView sPrefabFile, const WUuid& prefabAsset, const WUuid& prefabSeed, bool bEnginePrefab);
  /// Reverts the given object to its prefab state. Returns new guid of reverted object.
  virtual WUuid RevertPrefab(const WDocumentObject* pObject);

  ///@}

public:
  /// Meta data for all document objects.
  WUniquePtr<WObjectMetaData<WUuid, WDocumentObjectMetaData>> m_DocumentObjectMetaData;

  /// Event for document-specific notifications.
  mutable WEvent<const WDocumentEvent&> m_EventsOne;
  /// Static event for notifications across all documents.
  static WEvent<const WDocumentEvent&> s_EventsAny;

  /// Event for object accessor change notifications.
  mutable WEvent<const WObjectAccessorChangeEvent&> m_ObjectAccessorChangeEvents;

  /// Adds an error message to the list of loading errors.
  ///
  /// This can be used during loading to accumulate errors, e.g. unresolvable connections, missing prefabs, etc.
  void AddLoadingError(WStringView sError);

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  /// Internal save implementation. Returns a task group ID for async save.
  virtual WTaskGroupID InternalSaveDocument(AfterSaveCallback callback);
  /// Internal load implementation. Loads the document from disk.
  virtual WStatus InternalLoadDocument();
  /// Creates the document info structure. Must be implemented by derived classes.
  virtual WDocumentInfo* CreateDocumentInfo() = 0;

  /// Hook to execute additional code after successfully saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) {}
  virtual void InitializeAfterLoadingAndSaving() {}

  virtual void BeforeClosing();

  void SetUnknownObjectTypes(const WSet<WString>& Types, WUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  /// Recursively updates all prefab instances starting from the given object.
  virtual void UpdatePrefabsRecursive(WDocumentObject* pObject);
  virtual void UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab);

  ///@}

  WUniquePtr<WDocumentObjectManager> m_pObjectManager;
  mutable WUniquePtr<WCommandHistory> m_pCommandHistory;
  mutable WUniquePtr<WSelectionManager> m_pSelectionManager;
  mutable WUniquePtr<WObjectCommandAccessor> m_pObjectAccessor; ///< Default object accessor used by every doc.

  WDocumentInfo* m_pDocumentInfo = nullptr;
  const WDocumentTypeDescriptor* m_pTypeDescriptor = nullptr;

private:
  friend class WDocumentManager;
  friend class WCommandHistory;
  friend class WSaveDocumentTask;
  friend class WAfterSaveDocumentTask;

  void SetupDocumentInfo(const WDocumentTypeDescriptor* pTypeDescriptor);

  /// Cleans up the given path and stores it as the document path.
  ///
  /// All document paths go through here, so that string comparisons against them are reliable.
  void SetDocumentPath(WStringView sPath);

  /// The document manager that owns this document.
  WDocumentManager* m_pDocumentManager = nullptr;

  /// The absolute path to the document file.
  WString m_sDocumentPath;
  bool m_bModified = true;
  bool m_bReadOnly = false;
  bool m_bWindowRequested = false;
  bool m_bAddToRecentFilesList = true;
  WTime m_ModifiedTime;

  /// Set of unknown object types encountered during loading.
  WSet<WString> m_UnknownObjectTypes;
  /// Number of unknown object type instances encountered during loading.
  WUInt32 m_uiUnknownObjectTypeInstances = 0;
  /// Errors accumulated during loading, e.g. unresolvable connections.
  WDynamicArray<WString> m_LoadingErrors;

  WTaskGroupID m_ActiveSaveTask;
  WStatus m_LastSaveResult = W_SUCCESS;
};
