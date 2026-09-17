#pragma once

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

template <typename ElemType>
class ArrayPtrTask final : public WTask
{
public:
  ArrayPtrTask(WArrayPtr<ElemType> payload, WParallelForFunction<ElemType> taskCallback, WUInt32 uiItemsPerInvocation)
    : m_Payload(payload)
    , m_uiItemsPerInvocation(uiItemsPerInvocation)
    , m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(0, m_Payload);
  }

  void ExecuteWithMultiplicity(WUInt32 uiInvocation) const override
  {
    const WUInt32 uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;

    const WUInt32 uiRemainingItems = uiSliceStartIndex > m_Payload.GetCount() ? 0 : m_Payload.GetCount() - uiSliceStartIndex;
    const WUInt32 uiSliceItemCount = WMath::Min(m_uiItemsPerInvocation, uiRemainingItems);

    if (uiSliceItemCount > 0)
    {
      // Run through the calculated slice.
      auto taskItemSlice = m_Payload.GetSubArray(uiSliceStartIndex, uiSliceItemCount);
      m_TaskCallback(uiSliceStartIndex, taskItemSlice);
    }
  }

private:
  WArrayPtr<ElemType> m_Payload;
  WUInt32 m_uiItemsPerInvocation;
  WParallelForFunction<ElemType> m_TaskCallback;
};

template <typename ElemType>
void WTaskSystem::ParallelForInternal(WArrayPtr<ElemType> taskItems, WParallelForFunction<ElemType> taskCallback, const char* taskName, const WParallelForParams& params)
{
  if (taskItems.GetCount() <= params.m_uiBinSize)
  {
    ArrayPtrTask<ElemType> arrayPtrTask(taskItems, std::move(taskCallback), taskItems.GetCount());
    arrayPtrTask.ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    W_PROFILE_SCOPE(arrayPtrTask.m_sTaskName);
    arrayPtrTask.Execute();
  }
  else
  {
    WUInt32 uiMultiplicity;
    WUInt64 uiItemsPerInvocation;
    params.DetermineThreading(taskItems.GetCount(), uiMultiplicity, uiItemsPerInvocation);

    WAllocator* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : WFoundation::GetDefaultAllocator();

    WSharedPtr<ArrayPtrTask<ElemType>> pArrayPtrTask = W_NEW(pAllocator, ArrayPtrTask<ElemType>, taskItems, std::move(taskCallback), static_cast<WUInt32>(uiItemsPerInvocation));
    pArrayPtrTask->ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    pArrayPtrTask->SetMultiplicity(uiMultiplicity);
    WTaskGroupID taskGroupId = WTaskSystem::StartSingleTask(pArrayPtrTask, WTaskPriority::EarlyThisFrame);
    WTaskSystem::WaitForGroup(taskGroupId);
  }
}

template <typename ElemType, typename Callback>
void WTaskSystem::ParallelFor(WArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const WParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](
                           WUInt32 /*uiBaseIndex*/, WArrayPtr<ElemType> taskSlice)
  { taskCallback(taskSlice); };

  ParallelForInternal<ElemType>(
    taskItems, WParallelForFunction<ElemType>(std::move(wrappedCallback), WFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void WTaskSystem::ParallelForSingle(WArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const WParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](WUInt32 /*uiBaseIndex*/, WArrayPtr<ElemType> taskSlice)
  {
    // Handing in by non-const& allows to use callbacks with (non-)const& as well as value parameters.
    for (ElemType& taskItem : taskSlice)
    {
      taskCallback(taskItem);
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, WParallelForFunction<ElemType>(std::move(wrappedCallback), WFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void WTaskSystem::ParallelForSingleIndex(
  WArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const WParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](WUInt32 uiBaseIndex, WArrayPtr<ElemType> taskSlice)
  {
    for (WUInt32 uiIndex = 0; uiIndex < taskSlice.GetCount(); ++uiIndex)
    {
      // Handing in by dereferenced pointer allows to use callbacks with (non-)const& as well as value parameters.
      taskCallback(uiBaseIndex + uiIndex, *(taskSlice.GetPtr() + uiIndex));
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, WParallelForFunction<ElemType>(std::move(wrappedCallback), WFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}
