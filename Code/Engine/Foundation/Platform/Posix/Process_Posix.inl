#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/System/SystemInformation.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#if W_ENABLED(W_USE_LINUX_POSIX_EXTENSIONS)
#  include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

#ifndef _W_DEFINED_POLLFD_POD
#  define _W_DEFINED_POLLFD_POD
W_DEFINE_AS_POD_TYPE(struct pollfd);
#endif

class WFd
{
public:
  WFd() = default;
  WFd(const WFd&) = delete;
  WFd(WFd&& other)
  {
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  ~WFd()
  {
    Close();
  }

  void Close()
  {
    if (m_fd != -1)
    {
      close(m_fd);
      m_fd = -1;
    }
  }

  bool IsValid() const
  {
    return m_fd >= 0;
  }

  void operator=(const WFd&) = delete;
  void operator=(WFd&& other)
  {
    Close();
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  void TakeOwnership(int fd)
  {
    Close();
    m_fd = fd;
  }

  int Borrow() const { return m_fd; }

  int Detach()
  {
    auto result = m_fd;
    m_fd = -1;
    return result;
  }

  WResult AddFlags(int addFlags)
  {
    if (m_fd < 0)
      return W_FAILURE;

    if (addFlags & O_CLOEXEC)
    {
      int flags = fcntl(m_fd, F_GETFD);
      flags |= FD_CLOEXEC;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        WLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return W_FAILURE;
      }
      addFlags &= ~O_CLOEXEC;
    }

    if (addFlags)
    {
      int flags = fcntl(m_fd, F_GETFL);
      flags |= addFlags;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        WLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return W_FAILURE;
      }
    }

    return W_SUCCESS;
  }

  static WResult MakePipe(WFd (&fds)[2], int flags = 0)
  {
    fds[0].Close();
    fds[1].Close();
#if W_ENABLED(W_USE_LINUX_POSIX_EXTENSIONS)
    if (pipe2((int*)fds, flags) != 0)
    {
      return W_FAILURE;
    }
#else
    if (pipe((int*)fds) != 0)
    {
      return W_FAILURE;
    }
    if (flags != 0 && (fds[0].AddFlags(flags).Failed() || fds[1].AddFlags(flags).Failed()))
    {
      fds[0].Close();
      fds[1].Close();
      return W_FAILURE;
    }
#endif
    return W_SUCCESS;
  }

private:
  int m_fd = -1;
};

namespace
{
  struct ProcessStartupError
  {
    enum class Type : WUInt32
    {
      FailedToChangeWorkingDirectory = 0,
      FailedToExecv = 1,
      FailedToSetParentDeathSignal = 2
    };

    Type type;
    int errorCode;
  };

  static WInt32 GetExitCodeFromWaitStatus(int childStatus)
  {
    if (WIFEXITED(childStatus))
    {
      return WEXITSTATUS(childStatus);
    }

    if (WIFSIGNALED(childStatus))
    {
      return WTERMSIG(childStatus);
    }

    return -1;
  }

  static pid_t WaitPidInterruptedRetry(pid_t childPid, int* pChildStatus, int iOptions)
  {
    pid_t waitedPid = -1;
    do
    {
      waitedPid = waitpid(childPid, pChildStatus, iOptions);
    } while (waitedPid < 0 && errno == EINTR);

    return waitedPid;
  }
} // namespace

#if W_ENABLED(W_USE_LINUX_POSIX_EXTENSIONS)
namespace WInternal
{
  static thread_local bool s_bSetProcessParentDeathSignal = false;

  bool SetProcessLaunchParentDeathSignal(bool bEnable)
  {
    const bool bPrevious = s_bSetProcessParentDeathSignal;
    s_bSetProcessParentDeathSignal = bEnable;
    return bPrevious;
  }

  static bool GetProcessLaunchParentDeathSignal()
  {
    return s_bSetProcessParentDeathSignal;
  }
} // namespace WInternal
#endif


struct WProcessImpl
{
  ~WProcessImpl()
  {
    StopStreamWatcher();
  }

  pid_t m_childPid = -1;
  bool m_exitCodeAvailable = false;
  bool m_processSuspended = false;

  struct StdStreamInfo
  {
    WFd fd;
    WDelegate<void(WStringView)> callback;
  };
  WHybridArray<StdStreamInfo, 2> m_streams;
  WDynamicArray<WStringBuilder> m_overflowBuffers;
  WUniquePtr<WOSThread> m_streamWatcherThread;
  WFd m_wakeupPipeReadEnd;
  WFd m_wakeupPipeWriteEnd;

  static void* StreamWatcherThread(void* context)
  {
    WProcessImpl* self = reinterpret_cast<WProcessImpl*>(context);
    char buffer[4096];

    WHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd.Borrow(), POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd.Borrow(), POLLIN, 0});
    }

    bool run = true;
    while (run)
    {
      int result = poll(pollfds.GetData(), pollfds.GetCount(), -1);
      if (result > 0)
      {
        // Result at index 0 is special and means there was a WakeUp
        if (pollfds[0].revents != 0)
        {
          run = false;
        }

        for (WUInt32 i = 1; i < pollfds.GetCount(); ++i)
        {
          if (pollfds[i].revents & POLLIN)
          {
            WStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo& stream = self->m_streams[i - 1];
            while (true)
            {
              ssize_t numBytes = read(stream.fd.Borrow(), buffer, W_ARRAY_SIZE(buffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                WLog::Error("Process Posix read error on {}: {}", stream.fd.Borrow(), errno);
                return nullptr;
              }

              const char* szCurrentPos = buffer;
              const char* szEndPos = buffer + numBytes;
              while (szCurrentPos < szEndPos)
              {
                const char* szFound = WStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
                if (szFound)
                {
                  if (overflowBuffer.IsEmpty())
                  {
                    // If there is nothing in the overflow buffer this is a complete line and can be fired as is.
                    stream.callback(WStringView(szCurrentPos, szFound + 1));
                  }
                  else
                  {
                    // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.
                    overflowBuffer.Append(WStringView(szCurrentPos, szFound + 1));
                    stream.callback(overflowBuffer);
                    overflowBuffer.Clear();
                  }
                  szCurrentPos = szFound + 1;
                }
                else
                {
                  // This is either the start or a middle segment of a line, append to overflow buffer.
                  overflowBuffer.Append(WStringView(szCurrentPos, szEndPos));
                  szCurrentPos = szEndPos;
                }
              }

              if (numBytes < W_ARRAY_SIZE(buffer))
              {
                break;
              }
            }
          }
          pollfds[i].revents = 0;
        }
      }
      else if (result < 0)
      {
        WLog::Error("poll error {}", errno);
        break;
      }
    }

    for (WUInt32 i = 0; i < self->m_streams.GetCount(); ++i)
    {
      WStringBuilder& overflowBuffer = self->m_overflowBuffers[i];
      if (!overflowBuffer.IsEmpty())
      {
        self->m_streams[i].callback(overflowBuffer);
        overflowBuffer.Clear();
      }

      self->m_streams[i].fd.Close();
    }

    return nullptr;
  }

  WResult StartStreamWatcher()
  {
    WFd wakeupPipe[2];
    if (WFd::MakePipe(wakeupPipe, O_NONBLOCK | O_CLOEXEC).Failed())
    {
      WLog::Error("Failed to setup wakeup pipe {}", errno);
      return W_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd = std::move(wakeupPipe[0]);
      m_wakeupPipeWriteEnd = std::move(wakeupPipe[1]);
    }

    m_streamWatcherThread = W_DEFAULT_NEW(WOSThread, &StreamWatcherThread, this, "StdStrmWtch");
    m_streamWatcherThread->Start();

    return W_SUCCESS;
  }

  void StopStreamWatcher()
  {
    if (m_streamWatcherThread)
    {
      char c = 0;
      W_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd.Borrow(), &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    m_wakeupPipeReadEnd.Close();
    m_wakeupPipeWriteEnd.Close();
  }

  void AddStream(WFd fd, const WDelegate<void(WStringView)>& callback)
  {
    m_streams.PushBack({std::move(fd), callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  WUInt32 GetNumStreams() const { return m_streams.GetCount(); }

  static WResult StartChildProcess(const WProcessOptions& opt, pid_t& outPid, bool suspended, WFd& outStdOutFd, WFd& outStdErrFd)
  {
    WFd stdoutPipe[2];
    WFd stderrPipe[2];
    WFd startupErrorPipe[2];

    WStringBuilder executablePath = opt.m_sProcess;
    WFileStats stats;
    if (!opt.m_sProcess.IsAbsolutePath())
    {
      executablePath = WOSFile::GetCurrentWorkingDirectory();
      executablePath.AppendPath(opt.m_sProcess);
    }

    if (WOSFile::GetFileStats(executablePath, stats).Failed() || stats.m_bIsDirectory)
    {
      WHybridArray<char, 512> confPath;
      auto homePath = getenv("HOME");
      auto envPATH = getenv("PATH");
      if (envPATH == nullptr) // if no PATH environment variable is available, we need to fetch the system default;
      {
#if _POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE
        size_t confPathSize = confstr(_CS_PATH, nullptr, 0);
        if (confPathSize > 0)
        {
          confPath.SetCountUninitialized(confPathSize);
          if (confstr(_CS_PATH, confPath.GetData(), confPath.GetCount()) == 0)
          {
            confPath.SetCountUninitialized(0);
          }
        }
#endif
        if (confPath.GetCount() == 0)
        {
          confPath.PushBack('\0');
        }
        envPATH = confPath.GetData();
      }

      WStringView path = envPATH;
      WHybridArray<WStringView, 16> pathParts;
      path.Split(false, pathParts, ":");

      for (auto& pathPart : pathParts)
      {
        // Linux allows environmental values to be relative to home directory
        // e.g. PATH=$PATH:.local/bin or PATH=$PATH:~
        // Handle here or crash

        executablePath.Clear();

        if(!pathPart.IsAbsolutePath()){
            executablePath.AppendPath(homePath);
            executablePath.Append("/");
        }
        executablePath.AppendPath(pathPart);
        executablePath.AppendPath(opt.m_sProcess);
        if (WOSFile::GetFileStats(executablePath, stats).Succeeded() && !stats.m_bIsDirectory)
        {
          break;
        }
        executablePath.Clear();
      }
    }

    if (executablePath.IsEmpty())
    {
      return W_FAILURE;
    }

    if (opt.m_onStdOut.IsValid())
    {
      if (WFd::MakePipe(stdoutPipe).Failed())
      {
        return W_FAILURE;
      }
      if (stdoutPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return W_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (WFd::MakePipe(stderrPipe).Failed())
      {
        return W_FAILURE;
      }
      if (stderrPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return W_FAILURE;
      }
    }

    if (WFd::MakePipe(startupErrorPipe, O_CLOEXEC).Failed())
    {
      return W_FAILURE;
    }

#if W_ENABLED(W_USE_LINUX_POSIX_EXTENSIONS)
    const bool bSetParentDeathSignal = WInternal::GetProcessLaunchParentDeathSignal();
    const pid_t parentPid = getpid();
#endif

    WHybridArray<char*, 9> args;

    args.PushBack(const_cast<char*>(executablePath.GetData()));
    for (const WString& arg : opt.m_Arguments)
    {
      args.PushBack(const_cast<char*>(arg.GetData()));
    }
    args.PushBack(nullptr);

    pid_t childPid = fork();
    if (childPid < 0)
    {
      return W_FAILURE;
    }

    if (childPid == 0) // We are the child
    {
      // DANGER! We are the child process and are now working on a shadow copy of the parent process including the state of all locks etc. This means that if we hit any locks, e.g. by allocating memory we will deadlock if at the point of fork the lock was held by a different thread. So between this line and the call to `execv` we must not make any allocations or access any high level code.
#if W_ENABLED(W_USE_LINUX_POSIX_EXTENSIONS)
      if (bSetParentDeathSignal)
      {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0)
        {
          auto err = ProcessStartupError{ProcessStartupError::Type::FailedToSetParentDeathSignal, errno};
          W_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
          startupErrorPipe[1].Close();
          _exit(-1);
        }

        if (getppid() != parentPid)
        {
          _exit(-1);
        }
      }
#endif

      if (suspended)
      {
        if (raise(SIGSTOP) < 0)
        {
          _exit(-1);
        }
      }

      if (opt.m_bHideConsoleWindow == true)
      {
        // Redirect STDIN to /dev/null
        int stdinReplace = open("/dev/null", O_RDONLY);
        dup2(stdinReplace, STDIN_FILENO);
        close(stdinReplace);

        if (!opt.m_onStdOut.IsValid())
        {
          int stdoutReplace = open("/dev/null", O_WRONLY);
          dup2(stdoutReplace, STDOUT_FILENO);
          close(stdoutReplace);
        }

        if (!opt.m_onStdError.IsValid())
        {
          int stderrReplace = open("/dev/null", O_WRONLY);
          dup2(stderrReplace, STDERR_FILENO);
          close(stderrReplace);
        }
      }
      else
      {
        // TODO: Launch a x-terminal-emulator with the command and somehow redirect STDOUT, etc?
        W_ASSERT_NOT_IMPLEMENTED;
      }

      if (opt.m_onStdOut.IsValid())
      {
        stdoutPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1].Borrow(), STDOUT_FILENO); // redirect the write end to STDOUT
        stdoutPipe[1].Close();
      }

      if (opt.m_onStdError.IsValid())
      {
        stderrPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1].Borrow(), STDERR_FILENO); // redirect the write end to STDERR
        stderrPipe[1].Close();
      }

      startupErrorPipe[0].Close(); // we don't need the read end of the startup error pipe in the child process

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          auto err = ProcessStartupError{ProcessStartupError::Type::FailedToChangeWorkingDirectory, 0};
          W_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
          startupErrorPipe[1].Close();
          _exit(-1);
        }
      }

      if (execv(executablePath, args.GetData()) < 0)
      {
        auto err = ProcessStartupError{ProcessStartupError::Type::FailedToExecv, errno};
        W_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
        startupErrorPipe[1].Close();
        _exit(-1);
      }
    }
    else
    {
      startupErrorPipe[1].Close(); // We don't need the write end of the startup error pipe in the parent process
      stdoutPipe[1].Close();       // Don't need the write end in the parent process
      stderrPipe[1].Close();       // Don't need the write end in the parent process

      ProcessStartupError err = {};
      auto errSize = read(startupErrorPipe[0].Borrow(), &err, sizeof(err));
      startupErrorPipe[0].Close(); // we no longer need the read end of the startup error pipe

      // There are two possible cases here
      // Case 1: errSize is equal to 0, which means no error happened on the startupErrorPipe was closed during the execv call
      // Case 2: errSize > 0 in which case there was an error before the pipe was closed normally.
      if (errSize > 0)
      {
        W_ASSERT_DEV(errSize == sizeof(err), "Child process should have written a full ProcessStartupError struct");
        switch (err.type)
        {
          case ProcessStartupError::Type::FailedToChangeWorkingDirectory:
            WLog::Error("Failed to start process '{}' because the given working directory '{}' is invalid", opt.m_sProcess, opt.m_sWorkingDirectory);
            break;
          case ProcessStartupError::Type::FailedToExecv:
            WLog::Error("Failed to exec when starting process '{}' the error code is '{}'", opt.m_sProcess, err.errorCode);
            break;
          case ProcessStartupError::Type::FailedToSetParentDeathSignal:
            WLog::Error("Failed to configure parent death signal when starting process '{}' the error code is '{}'", opt.m_sProcess, err.errorCode);
            break;
        }
        return W_FAILURE;
      }

      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        outStdOutFd = std::move(stdoutPipe[0]);
      }

      if (opt.m_onStdError.IsValid())
      {
        outStdErrFd = std::move(stderrPipe[0]);
      }
    }

    return W_SUCCESS;
  }
};

WProcess::WProcess()
{
  m_pImpl = W_DEFAULT_NEW(WProcessImpl);
}

WProcess::~WProcess()
{
  if (GetState() == WProcessState::Running)
  {
    WLog::Dev("Process still running - terminating '{}'", m_sProcess);

    Terminate().IgnoreResult();
  }

  // Explicitly clear the implementation here so that member
  // state (e.g. delegates) used by the impl survives the implementation.
  m_pImpl.Clear();
}

WResult WProcess::Execute(const WProcessOptions& opt, WInt32* out_iExitCode /*= nullptr*/)
{
  pid_t childPid = 0;
  WFd stdoutFd;
  WFd stderrFd;
  if (WProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return W_FAILURE;
  }

  WProcessImpl impl;
  if (stdoutFd.IsValid())
  {
    impl.AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    impl.AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (impl.GetNumStreams() > 0 && impl.StartStreamWatcher().Failed())
  {
    return W_FAILURE;
  }

  int childStatus = -1;
  pid_t waitedPid = WaitPidInterruptedRetry(childPid, &childStatus, 0);
  if (waitedPid < 0)
  {
    return W_FAILURE;
  }
  if (out_iExitCode != nullptr)
  {
    *out_iExitCode = GetExitCodeFromWaitStatus(childStatus);
  }
  return W_SUCCESS;
}

WResult WProcess::Launch(const WProcessOptions& opt, WBitflags<WProcessLaunchFlags> launchFlags /*= WProcessLaunchFlags::None*/)
{
  W_ASSERT_DEV(m_pImpl->m_childPid == -1, "Can not reuse an instance of WProcess");

  WFd stdoutFd;
  WFd stderrFd;

  if (WProcessImpl::StartChildProcess(opt, m_pImpl->m_childPid, launchFlags.IsSet(WProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return W_FAILURE;
  }

  m_pImpl->m_exitCodeAvailable = false;
  m_pImpl->m_processSuspended = launchFlags.IsSet(WProcessLaunchFlags::Suspended);

  if (stdoutFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (m_pImpl->GetNumStreams() > 0)
  {
    if (m_pImpl->StartStreamWatcher().Failed())
    {
      return W_FAILURE;
    }
  }

  if (launchFlags.IsSet(WProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return W_SUCCESS;
}

WResult WProcess::ResumeSuspended()
{
  if (m_pImpl->m_childPid < 0 || !m_pImpl->m_processSuspended)
  {
    return W_FAILURE;
  }

  if (kill(m_pImpl->m_childPid, SIGCONT) < 0)
  {
    return W_FAILURE;
  }
  m_pImpl->m_processSuspended = false;
  return W_SUCCESS;
}

WResult WProcess::WaitToFinish(WTime timeout /*= WTime::MakeZero()*/)
{
  if (m_pImpl->m_exitCodeAvailable)
  {
    return W_SUCCESS;
  }

  if (timeout.IsZero())
  {
    int childStatus = 0;
    int waitResult = WaitPidInterruptedRetry(m_pImpl->m_childPid, &childStatus, 0);
    if (waitResult > 0)
    {
      m_iExitCode = GetExitCodeFromWaitStatus(childStatus);
      m_pImpl->m_exitCodeAvailable = true;

      m_pImpl->StopStreamWatcher();

      return W_SUCCESS;
    }
    return W_FAILURE;
  }
  else
  {
    WTime startWait = WTime::Now();
    while (true)
    {
      const WProcessState state = GetState();
      switch (state)
      {
        case WProcessState::NotStarted:
          return W_FAILURE;
        case WProcessState::Running:
          break;
        case WProcessState::Finished:
          return W_SUCCESS;
      }

      WTime timeSpent = WTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return W_FAILURE;
      }
      WThreadUtils::Sleep(WMath::Min(WTime::MakeFromMilliseconds(100.0), timeout - timeSpent));
    }
  }
  return W_SUCCESS;
}

WResult WProcess::Terminate()
{
  if (m_pImpl->m_childPid == -1)
  {
    return W_FAILURE;
  }

  W_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (kill(m_pImpl->m_childPid, SIGKILL) < 0)
  {
    if (errno != ESRCH) // ESRCH = Process does not exist
    {
      return W_FAILURE;
    }
  }

  int childStatus = 0;
  if (WaitPidInterruptedRetry(m_pImpl->m_childPid, &childStatus, 0) < 0)
  {
    if (errno != ECHILD) // ECHILD = Process is not a child of the calling process or was already waited for
    {
      return W_FAILURE;
    }
  }

  m_pImpl->m_exitCodeAvailable = true;
  m_iExitCode = -1;

  return W_SUCCESS;
}

WProcessState WProcess::GetState() const
{
  if (m_pImpl->m_childPid == -1)
  {
    return WProcessState::NotStarted;
  }

  if (m_pImpl->m_exitCodeAvailable)
  {
    return WProcessState::Finished;
  }

  int childStatus = -1;
  int waitResult = WaitPidInterruptedRetry(m_pImpl->m_childPid, &childStatus, WNOHANG);
  if (waitResult > 0)
  {
    m_iExitCode = GetExitCodeFromWaitStatus(childStatus);
    m_pImpl->m_exitCodeAvailable = true;

    m_pImpl->StopStreamWatcher();

    return WProcessState::Finished;
  }

  return WProcessState::Running;
}

void WProcess::Detach()
{
  m_pImpl->m_childPid = -1;
}

WOsProcessHandle WProcess::GetProcessHandle() const
{
  W_ASSERT_DEV(false, "There is no process handle on posix");
  return nullptr;
}

WOsProcessID WProcess::GetProcessID() const
{
  W_ASSERT_DEV(m_pImpl->m_childPid != -1, "No ProcessID available");
  return m_pImpl->m_childPid;
}

WOsProcessID WProcess::GetCurrentProcessID()
{
  return getpid();
}
