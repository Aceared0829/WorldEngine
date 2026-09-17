#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

template <typename HandleType>
class WEditorGuidEngineHandleMap
{
public:
  void Clear()
  {
    m_GuidToHandle.Clear();
    m_HandleToGuid.Clear();
  }

  void RegisterObject(WUuid guid, HandleType handle)
  {
    auto it = m_GuidToHandle.Find(guid);
    if (it.IsValid())
    {
      // During undo/redo we may register the same object again. In that case, just use the new version.
      UnregisterObject(guid);
    }
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;

    W_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(WUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    W_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(HandleType handle)
  {
    const WUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    W_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  HandleType GetHandle(WUuid guid) const
  {
    HandleType res = HandleType();
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  WUuid GetGuid(HandleType handle) const { return m_HandleToGuid.GetValueOrDefault(handle, WUuid()); }

  const WMap<HandleType, WUuid>& GetHandleToGuidMap() const { return m_HandleToGuid; }

private:
  WHashTable<WUuid, HandleType> m_GuidToHandle;
  WMap<HandleType, WUuid> m_HandleToGuid;
};
