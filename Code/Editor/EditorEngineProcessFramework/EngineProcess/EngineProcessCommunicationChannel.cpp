#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#elif W_ENABLED(W_PLATFORM_LINUX)
#  include <signal.h>
#endif

bool WEngineProcessCommunicationChannel::IsHostAlive() const
{
  if (WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return true;

  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  DWORD pid = static_cast<DWORD>(m_iHostPID);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  bValid = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr);

  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
    bValid = false;

  CloseHandle(hProcess);
#elif W_ENABLED(W_PLATFORM_LINUX)
  // We send the signal 0 to the given PID (signal 0 is a no-op)
  // If this succeeds, the process with the given PID exists
  // if it fails, the process does not / no longer exist.
  if (kill(m_iHostPID, 0) < 0)
    bValid = false;
#else
#  error Not implemented
#endif

  return bValid;
}

WResult WEngineProcessCommunicationChannel::ConnectToHostProcess()
{
  W_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  WResult res = W_SUCCESS;
  if (!WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    if (WCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
    {
      W_REPORT_FAILURE("Command Line does not contain -IPC parameter");
      return W_FAILURE;
    }

    if (WCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
    {
      W_REPORT_FAILURE("Command Line does not contain -PID parameter");
      return W_FAILURE;
    }

    m_iHostPID = 0;
    W_SUCCEED_OR_RETURN(WConversionUtils::StringToInt64(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID));

    WLog::Debug("Host Process ID: {0}", m_iHostPID);

    res = CreateAndConnectChannel(WIpcChannel::CreatePipeChannel(WCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), WIpcChannel::Mode::Client));
  }
  else
  {
    res = CreateAndConnectChannel(WIpcChannel::CreateNetworkChannel("localhost:1050", WIpcChannel::Mode::Server));
  }
  if (res.Failed())
  {
    WLog::Error("IpcChannel: CreateAndConnectChannel failed");
    return W_FAILURE;
  }

  return WaitForConnection(WTime::MakeFromSeconds(30));
}
