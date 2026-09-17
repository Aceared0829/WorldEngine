#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <future>

struct WPipeWin
{
  HANDLE m_pipeRead = nullptr;
  HANDLE m_pipeWrite = nullptr;
  std::thread m_readThread;
  std::atomic<bool> m_running = false;

  bool IsRunning() const
  {
    return m_running;
  }

  void Create()
  {
    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe for the child process.
    if (!CreatePipe(&m_pipeRead, &m_pipeWrite, &saAttr, 0))
      WLog::Error("WPipeWin: CreatePipe failed");

    // Ensure the read handle to the pipe is not inherited.
    if (!SetHandleInformation(m_pipeRead, HANDLE_FLAG_INHERIT, 0))
      WLog::Error("Stdout SetHandleInformation");
  }

  void Close()
  {
    if (m_pipeWrite)
    {
      CloseHandle(m_pipeWrite);
      m_pipeWrite = nullptr;

      if (m_readThread.joinable())
      {
        m_readThread.join();
      }
      CloseHandle(m_pipeRead);
      m_pipeRead = nullptr;
    }
  }

  static void ReportString(WDelegate<void(WStringView)> func, WHybridArray<char, 256>& ref_temp)
  {
    WStringBuilder result;

    WUnicodeUtils::RepairNonUtf8Text(ref_temp.GetData(), ref_temp.GetData() + ref_temp.GetCount(), result);
    func(result);
  }

  static void ReportString(WDelegate<void(WStringView)> func, const char* szStart, const char* szEnd)
  {
    WHybridArray<char, 256> tmp;

    while (szStart < szEnd)
    {
      tmp.PushBack(*szStart);
      ++szStart;
    }

    ReportString(func, tmp);
  }

  void StartRead(WDelegate<void(WStringView)>& ref_onStdOut)
  {
    if (m_pipeWrite)
    {
      m_running = true;
      m_readThread = std::thread([&]()
        {
          WHybridArray<char, 256> overflowBuffer;

          constexpr int BUFSIZE = 512;
          char chBuf[BUFSIZE];
          while (true)
          {
            DWORD bytesRead = 0;
            bool res = ReadFile(m_pipeRead, chBuf, BUFSIZE, &bytesRead, nullptr);
            if (!res || bytesRead == 0)
            {
              if (!overflowBuffer.IsEmpty())
              {
                ReportString(ref_onStdOut, overflowBuffer);
              }
              break;
            }

            const char* szCurrentPos = chBuf;
            const char* szEndPos = chBuf + bytesRead;

            while (szCurrentPos < szEndPos)
            {
              const char* szFound = WStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
              if (!szFound)
              {
                szFound = WStringUtils::FindSubString(szCurrentPos, "\r", szEndPos);
              }
              if (szFound)
              {
                if (overflowBuffer.IsEmpty())
                {
                  // If there is nothing in the overflow buffer this is a complete line and can be fired as is.
                  ReportString(ref_onStdOut, szCurrentPos, szFound + 1);
                }
                else
                {
                  // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.

                  while (szCurrentPos < szFound + 1)
                  {
                    overflowBuffer.PushBack(*szCurrentPos);
                    ++szCurrentPos;
                  }

                  ReportString(ref_onStdOut, overflowBuffer);

                  overflowBuffer.Clear();
                }

                szCurrentPos = szFound + 1;
              }
              else
              {
                // This is either the start or a middle segment of a line, append to overflow buffer.

                while (szCurrentPos < szEndPos)
                {
                  overflowBuffer.PushBack(*szCurrentPos);
                  ++szCurrentPos;
                }
              }
            }
          }
          m_running = false;
          //
        });
    }
  }
};

struct WProcessImpl
{
  WOsProcessHandle m_ProcessHandle = nullptr;
  WOsProcessHandle m_MainThreadHandle = nullptr;
  WOsProcessID m_ProcessID = 0;
  WPipeWin m_pipeStdOut;
  WPipeWin m_pipeStdErr;

  ~WProcessImpl() { Close(); }

  void Close()
  {
    if (m_MainThreadHandle != nullptr)
    {
      CloseHandle(m_MainThreadHandle);
      m_MainThreadHandle = nullptr;
    }

    if (m_ProcessHandle != nullptr)
    {
      CloseHandle(m_ProcessHandle);
      m_ProcessHandle = nullptr;
    }

    m_pipeStdOut.Close();
    m_pipeStdErr.Close();
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

WOsProcessHandle WProcess::GetProcessHandle() const
{
  return m_pImpl->m_ProcessHandle;
}

WOsProcessID WProcess::GetProcessID() const
{
  return m_pImpl->m_ProcessID;
}

WOsProcessID WProcess::GetCurrentProcessID()
{
  const WOsProcessID processID = GetCurrentProcessId();
  return processID;
}


// Taken from "Programmatically controlling which handles are inherited by new processes in Win32" by Raymond Chen
// https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
static BOOL CreateProcessWithExplicitHandles(LPCWSTR pApplicationName, LPWSTR pCommandLine, LPSECURITY_ATTRIBUTES pProcessAttributes,
  LPSECURITY_ATTRIBUTES pThreadAttributes, BOOL inheritHandles, DWORD uiCreationFlags, LPVOID pEnvironment, LPCWSTR pCurrentDirectory,
  LPSTARTUPINFOW pStartupInfo, LPPROCESS_INFORMATION pProcessInformation,
  // here is the new stuff
  DWORD uiHandlesToInherit, HANDLE* pHandlesToInherit)
{
  BOOL fSuccess;
  BOOL fInitialized = FALSE;
  SIZE_T size = 0;
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = nullptr;
  fSuccess = uiHandlesToInherit < 0xFFFFFFFF / sizeof(HANDLE) && pStartupInfo->cb == sizeof(*pStartupInfo);
  if (!fSuccess)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
  }

  if (uiHandlesToInherit > 0)
  {
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(nullptr, 1, 0, &size) || GetLastError() == ERROR_INSUFFICIENT_BUFFER;
    }
    if (fSuccess)
    {
      lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
      fSuccess = lpAttributeList != nullptr;
    }
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &size);
    }
    if (fSuccess)
    {
      fInitialized = TRUE;
      fSuccess = UpdateProcThreadAttribute(
        lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, pHandlesToInherit, uiHandlesToInherit * sizeof(HANDLE), nullptr, nullptr);
    }
  }

  if (fSuccess)
  {
    STARTUPINFOEXW info;
    ZeroMemory(&info, sizeof(info));
    info.StartupInfo = *pStartupInfo;
    info.StartupInfo.cb = sizeof(info);
    info.lpAttributeList = lpAttributeList;

    // it is both possible to pass in (STARTUPINFOW*)&info OR info.StartupInfo ...
    fSuccess = CreateProcessW(pApplicationName, pCommandLine, pProcessAttributes, pThreadAttributes, inheritHandles,
      uiCreationFlags | EXTENDED_STARTUPINFO_PRESENT, pEnvironment, pCurrentDirectory, &info.StartupInfo, pProcessInformation);
  }
  if (fInitialized)
    DeleteProcThreadAttributeList(lpAttributeList);
  if (lpAttributeList)
    HeapFree(GetProcessHeap(), 0, lpAttributeList);
  return fSuccess;
}

WResult WProcess::Launch(const WProcessOptions& opt, WBitflags<WProcessLaunchFlags> launchFlags /*= WAsyncProcessFlags::None*/)
{
  W_ASSERT_DEV(m_pImpl->m_ProcessHandle == nullptr, "Cannot reuse an instance of WProcess");
  W_ASSERT_DEV(m_pImpl->m_ProcessID == 0, "Cannot reuse an instance of WProcess");

  WStringBuilder sProcess = opt.m_sProcess;
  sProcess.MakeCleanPath();
  sProcess.ReplaceAll("/", "\\");

  m_sProcess = sProcess;
  m_OnStdOut = opt.m_onStdOut;
  m_OnStdError = opt.m_onStdError;

  STARTUPINFOW si;
  WMemoryUtils::ZeroFill(&si, 1);
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK; // do not show a wait cursor while launching the process

  // attention: passing in even a single null handle will fail the handle inheritance entirely,
  // but CreateProcess will still return success
  // therefore we must ensure to only pass non-null handles to inherit
  HANDLE HandlesToInherit[2];
  WUInt32 uiNumHandlesToInherit = 0;

  if (m_OnStdOut.IsValid())
  {
    m_pImpl->m_pipeStdOut.Create();
    si.hStdOutput = m_pImpl->m_pipeStdOut.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_pImpl->m_pipeStdOut.m_pipeWrite;
  }
  if (m_OnStdError.IsValid())
  {
    m_pImpl->m_pipeStdErr.Create();
    si.hStdError = m_pImpl->m_pipeStdErr.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_pImpl->m_pipeStdErr.m_pipeWrite;
  }

  // in theory this can be used to force the process's main window to be in the background,
  // but except for SW_HIDE and SW_SHOWMINNOACTIVE this doesn't work, and those are not useful
  // si.wShowWindow = SW_SHOWNOACTIVATE;
  // si.dwFlags |= STARTF_USESHOWWINDOW;

  PROCESS_INFORMATION pi;
  WMemoryUtils::ZeroFill(&pi, 1);


  WStringBuilder sCmdLine;
  BuildFullCommandLineString(opt, sProcess, sCmdLine);

  DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;

  if (opt.m_bHideConsoleWindow)
  {
    dwCreationFlags |= CREATE_NO_WINDOW;
  }

  if (launchFlags.IsSet(WProcessLaunchFlags::Suspended))
  {
    dwCreationFlags |= CREATE_SUSPENDED;
  }

  // We pass nullptr as lpApplicationName as setting it would prevent OpenProcess to run system apps or apps in PATH.
  // Instead, the module name is pre-pended to lpCommandLine in BuildFullCommandLineString.
  if (!CreateProcessWithExplicitHandles(nullptr, const_cast<wchar_t*>(WStringWChar(sCmdLine).GetData()),
        nullptr,                                  // lpProcessAttributes
        nullptr,                                  // lpThreadAttributes
        uiNumHandlesToInherit > 0 ? TRUE : FALSE, // bInheritHandles
        dwCreationFlags,
        nullptr,                                  // lpEnvironment
        opt.m_sWorkingDirectory.IsEmpty() ? nullptr : WStringWChar(opt.m_sWorkingDirectory).GetData(),
        &si,                                      // lpStartupInfo
        &pi,                                      // lpProcessInformation
        uiNumHandlesToInherit,                    // cHandlesToInherit
        HandlesToInherit                          // rgHandlesToInherit
        ))
  {
    m_pImpl->m_pipeStdOut.Close();
    m_pImpl->m_pipeStdErr.Close();
    WLog::Error("Failed to launch '{} {}' - {}", sProcess, WArgSensitive(sCmdLine, "CommandLine"), WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }
  m_pImpl->m_pipeStdOut.StartRead(m_OnStdOut);
  m_pImpl->m_pipeStdErr.StartRead(m_OnStdError);

  m_pImpl->m_ProcessHandle = pi.hProcess;
  m_pImpl->m_ProcessID = pi.dwProcessId;

  if (launchFlags.IsSet(WProcessLaunchFlags::Suspended))
  {
    // store the main thread handle for ResumeSuspended() later
    m_pImpl->m_MainThreadHandle = pi.hThread;
  }
  else
  {
    CloseHandle(pi.hThread);
  }

  if (launchFlags.IsSet(WProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return W_SUCCESS;
}

WResult WProcess::ResumeSuspended()
{
  if (m_pImpl->m_ProcessHandle == nullptr || m_pImpl->m_MainThreadHandle == nullptr)
    return W_FAILURE;

  const DWORD prevSuspendCount = ResumeThread(m_pImpl->m_MainThreadHandle);
  if (prevSuspendCount == -1)
  {
    WLog::Warning("WProcess::ResumeSuspended: Unexpected ResumeThread result ({})", WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }

  // invalidate the thread handle, so that we cannot resume the process twice
  if (!CloseHandle(m_pImpl->m_MainThreadHandle))
  {
    WLog::Warning("WProcess::ResumeSuspended: Failed to close handle");
  }

  m_pImpl->m_MainThreadHandle = nullptr;

  return W_SUCCESS;
}

WResult WProcess::WaitToFinish(WTime timeout /*= WTime::MakeZero()*/)
{
  W_ASSERT_DEV(m_pImpl->m_ProcessHandle != nullptr, "Launch a process before waiting on it");
  W_ASSERT_DEV(m_pImpl->m_ProcessID != 0, "Launch a process before waiting on it");

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  const DWORD res = WaitForSingleObject(m_pImpl->m_ProcessHandle, dwTimeout);

  if (res == WAIT_TIMEOUT)
  {
    // the process is not yet finished, the timeout was reached
    return W_FAILURE;
  }

  if (res == WAIT_FAILED)
  {
    WLog::Error("Failed to wait for '{}' - {}", m_sProcess, WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }

  // the process has finished

  m_pImpl->m_pipeStdOut.Close();
  m_pImpl->m_pipeStdErr.Close();

  GetExitCodeProcess(m_pImpl->m_ProcessHandle, reinterpret_cast<DWORD*>(&m_iExitCode));

  return W_SUCCESS;
}

WResult WProcess::Execute(const WProcessOptions& opt, WInt32* out_pExitCode /*= nullptr*/)
{
  WProcess proc;

  W_SUCCEED_OR_RETURN(proc.Launch(opt));
  W_SUCCEED_OR_RETURN(proc.WaitToFinish());

  if (out_pExitCode != nullptr)
  {
    *out_pExitCode = proc.GetExitCode();
  }

  return W_SUCCESS;
}

WResult WProcess::Terminate()
{
  W_ASSERT_DEV(m_pImpl->m_ProcessHandle != nullptr, "Launch a process before terminating it");
  W_ASSERT_DEV(m_pImpl->m_ProcessID != 0, "Launch a process before terminating it");

  if (TerminateProcess(m_pImpl->m_ProcessHandle, 0xFFFFFFFF) == FALSE)
  {
    const DWORD err = GetLastError();
    if (err == ERROR_ACCESS_DENIED) // this means the process already terminated, so from our perspective the goal was achieved
      return W_SUCCESS;

    WLog::Error("Failed to terminate process '{}' - {}", m_sProcess, WArgErrorCode(GetLastError()));
    return W_FAILURE;
  }

  W_SUCCEED_OR_RETURN(WaitToFinish());

  return W_SUCCESS;
}

WProcessState WProcess::GetState() const
{
  if (m_pImpl->m_ProcessHandle == 0)
    return WProcessState::NotStarted;

  DWORD exitCode = 0;
  if (GetExitCodeProcess(m_pImpl->m_ProcessHandle, &exitCode) == FALSE)
  {
    WLog::Error("Failed to retrieve exit code for process '{}' - {}", m_sProcess, WArgErrorCode(GetLastError()));

    // not sure what kind of errors can happen (probably access denied and such)
    // have to return something, so lets claim the process is finished
    return WProcessState::Finished;
  }

  if (exitCode == STILL_ACTIVE)
    return WProcessState::Running;

  if (m_ProcessExited.IsZero())
  {
    m_ProcessExited = WTime::Now();
  }

  // Do not consider a process finished if the pipe threads have not exited yet.
  if (m_pImpl->m_pipeStdOut.IsRunning() || m_pImpl->m_pipeStdErr.IsRunning())
  {
    if (WTime::Now() - m_ProcessExited < WTime::MakeFromSeconds(2))
    {
      return WProcessState::Running;
    }

    m_pImpl->m_pipeStdOut.Close();
    m_pImpl->m_pipeStdErr.Close();
  }

  m_iExitCode = (WInt32)exitCode;
  return WProcessState::Finished;
}

void WProcess::Detach()
{
  // throw away the previous WProcessImpl and create a blank one
  m_pImpl = W_DEFAULT_NEW(WProcessImpl);

  // reset the exit code to the default
  m_iExitCode = -0xFFFF;
}
