#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class WManipulatorAttribute;
class WDocumentObject;
struct WDocumentObjectPropertyEvent;
struct WQtDocumentWindowEvent;
class WObjectAccessorBase;
class WGridSettingsMsgToEngine;

/// Abstract base class for in-viewport gizmo adapters driven by an WManipulatorAttribute.
///
/// An adapter bridges one document object and one WManipulatorAttribute to a set of gizmos
/// rendered in the viewport. It is instantiated by WManipulatorAdapterRegistry and initialized
/// via SetManipulator(). Subclasses implement Finalize(), Update(), and UpdateGizmoTransform()
/// to set up and maintain their gizmos. UpdateGizmoTransform() is called every frame before
/// the viewport is redrawn, while Update() is called on property changes and after transactions.
///
/// Adapters are deleted synchronously by WManipulatorAdapterRegistry::ClearAdapters when the
/// active manipulator changes (e.g. due to a selection change). Any code path inside an adapter
/// that changes the selection must therefore defer the change to avoid deleting itself while
/// still on the call stack.
class W_EDITORFRAMEWORK_DLL WManipulatorAdapter
{
public:
  WManipulatorAdapter();
  virtual ~WManipulatorAdapter();

  /// Connects this adapter to the given attribute and document object, then calls Finalize() and Update().
  void SetManipulator(const WManipulatorAttribute* pAttribute, const WDocumentObject* pObject);

  virtual void QueryGridSettings(WGridSettingsMsgToEngine& out_gridSettings) {}

private:
  void DocumentObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const WQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e);

protected:
  virtual WTransform GetOffsetTransform() const;
  virtual WTransform GetObjectTransform() const;
  WObjectAccessorBase* GetObjectAccessor() const;
  const WAbstractProperty* GetProperty(const char* szProperty) const;

  /// Called once after SetManipulator(). Use this for one-time gizmo setup that depends on m_pObject being valid.
  virtual void Finalize() = 0;

  /// Called when a relevant property on m_pObject changes, after a transaction finishes, or on visibility changes.
  virtual void Update() = 0;

  /// Called every frame before the viewport redraws. Update gizmo world-space transforms here.
  virtual void UpdateGizmoTransform() = 0;

  /// Starts a temporary command sequence for live drag interaction. Must be paired with EndTemporaryInteraction() or CancelTemporayInteraction().
  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();

  /// Convenience wrapper that starts a transaction, sets up to six properties on m_pObject, and finishes the transaction.
  void ChangeProperties(const char* szProperty1, WVariant value1, const char* szProperty2 = nullptr, WVariant value2 = WVariant(),
    const char* szProperty3 = nullptr, WVariant value3 = WVariant(), const char* szProperty4 = nullptr, WVariant value4 = WVariant(),
    const char* szProperty5 = nullptr, WVariant value5 = WVariant(), const char* szProperty6 = nullptr, WVariant value6 = WVariant());

  bool m_bManipulatorIsVisible = false;
  const WManipulatorAttribute* m_pManipulatorAttr = nullptr;
  const WDocumentObject* m_pObject = nullptr; ///< The document object this adapter operates on. Valid for the lifetime of the adapter.

  void ClampProperty(const char* szProperty, WVariant& value) const;
};
