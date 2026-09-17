#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WWorld;

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineSyncObject : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineSyncObject, WReflectedClass);

public:
  WEditorEngineSyncObject();
  ~WEditorEngineSyncObject();

  void Configure(WUuid ownerGuid, WDelegate<void(WEditorEngineSyncObject*)> onDestruction);

  WUuid GetDocumentGuid() const;
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  WUuid GetGuid() const { return m_SyncObjectGuid; }

  // One-time setup on the engine side.
  // \returns Whether the sync object is pickable via uiNextComponentPickingID.
  virtual bool SetupForEngine(WWorld* pWorld, WUInt32 uiNextComponentPickingID) { return false; }
  virtual void UpdateForEngine(WWorld* pWorld) {}

private:
  W_ALLOW_PRIVATE_PROPERTIES(WEditorEngineSyncObject);

  friend class WAssetDocument;

  bool m_bModified;
  WUuid m_SyncObjectGuid;
  WUuid m_OwnerGuid;

  WDelegate<void(WEditorEngineSyncObject*)> m_OnDestruction;
};
