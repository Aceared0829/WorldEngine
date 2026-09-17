#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Utilities/DGMLWriter.h>

class WTestTask final : public WTask
{
public:
  WUInt32 m_uiIterations;
  WTestTask* m_pDependency;
  bool m_bSupportCancel;
  WInt32 m_iTaskID;

  WTestTask()
  {
    m_uiIterations = 50;
    m_pDependency = nullptr;
    m_bStarted = false;
    m_bDone = false;
    m_bSupportCancel = false;
    m_iTaskID = -1;

    ConfigureTask("WTestTask", WTaskNesting::Never);
  }

  bool IsStarted() const { return m_bStarted; }
  bool IsDone() const { return m_bDone; }
  bool IsMultiplicityDone() const { return m_iMultiplicityCount == (int)GetMultiplicity(); }

private:
  bool m_bStarted;
  bool m_bDone;
  mutable WAtomicInteger32 m_iMultiplicityCount;

  virtual void ExecuteWithMultiplicity(WUInt32 uiInvocation) const override { m_iMultiplicityCount.Increment(); }

  virtual void Execute() override
  {
    if (m_iTaskID >= 0)
      WLog::Printf("Starting Task %i at %.4f\n", m_iTaskID, WTime::Now().GetSeconds());

    m_bStarted = true;

    W_TEST_BOOL(m_pDependency == nullptr || m_pDependency->IsTaskFinished());

    for (WUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));
      WTime::Now();

      if (HasBeenCanceled() && m_bSupportCancel)
      {
        if (m_iTaskID >= 0)
          WLog::Printf("Canceling Task %i at %.4f\n", m_iTaskID, WTime::Now().GetSeconds());
        return;
      }
    }

    m_bDone = true;

    if (m_iTaskID >= 0)
      WLog::Printf("Finishing Task %i at %.4f\n", m_iTaskID, WTime::Now().GetSeconds());
  }
};

class TaskCallbacks
{
public:
  void TaskFinished(const WSharedPtr<WTask>& pTask) { m_pInt->Increment(); }

  void TaskGroupFinished(WTaskGroupID id) { m_pInt->Increment(); }

  WAtomicInteger32* m_pInt;
};

W_CREATE_SIMPLE_TEST(Threading, TaskSystem)
{
  WInt8 iWorkersShort = 4;
  WInt8 iWorkersLong = 4;

  WTaskSystem::SetWorkerThreadCount(iWorkersShort, iWorkersLong);
  WThreadUtils::Sleep(WTime::MakeFromMilliseconds(500));

  W_TEST_BLOCK(WTestBlock::Enabled, "Single Tasks")
  {
    WSharedPtr<WTestTask> t[3];

    t[0] = W_DEFAULT_NEW(WTestTask);
    t[1] = W_DEFAULT_NEW(WTestTask);
    t[2] = W_DEFAULT_NEW(WTestTask);

    t[0]->ConfigureTask("Task 0", WTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", WTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", WTaskNesting::Never);

    auto tg0 = WTaskSystem::StartSingleTask(t[0], WTaskPriority::LateThisFrame);
    auto tg1 = WTaskSystem::StartSingleTask(t[1], WTaskPriority::ThisFrame);
    auto tg2 = WTaskSystem::StartSingleTask(t[2], WTaskPriority::EarlyThisFrame);

    WTaskSystem::WaitForGroup(tg0);
    WTaskSystem::WaitForGroup(tg1);
    WTaskSystem::WaitForGroup(tg2);

    W_TEST_BOOL(t[0]->IsDone());
    W_TEST_BOOL(t[1]->IsDone());
    W_TEST_BOOL(t[2]->IsDone());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Single Tasks with Dependencies")
  {
    WSharedPtr<WTestTask> t[4];

    t[0] = W_DEFAULT_NEW(WTestTask);
    t[1] = W_DEFAULT_NEW(WTestTask);
    t[2] = W_DEFAULT_NEW(WTestTask);
    t[3] = W_DEFAULT_NEW(WTestTask);

    WTaskGroupID g[4];

    t[0]->ConfigureTask("Task 0", WTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", WTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", WTaskNesting::Never);
    t[3]->ConfigureTask("Task 3", WTaskNesting::Maybe);

    g[0] = WTaskSystem::StartSingleTask(t[0], WTaskPriority::LateThisFrame);
    g[1] = WTaskSystem::StartSingleTask(t[1], WTaskPriority::ThisFrame, g[0]);
    g[2] = WTaskSystem::StartSingleTask(t[2], WTaskPriority::EarlyThisFrame, g[1]);
    g[3] = WTaskSystem::StartSingleTask(t[3], WTaskPriority::EarlyThisFrame, g[0]);

    WTaskSystem::WaitForGroup(g[2]);
    WTaskSystem::WaitForGroup(g[3]);

    W_TEST_BOOL(t[0]->IsDone());
    W_TEST_BOOL(t[1]->IsDone());
    W_TEST_BOOL(t[2]->IsDone());
    W_TEST_BOOL(t[3]->IsDone());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Grouped Tasks / TaskFinished Callback / GroupFinished Callback")
  {
    WSharedPtr<WTestTask> t[8];

    WTaskGroupID g[4];
    WAtomicInteger32 GroupsFinished;
    WAtomicInteger32 TasksFinished;

    TaskCallbacks callbackGroup;
    callbackGroup.m_pInt = &GroupsFinished;

    TaskCallbacks callbackTask;
    callbackTask.m_pInt = &TasksFinished;

    g[0] = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame, WMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[1] = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame, WMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[2] = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame, WMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[3] = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame, WMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));

    for (int i = 0; i < 4; ++i)
      W_TEST_BOOL(!WTaskSystem::IsTaskGroupFinished(g[i]));

    WTaskSystem::AddTaskGroupDependency(g[1], g[0]);
    WTaskSystem::AddTaskGroupDependency(g[2], g[0]);
    WTaskSystem::AddTaskGroupDependency(g[3], g[1]);

    for (int i = 0; i < 8; ++i)
    {
      t[i] = W_DEFAULT_NEW(WTestTask);
      t[i]->ConfigureTask("Test Task", WTaskNesting::Maybe, WMakeDelegate(&TaskCallbacks::TaskFinished, &callbackTask));
    }

    WTaskSystem::AddTaskToGroup(g[0], t[0]);
    WTaskSystem::AddTaskToGroup(g[1], t[1]);
    WTaskSystem::AddTaskToGroup(g[1], t[2]);
    WTaskSystem::AddTaskToGroup(g[2], t[3]);
    WTaskSystem::AddTaskToGroup(g[2], t[4]);
    WTaskSystem::AddTaskToGroup(g[2], t[5]);
    WTaskSystem::AddTaskToGroup(g[3], t[6]);
    WTaskSystem::AddTaskToGroup(g[3], t[7]);

    for (int i = 0; i < 8; ++i)
    {
      W_TEST_BOOL(!t[i]->IsTaskFinished());
      W_TEST_BOOL(!t[i]->IsDone());
    }

    // do a snapshot
    // we don't validate it, just make sure it doesn't crash
    WDGMLGraph graph;
    WTaskSystem::WriteStateSnapshotToDGML(graph);

    WTaskSystem::StartTaskGroup(g[3]);
    WTaskSystem::StartTaskGroup(g[2]);
    WTaskSystem::StartTaskGroup(g[1]);
    WTaskSystem::StartTaskGroup(g[0]);

    WTaskSystem::WaitForGroup(g[3]);
    WTaskSystem::WaitForGroup(g[2]);
    WTaskSystem::WaitForGroup(g[1]);
    WTaskSystem::WaitForGroup(g[0]);

    W_TEST_INT(TasksFinished, 8);

    // It is not guaranteed that group finished callback is called after WaitForGroup returned so we need to wait a bit here.
    for (int i = 0; i < 10; i++)
    {
      if (GroupsFinished == 4)
      {
        break;
      }
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));
    }
    W_TEST_INT(GroupsFinished, 4);

    for (int i = 0; i < 4; ++i)
      W_TEST_BOOL(WTaskSystem::IsTaskGroupFinished(g[i]));

    for (int i = 0; i < 8; ++i)
    {
      W_TEST_BOOL(t[i]->IsTaskFinished());
      W_TEST_BOOL(t[i]->IsDone());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "This Frame Tasks / Next Frame Tasks")
  {
    const WUInt32 uiNumTasks = 20;
    WSharedPtr<WTestTask> t[uiNumTasks];
    WTaskGroupID tg[uiNumTasks];
    bool finished[uiNumTasks];

    for (WUInt32 i = 0; i < uiNumTasks; i += 2)
    {
      finished[i] = false;
      finished[i + 1] = false;

      t[i] = W_DEFAULT_NEW(WTestTask);
      t[i + 1] = W_DEFAULT_NEW(WTestTask);

      t[i]->m_uiIterations = 10;
      t[i + 1]->m_uiIterations = 20;

      tg[i] = WTaskSystem::StartSingleTask(t[i], WTaskPriority::ThisFrame);
      tg[i + 1] = WTaskSystem::StartSingleTask(t[i + 1], WTaskPriority::NextFrame);
    }

    // 'finish' the first frame
    WTaskSystem::FinishFrameTasks();

    {
      WUInt32 uiNotAllThisTasksFinished = 0;
      WUInt32 uiNotAllNextTasksFinished = 0;

      for (WUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // up to the number of worker threads tasks can still be active
      W_TEST_BOOL(uiNotAllThisTasksFinished <= WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::ShortTasks));
      W_TEST_BOOL(uiNotAllNextTasksFinished <= uiNumTasks);
    }


    // 'finish' the second frame
    WTaskSystem::FinishFrameTasks();

    {
      WUInt32 uiNotAllThisTasksFinished = 0;
      WUInt32 uiNotAllNextTasksFinished = 0;

      for (int i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      W_TEST_BOOL(
        uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::ShortTasks));
    }

    // 'finish' all frames
    WTaskSystem::FinishFrameTasks();

    {
      WUInt32 uiNotAllThisTasksFinished = 0;
      WUInt32 uiNotAllNextTasksFinished = 0;

      for (WUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          W_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // even after finishing multiple frames, the previous frame tasks may still be in execution
      // since no N+x tasks enforce their completion in this test
      W_TEST_BOOL(
        uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::ShortTasks));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Main Thread Tasks")
  {
    const WUInt32 uiNumTasks = 20;
    WSharedPtr<WTestTask> t[uiNumTasks];

    for (WUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t[i] = W_DEFAULT_NEW(WTestTask);
      t[i]->m_uiIterations = 10;

      WTaskSystem::StartSingleTask(t[i], WTaskPriority::ThisFrameMainThread);
    }

    WTaskSystem::FinishFrameTasks();

    for (WUInt32 i = 0; i < uiNumTasks; ++i)
    {
      W_TEST_BOOL(t[i]->IsTaskFinished());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Canceling Tasks")
  {
    const WUInt32 uiNumTasks = 20;
    WSharedPtr<WTestTask> t[uiNumTasks];
    WTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i] = W_DEFAULT_NEW(WTestTask);
      t[i]->m_uiIterations = 50;

      tg[i] = WTaskSystem::StartSingleTask(t[i], WTaskPriority::ThisFrame);
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

    WUInt32 uiCanceled = 0;

    for (WUInt32 i0 = uiNumTasks; i0 > 0; --i0)
    {
      const WUInt32 i = i0 - 1;

      if (WTaskSystem::CancelTask(t[i], WOnTaskRunning::ReturnWithoutBlocking) == W_SUCCESS)
        ++uiCanceled;
    }

    WUInt32 uiDone = 0;
    WUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      WTaskSystem::WaitForGroup(tg[i]);
      W_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // at least one task should have run and thus be 'done'
    W_TEST_BOOL(uiDone > 0);
    W_TEST_BOOL(uiDone < uiNumTasks);

    W_TEST_BOOL(uiStarted > 0);
    W_TEST_BOOL_MSG(uiStarted <= WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::ShortTasks),
      "This test can fail when the PC is under heavy load."); // should not have managed to start more tasks than there are threads
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Canceling Tasks (forcefully)")
  {
    const WUInt32 uiNumTasks = 20;
    WSharedPtr<WTestTask> t[uiNumTasks];
    WTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i] = W_DEFAULT_NEW(WTestTask);
      t[i]->m_uiIterations = 50;
      t[i]->m_bSupportCancel = true;

      tg[i] = WTaskSystem::StartSingleTask(t[i], WTaskPriority::ThisFrame);
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

    WUInt32 uiCanceled = 0;

    for (int i = uiNumTasks - 1; i >= 0; --i)
    {
      if (WTaskSystem::CancelTask(t[i], WOnTaskRunning::ReturnWithoutBlocking) == W_SUCCESS)
        ++uiCanceled;
    }

    WUInt32 uiDone = 0;
    WUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      WTaskSystem::WaitForGroup(tg[i]);
      W_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // not a single thread should have finished the execution
    if (W_TEST_BOOL_MSG(uiDone == 0, "This test can fail when the PC is under heavy load."))
    {
      W_TEST_BOOL(uiStarted > 0);
      W_TEST_BOOL(uiStarted <= WTaskSystem::GetNumAllocatedWorkerThreads(
                                  WWorkerThreadType::ShortTasks)); // should not have managed to start more tasks than there are threads
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Canceling Group")
  {
    const WUInt32 uiNumTasks = 4;
    WSharedPtr<WTestTask> t1[uiNumTasks];
    WSharedPtr<WTestTask> t2[uiNumTasks];

    WTaskGroupID g1, g2;
    g1 = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame);
    g2 = WTaskSystem::CreateTaskGroup(WTaskPriority::ThisFrame);

    WTaskSystem::AddTaskGroupDependency(g2, g1);

    for (WUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t1[i] = W_DEFAULT_NEW(WTestTask);
      t2[i] = W_DEFAULT_NEW(WTestTask);

      WTaskSystem::AddTaskToGroup(g1, t1[i]);
      WTaskSystem::AddTaskToGroup(g2, t2[i]);
    }

    WTaskSystem::StartTaskGroup(g2);
    WTaskSystem::StartTaskGroup(g1);

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(10));

    W_TEST_BOOL(WTaskSystem::CancelGroup(g2, WOnTaskRunning::WaitTillFinished) == W_SUCCESS);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      W_TEST_BOOL(!t2[i]->IsDone());
      W_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));

    W_TEST_BOOL(WTaskSystem::CancelGroup(g1, WOnTaskRunning::WaitTillFinished) == W_FAILURE);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      W_TEST_BOOL(!t2[i]->IsDone());

      W_TEST_BOOL(t1[i]->IsTaskFinished());
      W_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Tasks with Multiplicity")
  {
    WSharedPtr<WTestTask> t[3];
    WTaskGroupID tg[3];

    t[0] = W_DEFAULT_NEW(WTestTask);
    t[1] = W_DEFAULT_NEW(WTestTask);
    t[2] = W_DEFAULT_NEW(WTestTask);

    t[0]->ConfigureTask("Task 0", WTaskNesting::Maybe);
    t[1]->ConfigureTask("Task 1", WTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", WTaskNesting::Never);

    t[0]->SetMultiplicity(1);
    t[1]->SetMultiplicity(100);
    t[2]->SetMultiplicity(1000);

    tg[0] = WTaskSystem::StartSingleTask(t[0], WTaskPriority::LateThisFrame);
    tg[1] = WTaskSystem::StartSingleTask(t[1], WTaskPriority::ThisFrame);
    tg[2] = WTaskSystem::StartSingleTask(t[2], WTaskPriority::EarlyThisFrame);

    WTaskSystem::WaitForGroup(tg[0]);
    WTaskSystem::WaitForGroup(tg[1]);
    WTaskSystem::WaitForGroup(tg[2]);

    W_TEST_BOOL(t[0]->IsMultiplicityDone());
    W_TEST_BOOL(t[1]->IsMultiplicityDone());
    W_TEST_BOOL(t[2]->IsMultiplicityDone());
  }

  // capture profiling info for testing
  /*WStringBuilder sOutputPath = WTestFramework::GetInstance()->GetAbsOutputPath();

  WFileSystem::AddDataDirectory(sOutputPath.GetData());

  WFileWriter fileWriter;
  if (fileWriter.Open("profiling.json") == W_SUCCESS)
  {
  WProfilingSystem::Capture(fileWriter);
  }*/
}
