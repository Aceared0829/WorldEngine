#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/TaskSystem.h>

namespace
{
  static constexpr WUInt32 s_uiNumberOfWorkers = 4;
  static constexpr WUInt32 s_uiTaskItemSliceSize = 25;
  static constexpr WUInt32 s_uiTotalNumberOfTaskItems = s_uiNumberOfWorkers * s_uiTaskItemSliceSize;
} // namespace

W_CREATE_SIMPLE_TEST(Threading, ParallelFor)
{
  // set up controlled task system environment
  WTaskSystem::SetWorkerThreadCount(::s_uiNumberOfWorkers, ::s_uiNumberOfWorkers);

  // shared variables
  WMutex dataAccessMutex;

  WUInt32 uiRangesEncounteredCheck = 0;
  WUInt32 uiNumbersSum = 0;

  WUInt32 uiNumbersCheckSum = 0;
  WStaticArray<WUInt32, ::s_uiTotalNumberOfTaskItems> numbers;

  WParallelForParams parallelForParams;
  parallelForParams.m_uiBinSize = ::s_uiTaskItemSliceSize;
  parallelForParams.m_uiMaxTasksPerThread = 1;

  auto ResetSharedVariables = [&uiRangesEncounteredCheck, &uiNumbersSum, &uiNumbersCheckSum, &numbers]()
  {
    uiRangesEncounteredCheck = 0;
    uiNumbersSum = 0;

    uiNumbersCheckSum = 0;

    numbers.EnsureCount(::s_uiTotalNumberOfTaskItems);
    for (WUInt32 i = 0; i < ::s_uiTotalNumberOfTaskItems; ++i)
    {
      numbers[i] = i + 1;
      uiNumbersCheckSum += numbers[i];
    }
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Indexed)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of number assigned to us via index ranges and
    // check if the ranges described by them are as expected
    WTaskSystem::ParallelForIndexed(
      0, ::s_uiTotalNumberOfTaskItems,
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &numbers](WUInt32 uiStartIndex, WUInt32 uiEndIndex)
      {
        W_LOCK(dataAccessMutex);

        // size check
        W_TEST_INT(uiEndIndex - uiStartIndex, ::s_uiTaskItemSliceSize);

        // note down which range this is
        uiRangesEncounteredCheck |= 1 << (uiStartIndex / ::s_uiTaskItemSliceSize);

        // sum up numbers in our slice
        for (WUInt32 uiIndex = uiStartIndex; uiIndex < uiEndIndex; ++uiIndex)
        {
          uiNumbersSum += numbers[uiIndex];
        }
      },
      "ParallelForIndexed Test", WTaskNesting::Never, parallelForParams);

    // check results
    W_TEST_INT(uiRangesEncounteredCheck, 0b1111);
    W_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Array)")
  {
    // reset
    ResetSharedVariables();

    // test-specific data
    WStaticArray<WUInt32*, ::s_uiNumberOfWorkers> startAddresses;
    for (WUInt32 i = 0; i < ::s_uiNumberOfWorkers; ++i)
    {
      startAddresses.PushBack(numbers.GetArrayPtr().GetPtr() + (i * ::s_uiTaskItemSliceSize));
    }

    // test
    // sum up the slice of numbers assigned to us via array pointers and
    // check if the ranges described by them are as expected
    WTaskSystem::ParallelFor<WUInt32>(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &startAddresses](WArrayPtr<WUInt32> taskItemSlice)
      {
        W_LOCK(dataAccessMutex);

        // size check
        W_TEST_INT(taskItemSlice.GetCount(), ::s_uiTaskItemSliceSize);

        // note down which range this is
        for (WUInt32 index = 0; index < startAddresses.GetCount(); ++index)
        {
          if (startAddresses[index] == taskItemSlice.GetPtr())
          {
            uiRangesEncounteredCheck |= 1 << index;
          }
        }

        // sum up numbers in our slice
        for (const WUInt32& number : taskItemSlice)
        {
          uiNumbersSum += number;
        }
      },
      "ParallelFor Array Test", parallelForParams);

    // check results
    W_TEST_INT(15, 0b1111);
    W_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Array, Single)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers by summing up the individual numbers that get handed to us
    WTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](WUInt32 uiNumber)
      {
        W_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Test", parallelForParams);

    // check the resulting sum
    W_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Array, Single, Index)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers that got assigned to us via an index range
    WTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](WUInt32 uiIndex, WUInt32 uiNumber)
      {
        W_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber + (uiIndex + 1);
      },
      "ParallelFor Array Single Index Test", parallelForParams);

    // check the resulting sum
    W_TEST_INT(uiNumbersSum, 2 * uiNumbersCheckSum);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Array, Single) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    WTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](WUInt32& ref_uiNumber)
      {
        W_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 3;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    WTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const WUInt32& uiNumber)
      {
        W_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    W_TEST_INT(uiNumbersSum, 3 * uiNumbersCheckSum);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parallel For (Array, Single, Index) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    WTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](WUInt32, WUInt32& ref_uiNumber)
      {
        W_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 4;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    WTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const WUInt32& uiNumber)
      {
        W_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    W_TEST_INT(uiNumbersSum, 4 * uiNumbersCheckSum);
  }
}
