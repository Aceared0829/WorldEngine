#pragma once

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/GuidHandleMap.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

/// The world rtti converter context tracks created objects and is capable of also handling
///  components / game objects. Used by the WIPCObjectMirror to create / destroy objects.
///
/// Atm it does not remove owner ptr when a parent is deleted, so it will accumulate zombie entries.
/// As requests to dead objects shouldn't generally happen this is for the time being not a problem.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WWorldRttiConverterContext : public WRttiConverterContext
{
public:
  virtual void Clear() override;
  void DeleteExistingObjects();

  virtual WInternal::NewInstance<void> CreateObject(const WUuid& guid, const WRTTI* pRtti) override;
  virtual void DeleteObject(const WUuid& guid) override;

  virtual void RegisterObject(const WUuid& guid, const WRTTI* pRtti, void* pObject) override;
  virtual void UnregisterObject(const WUuid& guid) override;

  virtual WRttiConverterObject GetObjectByGUID(const WUuid& guid) const override;
  virtual WUuid GetObjectGUID(const WRTTI* pRtti, const void* pObject) const override;

  virtual void OnUnknownTypeError(WStringView sTypeName) override;

  WWorld* m_pWorld = nullptr;
  WEditorGuidEngineHandleMap<WGameObjectHandle> m_GameObjectMap;
  WEditorGuidEngineHandleMap<WComponentHandle> m_ComponentMap;

  WEditorGuidEngineHandleMap<WUInt32> m_OtherPickingMap;
  WEditorGuidEngineHandleMap<WUInt32> m_ComponentPickingMap;
  WUInt32 m_uiNextComponentPickingID = 1;
  WUInt32 m_uiHighlightID = 1;

  struct Event
  {
    enum class Type
    {
      GameObjectCreated,
      GameObjectDeleted,
    };

    Type m_Type;
    WUuid m_ObjectGuid;
  };

  WEvent<const Event&> m_Events;

  WSet<WString> m_UnknownTypes;
};
