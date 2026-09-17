#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Lock.h>

WTaskGroup::WTaskGroup() = default;
WTaskGroup::~WTaskGroup() = default;

void WTaskGroup::WaitForFinish(WTaskGroupID group) const
{
  if (m_uiGroupCounter != group.m_uiGroupCounter)
    return;

  W_PROFILE_SCOPE("WTaskGroup::WaitForFinish");
  W_LOCK(m_CondVarGroupFinished);

  while (m_uiGroupCounter == group.m_uiGroupCounter)
  {
    m_CondVarGroupFinished.UnlockWaitForSignalAndLock();
  }
}

void WTaskGroup::Reuse(WTaskPriority::Enum priority, WOnTaskGroupFinishedCallback callback)
{
  m_bInUse = true;
  m_bStartedByUser = false;
  m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  m_Tasks.Clear();
  m_DependsOnGroups.Clear();
  m_OthersDependingOnMe.Clear();
  m_Priority = priority;
  m_OnFinishedCallback = callback;
}

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
void WTaskGroup::DebugCheckTaskGroup(WTaskGroupID groupID, WMutex& mutex)
{
  W_LOCK(mutex);

  const WTaskGroup* pGroup = groupID.m_pTaskGroup;
  W_IGNORE_UNUSED(pGroup);

  W_ASSERT_DEV(pGroup != nullptr, "TaskGroupID is invalid.");
  W_ASSERT_DEV(pGroup->m_uiGroupCounter == groupID.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  W_ASSERT_DEV(!pGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  W_ASSERT_DEV(pGroup->m_iNumActiveDependencies == 0, "Invalid active dependenices");
}
#endif
