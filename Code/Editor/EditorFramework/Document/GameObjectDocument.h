#pragma once
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class WAssetFileHeader;
class WGameObjectEditTool;
class WGameObjectDocument;

struct W_EDITORFRAMEWORK_DLL TransformationChanges
{
  enum Enum
  {
    Translation = W_BIT(0),
    Rotation = W_BIT(1),
    Scale = W_BIT(2),
    All = 0xFF
  };
};

struct W_EDITORFRAMEWORK_DLL WGameObjectEvent
{
  enum class Type
  {
    RenderSelectionOverlayChanged,
    RenderVisualizersChanged,
    RenderShapeIconsChanged,
    AddAmbientLightChanged,
    SimulationSpeedChanged,
    PickTransparentChanged,

    ActiveEditToolChanged,

    TriggerExpandScenegraph,
    TriggerShowSelectionInScenegraph,
    TriggerFocusOnSelection_Hovered,
    TriggerFocusOnSelection_All,

    TriggerSnapSelectionPivotToGrid,
    TriggerSnapEachSelectedObjectToGrid,

    GameModeChanged,

    GizmoTransformMayBeInvalid, ///< Sent when a change was made that may affect the current gizmo / manipulator state (ie. objects have been moved)

    TriggerSetScenegraphFilter, ///< Sets the scenegraph search filter text. The filter string is stored in m_sPayload.
  };

  Type m_Type;
  WString m_sPayload;
};

struct WGameObjectDocumentEvent
{
  enum class Type
  {
    GameMode_Stopped,
    GameMode_StartingSimulate,
    GameMode_StartingPlay,
    GameMode_StartingExternal, ///< ie. scene is exported for WPlayer
  };

  Type m_Type;
  WGameObjectDocument* m_pDocument = nullptr;
};

class W_EDITORFRAMEWORK_DLL WGameObjectMetaData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectMetaData, WReflectedClass);

public:
  enum ModifiedFlags
  {
    CachedName = W_BIT(2),
    AllFlags = 0xFFFFFFFF
  };

  WGameObjectMetaData() = default;

  WString m_CachedNodeName;
  QIcon m_Icon;
};

struct W_EDITORFRAMEWORK_DLL WSelectedGameObject
{
  const WDocumentObject* m_pObject;
  WVec3 m_vLocalScaling;
  float m_fLocalUniformScaling;
  WTransform m_GlobalTransform;
};

class W_EDITORFRAMEWORK_DLL WGameObjectDocument : public WAssetDocument
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectDocument, WAssetDocument);

public:
  WGameObjectDocument(WStringView sDocumentPath, WDocumentObjectManager* pObjectManager, WAssetDocEngineConnection engineConnectionType = WAssetDocEngineConnection::FullObjectMirroring);
  ~WGameObjectDocument();

  /// In case a document consists of multiple layers, this redirection is necessary to execute actions on the active layer.
  virtual WGameObjectDocument* GetRedirectedGameObjectDoc() { return this; }

  virtual WEditorInputContext* GetEditorInputContextOverride() override;

protected:
  void SubscribeGameObjectEventHandlers();
  void UnsubscribeGameObjectEventHandlers();

  void GameObjectDocumentEventHandler(const WGameObjectDocumentEvent& e);

  /// \name Gizmo
  ///@{
public:
  /// Makes an edit tool of the given type active. Allocates a new one, if necessary. Only works when SetEditToolConfigDelegate() is set.
  void SetActiveEditTool(const WRTTI* pEditToolType);

  /// Returns the currently active edit tool (nullptr for none).
  WGameObjectEditTool* GetActiveEditTool() const { return m_pActiveEditTool; }

  /// Checks whether an edit tool of the given type, or nullptr for none, is active.
  bool IsActiveEditTool(const WRTTI* pEditToolType) const;

  /// Needs to be called by some higher level code (usually the DocumentWindow) to react to newly created edit tools to configure them (call
  /// WGameObjectEditTool::ConfigureTool()).
  void SetEditToolConfigDelegate(WDelegate<void(WGameObjectEditTool*)> configDelegate);

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  void SetGizmoMoveParentOnly(bool bMoveParent);
  bool GetGizmoMoveParentOnly() const;

  /// Finds all objects that are selected at the top level, ie. none of their parents is selected.
  ///
  /// Additionally stores the current transformation. Useful to store this at the start of an operation
  /// to then do modifications on this base transformation every frame.
  void ComputeTopLevelSelectedGameObjects(WDeque<WSelectedGameObject>& out_selection);

  virtual void HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg) override;

  /// Finds all usages of the given asset in this document and appends them to out_usages. The default implementation does nothing, override this if your document can reference other assets.
  virtual void FindAssetUsages(WStringView sAssetToFind, WDynamicArray<AssetUsage>& out_usages, WUInt32 uiMaxResults) const override;

private:
  void FindAssetUsagesInternal(WStringView sAssetToFind, const WDocumentObject* pObject, WDynamicArray<AssetUsage>& out_usages, WUInt32 uiMaxResults) const;
  void DeallocateEditTools();

  WDelegate<void(WGameObjectEditTool*)> m_EditToolConfigDelegate;
  WGameObjectEditTool* m_pActiveEditTool = nullptr;
  WMap<const WRTTI*, WGameObjectEditTool*> m_CreatedEditTools;


  ///@}
  /// \name Actions
  ///@{

public:
  void TriggerExpandScenegraph() const;
  void TriggerShowSelectionInScenegraph() const;
  void TriggerFocusOnSelection(bool bAllViews) const;
  void TriggerSnapPivotToGrid() const;
  void TriggerSnapEachObjectToGrid() const;
  /// Moves the editor camera to the same position as the selected object
  void SnapCameraToObject();
  /// Moves the camera to the current picking position
  void MoveCameraHere();

  void ScheduleSendObjectSelection();

  /// Sends the current object selection, but only if it was modified or specifically tagged for resending with ScheduleSendObjectSelection().
  void SendObjectSelection();

  ///@}
  /// \name Settings
  ///@{
public:
  bool GetAddAmbientLight() const { return m_bAddAmbientLight; }
  void SetAddAmbientLight(bool b);

  float GetSimulationSpeed() const { return m_fSimulationSpeed; }
  void SetSimulationSpeed(float f);

  bool GetPauseSimulation() const { return m_bPauseSimulation; }
  void SetPauseSimulation(bool b);

  bool GetRenderSelectionOverlay() const { return m_CurrentMode.m_bRenderSelectionOverlay; }
  void SetRenderSelectionOverlay(bool b);

  bool GetRenderVisualizers() const { return m_CurrentMode.m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  bool GetRenderShapeIcons() const { return m_CurrentMode.m_bRenderShapeIcons; }
  void SetRenderShapeIcons(bool b);

  bool GetPickTransparent() const { return m_bPickTransparent; }
  void SetPickTransparent(bool b);

  void SetStepSimulation(bool b) { m_bStepSimulation = b; }
  bool GetStepSimulation() const { return m_bStepSimulation; }

  /// Specifies which object is the 'active parent', which is the object under which newly created objects should be parented.
  void SetActiveParent(WUuid object);
  /// Returns the object under which newly created objects should be parented.
  ///
  /// \note The object may not exist anymore! So check with the ObjectManager first.
  WUuid GetActiveParent() const { return m_ActiveParent; }

private:
  WUuid m_ActiveParent = WUuid::MakeInvalid();

  ///@}
  /// \name Transform
  ///@{

public:
  /// Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus
  /// some work can be spared.
  void SetGlobalTransform(const WDocumentObject* pObject, const WTransform& t, WUInt8 uiTransformationChanges) const;

  /// Same as SetGlobalTransform, except that all children will keep their current global transform (thus their local transforms are adjusted)
  void SetGlobalTransformParentOnly(const WDocumentObject* pObject, const WTransform& t, WUInt8 uiTransformationChanges) const;

  /// Returns a cached value for the global transform of the given object, if available. Otherwise it calls ComputeGlobalTransform().
  WTransform GetGlobalTransform(const WDocumentObject* pObject) const;

  /// Retrieves the local transform property values from the object and combines it into one WTransform
  static WTransform QueryLocalTransform(const WDocumentObject* pObject);
  static WSimdTransform QueryLocalTransformSimd(const WDocumentObject* pObject);

  /// Computes the global transform of the parent and combines it with the local transform of the given object.
  /// This function does not return a cached value, but always computes it. It does update the internal cache for later reads though.
  WTransform ComputeGlobalTransform(const WDocumentObject* pObject) const;

  /// Traverses the pObject hierarchy up until it hits an WGameObject, then computes the global transform of that.
  virtual WResult ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const override;

  ///@}
  /// \name Node Names
  ///@{

  /// Generates a good name for pObject. Queries the "Name" property, child components and asset properties, if necessary.
  void DetermineNodeName(const WDocumentObject* pObject, const WUuid& prefabGuid, WStringBuilder& out_sResult, QIcon* out_pIcon = nullptr) const;

  /// Similar to DetermineNodeName() but prefers to return the last cached value from scene meta data. This is more efficient, but may give an
  /// outdated result.
  void QueryCachedNodeName(
    const WDocumentObject* pObject, WStringBuilder& out_sResult, WUuid* out_pPrefabGuid = nullptr, QIcon* out_pIcon = nullptr) const;

  /// Creates a full "path" to a scene object for display in UIs. No guarantee for uniqueness.
  void GenerateFullDisplayName(const WDocumentObject* pRoot, WStringBuilder& out_sFullPath) const;

  ///@}

public:
  mutable WEvent<const WGameObjectEvent&> m_GameObjectEvents;
  mutable WUniquePtr<WObjectMetaData<WUuid, WGameObjectMetaData>> m_GameObjectMetaData;

  static WEvent<const WGameObjectDocumentEvent&> s_GameObjectDocumentEvents;

protected:
  void InvalidateGlobalTransformValue(const WDocumentObject* pObject) const;
  /// Sends the current state of the scene to the engine process. This is typically done after scene load or when the world might have deviated
  /// on the engine side (after play the game etc.)
  virtual void SendGameWorldToEngine();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;

public:
  void SelectionManagerEventHandler(const WSelectionManagerEvent& e);
  void ObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const WDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const WDocumentObjectEvent& e);

protected:
  struct W_EDITORFRAMEWORK_DLL GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };
  GameModeData m_CurrentMode;

private:
  bool m_bAddAmbientLight = false;
  bool m_bGizmoWorldSpace = true; // whether the gizmo is in local/global space mode
  bool m_bGizmoMoveParentOnly = false;
  bool m_bPickTransparent = true;

  bool m_bPauseSimulation = false;
  bool m_bStepSimulation = false;
  float m_fSimulationSpeed = 1.0f;

  using TransformTable = WHashTable<const WDocumentObject*, WSimdTransform, WHashHelper<const WDocumentObject*>, WAlignedAllocatorWrapper>;
  mutable TransformTable m_GlobalTransforms;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  WInt8 m_iResendSelection = 0;

protected:
  WEventSubscriptionID m_SelectionManagerEventHandlerID;
  WEventSubscriptionID m_ObjectPropertyEventHandlerID;
  WEventSubscriptionID m_ObjectStructureEventHandlerID;
  WEventSubscriptionID m_ObjectEventHandlerID;
};
