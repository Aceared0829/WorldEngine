#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS) && W_ENABLED(W_SUPPORTS_PROCESSES)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/System/ProcessGroup.h>

struct WProcessGroupImpl
{
  HANDLE m_hJobObject = INVALID_HANDLE_VALUE;
  HANDLE m_hCompletionPort = INVALID_HANDLE_VALUE;
  WString m_sName;

  ~WProcessGroupImpl();
  void Close();
  void Initialize();
};

WProcessGroupImpl::~WProcessGroupImpl()
{
  Close();
}

void WProcessGroupImpl::Close()
{
  if (m_hJobObject != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hJobObject);
    m_hJobObject = INVALID_HANDLE_VALUE;
  }
}

void WProcessGroupImpl::Initialize()
{
  if (m_hJobObject == INVALID_HANDLE_VALUE)
  {
    m_hJobObject = CreateJobObjectW(nullptr, nullptr);

    if (m_hJobObject == nullptr || m_hJobObject == INVALID_HANDLE_VALUE)
    {
      WLog::Error("Failed to create process group '{}' - {}", m_sName, WArgErrorCode(GetLastError()));
      return;
    }

    // configure the job object such that it kill all processes once this job object is cleaned up
    // ie. either when all job object handles are closed, or the application crashes

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION exinfo = {};
    exinfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (SetInformationJobObject(m_hJobObject, JobObjectExtendedLimitInformation, &exinfo, sizeof(exinfo)) == FALSE)
    {
      WLog::Error("WProcessGroup: failed to configure 'kill jobs on close' - '{}'", WArgErrorCode(GetLastError()));
    }

    // the completion port is necessary to implement WaitToFinish()
    // see https://devblogs.microsoft.com/oldnewthing/20130405-00/?p=4743
    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
    Port.CompletionKey = m_hJobObject;
    Port.CompletionPort = m_hCompletionPort;
    SetInformationJobObject(m_hJobObject, JobObjectAssociateCompletionPortInformation, &Port, sizeof(Port));
  }
}

WProcessGroup::WProcessGroup(WStringView sGroupName)
{
  m_pImpl = W_DEFAULT_NEW(WProcessGroupImpl);
  m_pImpl->m_sName = sGroupName;
}

WProcessGroup::~WProcessGroup()
{
  TerminateAll().IgnoreResult();
}

WResult WProcessGroup::Launch(const WProcessOptions& opt)
{
  m_pImpl->Initialize();

  WProcess& process = m_Processes.ExpandAndGetRef();
  W_SUCCEED_OR_RETURN(process.Launch(opt, WProcessLaunchFlags::Suspended));

  if (AssignProcessToJobObject(m_pImpl->m_hJobObject, process.GetProcessHandle()) == FALSE)
  {
    WLog::Warning("Failed to add process to process group '{}' - {}. Process will not be tracked by the job object.", m_pImpl->m_sName, WArgErrorCode(GetLastError()));
    // Keep the process in m_Processes so callers can still query its state,
    // but it won't be covered by the job object's kill-on-close guarantee.
  }

  if (process.ResumeSuspended().Failed())
  {
    WLog::Error("Failed to resume the given process. Processes must be launched in a suspended state before adding them to process groups.");
    m_Processes.PopBack();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WProcessGroup::WaitToFinish(WTime timeout /*= WTime::MakeZero()*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
    return W_SUCCESS;

  // check if no new processes were launched, because waiting could end up in an infinite loop,
  // so don't even try in this case
  bool allProcessesGone = true;
  for (const WProcess& p : m_Processes)
  {
    DWORD exitCode = 0;
    GetExitCodeProcess(p.GetProcessHandle(), &exitCode);
    if (exitCode == STILL_ACTIVE)
    {
      allProcessesGone = false;
      break;
    }
  }

  if (allProcessesGone)
  {
    // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
    for (WProcess& p : m_Processes)
    {
      p.WaitToFinish().IgnoreResult();
    }
    m_pImpl->Close();
    return W_SUCCESS;
  }

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  DWORD CompletionCode;
  ULONG_PTR CompletionKey;
  LPOVERLAPPED Overlapped;

  WTime tStart = WTime::Now();

  while (true)
  {
    // ATTENTION !
    // If you are looking at a crash dump of W this line will typically be at the top of the callstack.
    // That is because to write the crash dump an external process is called and this is where we are waiting for that process to finish.
    // To see the actual reason for the crash, locate the call to WCrashHandlerFunc further down in the callstack.
    // The crashing code is usually the one calling that function.

    if (GetQueuedCompletionStatus(m_pImpl->m_hCompletionPort, &CompletionCode, &CompletionKey, &Overlapped, dwTimeout) == FALSE)
    {
      DWORD res = GetLastError();

      if (res != WAIT_TIMEOUT)
      {
        WLog::Error("Failed to wait for process group '{}' - {}", m_pImpl->m_sName, WArgErrorCode(res));
      }

      return W_FAILURE;
    }

    // we got the expected result, all processes have finished
    if (((HANDLE)CompletionKey == m_pImpl->m_hJobObject && CompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
    {
      // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
      for (WProcess& p : m_Processes)
      {
        p.WaitToFinish().IgnoreResult();
      }

      m_pImpl->Close();
      return W_SUCCESS;
    }

    // we got some different message, ignore this
    // however, we need to adjust our timeout

    if (timeout.IsPositive())
    {
      // subtract the time that we spent
      const WTime now = WTime::Now();
      timeout -= now - tStart;
      tStart = now;

      // the timeout has been reached
      if (timeout.IsZeroOrNegative())
      {
        return W_FAILURE;
      }

      // otherwise try again, but with a reduced timeout
      dwTimeout = (DWORD)timeout.GetMilliseconds();
    }
  }
}

WResult WProcessGroup::TerminateAll(WInt32 iForcedExitCode /*= -2*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
  {
    // No job object - terminate processes individually
    auto result = W_SUCCESS;
    for (auto& process : m_Processes)
    {
      if (process.GetState() == WProcessState::Running && process.Terminate().Failed())
        result = W_FAILURE;
    }
    return result;
  }

  if (TerminateJobObject(m_pImpl->m_hJobObject, (UINT)iForcedExitCode) == FALSE)
  {
    WLog::Error("Failed to terminate process group '{}' - {}", m_pImpl->m_sName, WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }

  // Close the job object handle. The OS will kill all remaining processes in the job
  // due to JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE. No need to wait here.
  m_pImpl->Close();

  // Detach all tracked processes so their destructors don't block waiting for
  // termination that was already requested via the job object above.
  for (auto& process : m_Processes)
    process.Detach();

  return W_SUCCESS;
}

#endif
