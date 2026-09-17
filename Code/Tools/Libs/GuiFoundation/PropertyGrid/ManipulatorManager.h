#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class WManipulatorAttribute;
struct WPhantomRttiManagerEvent;
struct WSelectionManagerEvent;

/// Broadcast by WManipulatorManager whenever the active manipulator or its selection changes.
///
/// If m_pManipulator is nullptr or m_bHideManipulators is true, any active gizmos should be
/// torn down. Listeners are responsible for updating or recreating their gizmos accordingly.
struct W_GUIFOUNDATION_DLL WManipulatorManagerEvent
{
  const WDocument* m_pDocument = nullptr;
  const WManipulatorAttribute* m_pManipulator = nullptr;
  const WHybridArray<WPropertySelection, 8>* m_pSelection = nullptr;
  bool m_bHideManipulators = false;
};

/// Singleton that tracks which manipulator is active per document.
///
/// A manipulator becomes active when the property grid detects a focused property that carries
/// an WManipulatorAttribute. The manager stores one active attribute and a corresponding
/// object selection per document and broadcasts m_Events whenever the state changes.
///
/// On selection changes the manager automatically re-evaluates which objects in the current
/// selection carry the same attribute and updates the stored selection accordingly. The
/// document type controls whether the manager looks at selected objects directly or at their
/// children via GetManipulatorSearchStrategy().
///
/// Hiding a manipulator (HideActiveManipulator / ToggleHideActiveManipulator) suppresses the
/// gizmo without clearing the active attribute, so it can be restored when un-hidden.
class W_GUIFOUNDATION_DLL WManipulatorManager
{
  W_DECLARE_SINGLETON(WManipulatorManager);

public:
  WManipulatorManager();
  ~WManipulatorManager();

  /// Returns the currently active manipulator attribute and the associated selection for the given document.
  /// Returns nullptr and sets out_pSelection to nullptr when no manipulator is active.
  const WManipulatorAttribute* GetActiveManipulator(const WDocument* pDoc, const WHybridArray<WPropertySelection, 8>*& out_pSelection) const;

  /// Sets the active manipulator for a document and broadcasts the change.
  /// Also resets the hidden flag so the gizmo becomes visible.
  void SetActiveManipulator(const WDocument* pDoc, const WManipulatorAttribute* pManipulator, const WArrayPtr<WPropertySelection>& selection);

  /// Deactivates the manipulator for the given document and broadcasts the change.
  void ClearActiveManipulator(const WDocument* pDoc);

  /// Fired whenever the active manipulator, its selection, or the hidden state changes for any document.
  WCopyOnBroadcastEvent<const WManipulatorManagerEvent&> m_Events;

  /// Sets the hidden flag without deactivating the manipulator. Gizmos are hidden when bHide is true.
  void HideActiveManipulator(const WDocument* pDoc, bool bHide);

  /// Toggles the hidden flag for the active manipulator of the given document.
  void ToggleHideActiveManipulator(const WDocument* pDoc);

  /// Cycles through the manipulators available on the last selected object.
  ///
  /// If no manipulator is active, activates the first available one. If the currently active
  /// manipulator is not the last in the list, activates the next one. If it is the last,
  /// clears the active manipulator.
  void CycleActiveManipulator(const WDocument* pDoc);

private:
  struct Data
  {
    const WManipulatorAttribute* m_pAttribute = nullptr;
    WHybridArray<WPropertySelection, 8> m_Selection;
    bool m_bHideManipulators = false;
  };

  void InternalSetActiveManipulator(const WDocument* pDoc, const WManipulatorAttribute* pManipulator, const WArrayPtr<WPropertySelection>& selection, bool bUnhide);

  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  void TransferToCurrentSelection(const WDocument* pDoc);

  void PhantomTypeManagerEventHandler(const WPhantomRttiManagerEvent& e);
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);

  WMap<const WDocument*, Data> m_ActiveManipulator;
};
