#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocument;
struct WDocumentObjectStructureEvent;


/// Event describing changes to the selection in the selection manager.
struct WSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
    ChangedRuntimeOverrideSelection, ///< Broadcast by SetRuntimeOverrideSelection().
  };

  Type m_Type;
  const WDocument* m_pDocument;
  const WDocumentObject* m_pObject;
};

struct WSelectionEntry
{
  const WDocumentObject* m_pObject;
  WUInt32 m_uiSelectionOrder = 0; // the index at which this item was in the selection
};

/// Selection Manager stores a set of selected document objects.
class W_TOOLSFOUNDATION_DLL WSelectionManager
{
public:
  /// Event that is broadcast when the selection changes.
  WCopyOnBroadcastEvent<const WSelectionManagerEvent&> m_Events;

  /// Storage for the selection so it can be swapped when using multiple sub documents.
  class Storage : public WRefCounted
  {
  public:
    WDeque<const WDocumentObject*> m_SelectionList;
    WSet<WUuid> m_SelectionSet;
    const WDocumentObjectManager* m_pObjectManager = nullptr;
    WCopyOnBroadcastEvent<const WSelectionManagerEvent&> m_Events;
  };

public:
  WSelectionManager(const WDocumentObjectManager* pObjectManager);
  ~WSelectionManager();

  void Clear();
  void AddObject(const WDocumentObject* pObject);
  void RemoveObject(const WDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const WDocumentObject* pSingleObject);
  void SetSelection(const WDeque<const WDocumentObject*>& selection);
  void ToggleObject(const WDocumentObject* pObject);

  /// Forces all UI that is bound to the selection to rebuild, without actually changing which objects are selected.
  ///
  /// Clears and immediately reapplies the current selection, which triggers the same events as SetSelection().
  /// Use this when external state that affects how the selection is displayed (e.g. available tags) has changed,
  /// but the set of selected objects has not, so a plain SetSelection() call would be a no-op.
  void RefreshSelection();

  /// Sets a separate selection (temporarily), which is sent to the engine but not propagated to the editor.
  ///
  /// This is used for cases where temporarily the engine should use a different selection than the editor.
  /// Currently this is used during drag-and-drop, to already show the dragged object as selected and especially to exclude it from picking,
  /// but not yet show the new object as selected in the property grids, such that users can interact with the previously selected object.
  ///
  /// To clear a runtime override selection, simply set an empty selection.
  void SetRuntimeOverrideSelection(const WDeque<const WDocumentObject*>& selection);

  /// Returns the current runtime override selection.
  ///
  /// Valid, if the selection is non-empty.
  /// See SetRuntimeOverrideSelection() for details.
  const WDeque<const WDocumentObject*>& GetRuntimeOverrideSelection() const { return m_RuntimeOverrideSelection; }

  /// Returns the last selected object in the selection or null if empty.
  const WDocumentObject* GetCurrentObject() const;

  /// Returns the selection in the same order the objects were added to the list.
  const WDeque<const WDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }



  /// Returns the subset of selected items which have no parent selected.
  ///
  /// I.e. if an object is selected and one of its ancestors is selected, it is culled from the list.
  /// Items are returned in the order of appearance in an expanded scene tree.
  /// Their order in the selection is returned through WSelectionEntry.
  void GetTopLevelSelection(WDynamicArray<WSelectionEntry>& out_entries) const;

  /// Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  void GetTopLevelSelectionOfType(const WRTTI* pBase, WDynamicArray<WSelectionEntry>& out_entries) const;

  bool IsSelected(const WDocumentObject* pObject) const;
  bool IsParentSelected(const WDocumentObject* pObject) const;

  const WDocument* GetDocument() const;

  WSharedPtr<WSelectionManager::Storage> SwapStorage(WSharedPtr<WSelectionManager::Storage> pNewStorage);
  WSharedPtr<WSelectionManager::Storage> GetStorage() { return m_pSelectionStorage; }

private:
  void TreeEventHandler(const WDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const WDocumentObject* pObject);

  friend class WDocument;

  WSharedPtr<WSelectionManager::Storage> m_pSelectionStorage;
  WDeque<const WDocumentObject*> m_RuntimeOverrideSelection;

  WCopyOnBroadcastEvent<const WDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  WCopyOnBroadcastEvent<const WSelectionManagerEvent&>::Unsubscriber m_EventsUnsubscriber;
};
