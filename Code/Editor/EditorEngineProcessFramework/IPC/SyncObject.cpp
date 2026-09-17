#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineSyncObject, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SyncGuid", m_SyncObjectGuid),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEditorEngineSyncObject::WEditorEngineSyncObject()
{
  m_SyncObjectGuid = WUuid::MakeUuid();
  m_bModified = true;
}

WEditorEngineSyncObject::~WEditorEngineSyncObject()
{
  if (m_OnDestruction.IsValid())
  {
    m_OnDestruction(this);
  }
}

void WEditorEngineSyncObject::Configure(WUuid ownerGuid, WDelegate<void(WEditorEngineSyncObject*)> onDestruction)
{
  m_OwnerGuid = ownerGuid;
  m_OnDestruction = onDestruction;
}

WUuid WEditorEngineSyncObject::GetDocumentGuid() const
{
  return m_OwnerGuid;
}
