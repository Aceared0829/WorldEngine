#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_LINUX) && W_ENABLED(W_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>

namespace WInternal
{
  bool SetProcessLaunchParentDeathSignal(bool bEnable);
}

struct WProcessGroupImpl
{
  W_DECLARE_POD_TYPE();
};

WProcessGroup::WProcessGroup(WStringView sGroupName)
{
  W_IGNORE_UNUSED(sGroupName);
}

WProcessGroup::~WProcessGroup()
{
  TerminateAll().IgnoreResult();
}

WResult WProcessGroup::Launch(const WProcessOptions& opt)
{
  WProcess& process = m_Processes.ExpandAndGetRef();

  const bool bPreviousValue = WInternal::SetProcessLaunchParentDeathSignal(true);
  WResult result = process.Launch(opt);
  WInternal::SetProcessLaunchParentDeathSignal(bPreviousValue);

  return result;
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
  W_IGNORE_UNUSED(iForcedExitCode);

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
