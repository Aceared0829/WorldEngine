#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct WManipulatorManagerEvent;
class WDocument;

/// Singleton that owns and manages WManipulatorAdapter instances for all open documents.
///
/// Listens to WManipulatorManager::m_Events. When the active manipulator changes, it tears
/// down all existing adapters for that document and creates new ones via m_Factory, one per
/// object in the selection. m_Factory maps the RTTI of an WManipulatorAttribute subclass to
/// the corresponding WManipulatorAdapter subclass. Adapters must be registered there by the
/// code that introduces them.
///
/// Adapters are heap-allocated and owned by this registry. They are deleted synchronously
/// in ClearAdapters, which means an adapter must not call into code that reaches ClearAdapters
/// on the same call stack (e.g. changing the document selection from within a gizmo event).
class W_EDITORFRAMEWORK_DLL WManipulatorAdapterRegistry
{
  W_DECLARE_SINGLETON(WManipulatorAdapterRegistry);

public:
  WManipulatorAdapterRegistry();
  ~WManipulatorAdapterRegistry();

  /// Maps WManipulatorAttribute RTTI types to their corresponding WManipulatorAdapter factory functions.
  /// Register new adapter types here to make them available to the system.
  WRttiMappedObjectFactory<WManipulatorAdapter> m_Factory;

  /// Collects grid settings from all active adapters for the given document.
  void QueryGridSettings(const WDocument* pDocument, WGridSettingsMsgToEngine& out_gridSettings);

private:
  void ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e);
  void ClearAdapters(const WDocument* pDocument);

  struct Data
  {
    WHybridArray<WManipulatorAdapter*, 8> m_Adapters;
  };

  WMap<const WDocument*, Data> m_DocumentAdapters;
};
