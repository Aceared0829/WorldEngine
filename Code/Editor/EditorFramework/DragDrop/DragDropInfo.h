#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class QMimeData;
class QDataStream;
class WDocumentObject;
class WQtDocumentTreeModelAdapter;

/// This type is used to provide WDragDropHandler instances with all the important information for a drag & drop target
///
/// It is a reflected class such that one can derive and extend it, if necessary.
/// DragDrop handlers can then inspect whether it is a known extended type and cast to the type to get access to additional information.
class W_EDITORFRAMEWORK_DLL WDragDropInfo : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDragDropInfo, WReflectedClass);

public:
  WDragDropInfo();

  const QMimeData* m_pMimeData;

  /// A string identifying into what context the object is dropped, e.g. "viewport" or "scenetree" etc.
  WString m_sTargetContext;

  /// The WDocument GUID
  WUuid m_TargetDocument;

  /// GUID of the WDocumentObject that is at the dropped position. May be invalid. Can be used to attach as a child, to modify the object itself or
  /// can be ignored.
  WUuid m_TargetObject;

  /// GUID of the WDocumentObject that may be used as the parent, if no other target is more important.
  WUuid m_ActiveParentObject;

  /// GUID of the WDocumentObject that is the more specific component (of m_TargetObject) that was dragged on. May be invalid.
  WUuid m_TargetComponent;

  /// World space position where the object is dropped. May be NaN.
  WVec3 m_vDropPosition;

  /// World space normal at the point where the object is dropped. May be NaN.
  WVec3 m_vDropNormal;

  /// Some kind of index / ID for the object that is at the drop location. For meshes this is the material index.
  WInt32 m_iTargetObjectSubID;

  /// If dropped on a scene tree, this may say as which child the object is supposed to be inserted. -1 if invalid (ie. append)
  WInt32 m_iTargetObjectInsertChildIndex;

  /// If dropped on a scene tree, this is the adapter for the target object.
  const WQtDocumentTreeModelAdapter* m_pAdapter = nullptr;

  bool m_bShiftKeyDown;
  bool m_bCtrlKeyDown;
};


/// After an WDragDropHandler has been chosen to handle an operation, it is queried once to fill out an instance of this type (or an extended
/// derived type) to enable configuring how WDragDropInfo is computed by the target.
class W_EDITORFRAMEWORK_DLL WDragDropConfig : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDragDropConfig, WReflectedClass);

public:
  WDragDropConfig();

  /// Whether the currently selected objects (ie the dragged objects) should be considered for picking or not. Default is disabled.
  bool m_bPickSelectedObjects;
};
