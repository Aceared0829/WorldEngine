
///
/// Implements WProcessGroup by using WProcess
///

#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/ProcessGroup.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES)

struct WProcessGroupImpl
{
  W_DECLARE_POD_TYPE();
};

WProcessGroup::WProcessGroup(WStringView sGroupName)
{
}

WProcessGroup::~WProcessGroup()
{
  TerminateAll().IgnoreResult();
}

WResult WProcessGroup::Launch(const WProcessOptions& opt)
{
  WProcess& process = m_Processes.ExpandAndGetRef();
  return process.Launch(opt);
}

WResult WProcessGroup::WaitToFinish(WTime timeout /*= WTime::MakeZero()*/)
{
  for (auto& process : m_Processes)
  {
    if (process.GetState() != WProcessState::Finished && process.WaitToFinish(timeout).Failed())
    {
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

WResult WProcessGroup::TerminateAll(WInt32 iForcedExitCode /*= -2*/)
{
  auto result = W_SUCCESS;
  for (auto& process : m_Processes)
  {
    if (process.GetState() == WProcessState::Running && process.Terminate().Failed())
    {
      result = W_FAILURE;
    }
  }

  return result;
}

#endif
