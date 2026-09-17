#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/SharedPtr.h>

/// \internal Represents the state of a group of tasks that can be waited on
class WTaskGroup
{
  W_DISALLOW_COPY_AND_ASSIGN(WTaskGroup);

public:
  WTaskGroup();
  ~WTaskGroup();

private:
  friend class WTaskSystem;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  static void DebugCheckTaskGroup(WTaskGroupID groupID, WMutex& mutex);
#else
  W_ALWAYS_INLINE static void DebugCheckTaskGroup(WTaskGroupID groupID, WMutex& mutex)
  {
    W_IGNORE_UNUSED(groupID);
    W_IGNORE_UNUSED(mutex);
  }
#endif

  /// Puts the calling thread to sleep until this group is fully finished.
  void WaitForFinish(WTaskGroupID group) const;
  void Reuse(WTaskPriority::Enum priority, WOnTaskGroupFinishedCallback callback);

  bool m_bInUse = true;
  bool m_bStartedByUser = false;
  WUInt16 m_uiTaskGroupIndex = 0xFFFF; // only there as a debugging aid
  WUInt32 m_uiGroupCounter = 1;
  WHybridArray<WSharedPtr<WTask>, 16> m_Tasks;
  WHybridArray<WTaskGroupID, 4> m_DependsOnGroups;
  WHybridArray<WTaskGroupID, 8> m_OthersDependingOnMe;
  WAtomicInteger32 m_iNumActiveDependencies;
  WAtomicInteger32 m_iNumRemainingTasks;
  WOnTaskGroupFinishedCallback m_OnFinishedCallback;
  WTaskPriority::Enum m_Priority = WTaskPriority::ThisFrame;
  mutable WConditionVariable m_CondVarGroupFinished;
};
