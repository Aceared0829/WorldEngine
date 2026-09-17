#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>

WTask::WTask() = default;
WTask::~WTask() = default;

void WTask::Reset()
{
  m_iRemainingRuns = (int)WMath::Max(1u, m_uiMultiplicity);
  m_bCancelExecution = false;
  m_bTaskIsScheduled = false;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void WTask::ConfigureTask(const char* szTaskName, WTaskNesting nestingMode, WOnTaskFinishedCallback callback /*= WOnTaskFinishedCallback()*/)
{
  W_ASSERT_DEV(IsTaskFinished(), "This function must be called before the task is started.");

  m_sTaskName = szTaskName;
  m_NestingMode = nestingMode;
  m_OnTaskFinished = callback;
}

void WTask::SetMultiplicity(WUInt32 uiMultiplicity)
{
  m_uiMultiplicity = uiMultiplicity;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void WTask::Run(WUInt32 uiInvocation)
{
  // actually this should not be possible to happen
  if (m_iRemainingRuns == 0 || m_bCancelExecution)
  {
    m_iRemainingRuns = 0;
    return;
  }

  {
    WStringBuilder scopeName("Task: ", m_sTaskName);

    if (m_bUsesMultiplicity)
      scopeName.AppendFormat("-{}", uiInvocation);

    W_PROFILE_SCOPE(scopeName.GetData());

    if (m_bUsesMultiplicity)
    {
      ExecuteWithMultiplicity(uiInvocation);
    }
    else
    {
      Execute();
    }
  }

  m_iRemainingRuns.Decrement();
}
