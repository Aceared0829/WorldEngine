#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/TaskSystem.h>

/// This is a helper class that splits up task items via index ranges.
template <typename IndexType, typename Callback>
class IndexedTask final : public WTask
{
public:
  IndexedTask(IndexType uiStartIndex, IndexType uiNumItems, Callback taskCallback, IndexType uiItemsPerInvocation)
    : m_uiStartIndex(uiStartIndex)
    , m_uiNumItems(uiNumItems)
    , m_uiItemsPerInvocation(uiItemsPerInvocation)
    , m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(m_uiStartIndex, m_uiStartIndex + m_uiNumItems);
  }

  void ExecuteWithMultiplicity(WUInt32 uiInvocation) const override
  {
    const IndexType uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;
    const IndexType uiSliceEndIndex = WMath::Min(uiSliceStartIndex + m_uiItemsPerInvocation, m_uiStartIndex + m_uiNumItems);

    W_ASSERT_DEV(uiSliceStartIndex < uiSliceEndIndex, "ParallelFor start/end indices given to index task are invalid: {} -> {}", uiSliceStartIndex, uiSliceEndIndex);

    // Run through the calculated slice, the end index is exclusive, i.e., should not be handled by this instance.
    m_TaskCallback(uiSliceStartIndex, uiSliceEndIndex);
  }

private:
  IndexType m_uiStartIndex;
  IndexType m_uiNumItems;
  IndexType m_uiItemsPerInvocation;
  Callback m_TaskCallback;
};

template <typename IndexType, typename Callback>
void ParallelForIndexedInternal(IndexType uiStartIndex, IndexType uiNumItems, const Callback&& taskCallback, const char* szTaskName, const WParallelForParams& params, WTaskNesting taskNesting)
{
  typedef IndexedTask<IndexType, Callback> Task;

  if (!szTaskName)
  {
    szTaskName = "Generic Indexed Task";
  }

  if (uiNumItems <= params.m_uiBinSize)
  {
    // If we have not exceeded the threading threshold we use serial execution

    Task indexedTask(uiStartIndex, uiNumItems, std::move(taskCallback), uiNumItems);
    indexedTask.ConfigureTask(szTaskName, taskNesting);

    W_PROFILE_SCOPE(szTaskName);
    indexedTask.Execute();
  }
  else
  {
    WUInt32 uiMultiplicity;
    WUInt64 uiItemsPerInvocation;
    params.DetermineThreading(uiNumItems, uiMultiplicity, uiItemsPerInvocation);

    WAllocator* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : WFoundation::GetDefaultAllocator();

    WSharedPtr<Task> pIndexedTask = W_NEW(pAllocator, Task, uiStartIndex, uiNumItems, std::move(taskCallback), static_cast<IndexType>(uiItemsPerInvocation));
    pIndexedTask->ConfigureTask(szTaskName, taskNesting);

    pIndexedTask->SetMultiplicity(uiMultiplicity);
    WTaskGroupID taskGroupId = WTaskSystem::StartSingleTask(pIndexedTask, WTaskPriority::EarlyThisFrame);
    WTaskSystem::WaitForGroup(taskGroupId);
  }
}

void WParallelForParams::DetermineThreading(WUInt64 uiNumItemsToExecute, WUInt32& out_uiNumTasksToRun, WUInt64& out_uiNumItemsPerTask) const
{
  // we create a single task, but we set it's multiplicity to M (= out_uiNumTasksToRun)
  // so that it gets scheduled M times, which is effectively the same as creating M tasks

  const WUInt32 uiNumWorkerThreads = WTaskSystem::GetWorkerThreadCount(WWorkerThreadType::ShortTasks);
  const WUInt64 uiMaxTasksToUse = uiNumWorkerThreads * m_uiMaxTasksPerThread;
  const WUInt64 uiMaxExecutionsRequired = WMath::Max(1llu, uiNumItemsToExecute / m_uiBinSize);

  if (uiMaxExecutionsRequired >= uiMaxTasksToUse)
  {
    // if we have more items to execute, than the upper limit of tasks that we want to spawn, clamp the number of tasks
    // and give each task more items to do
    out_uiNumTasksToRun = uiMaxTasksToUse & 0xFFFFFFFF;
  }
  else
  {
    // if we want to execute fewer items than we have tasks available, just run exactly as many tasks as we have items
    out_uiNumTasksToRun = uiMaxExecutionsRequired & 0xFFFFFFFF;
  }

  // now that we determined the number of tasks to run, compute how much each task should do
  out_uiNumItemsPerTask = uiNumItemsToExecute / out_uiNumTasksToRun;

  // due to rounding down in the line above, it can happen that we would execute too few tasks
  if (out_uiNumItemsPerTask * out_uiNumTasksToRun < uiNumItemsToExecute)
  {
    // to fix this, either do one more task invocation, or one more item per task
    if (out_uiNumItemsPerTask * (out_uiNumTasksToRun + 1) >= uiNumItemsToExecute)
    {
      ++out_uiNumTasksToRun;

      // though with one more task we may execute too many items, so if possible reduce the number of items that each task executes
      while ((out_uiNumItemsPerTask - 1) * out_uiNumTasksToRun >= uiNumItemsToExecute)
      {
        --out_uiNumItemsPerTask;
      }
    }
    else
    {
      ++out_uiNumItemsPerTask;

      // though if every task executes one more item, we may execute too many items, so if possible reduce the number of tasks again
      while (out_uiNumItemsPerTask * (out_uiNumTasksToRun - 1) >= uiNumItemsToExecute)
      {
        --out_uiNumTasksToRun;
      }
    }

    W_ASSERT_DEV(out_uiNumItemsPerTask * out_uiNumTasksToRun >= uiNumItemsToExecute, "WParallelFor is missing invocations");
  }
}

void WTaskSystem::ParallelForIndexed(WUInt32 uiStartIndex, WUInt32 uiNumItems, WParallelForIndexedFunction32 taskCallback, const char* szTaskName, WTaskNesting taskNesting, const WParallelForParams& params)
{
  ParallelForIndexedInternal<WUInt32, WParallelForIndexedFunction32>(uiStartIndex, uiNumItems, std::move(taskCallback), szTaskName, params, taskNesting);
}

void WTaskSystem::ParallelForIndexed(WUInt64 uiStartIndex, WUInt64 uiNumItems, WParallelForIndexedFunction64 taskCallback, const char* szTaskName, WTaskNesting taskNesting, const WParallelForParams& params)
{
  ParallelForIndexedInternal<WUInt64, WParallelForIndexedFunction64>(uiStartIndex, uiNumItems, std::move(taskCallback), szTaskName, params, taskNesting);
}
