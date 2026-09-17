#pragma once

#if W_ENABLED(W_SUPPORTS_PROCESSES)
#  include <Foundation/System/Process.h>

/// Process groups are used to tie multiple processes together and ensure they get terminated either on demand or when the
/// application crashes
///
/// On Windows when an WProcessGroup instance is destroyed (either normally or due to a crash), all processes that have
/// been added to the group will be terminated by the OS. Other operating systems do not provide the terminate on crash guarantee.
///
/// Only processes that were launched asynchronously and in a suspended state can be added to process groups.
/// They will be resumed by the group.
class W_FOUNDATION_DLL WProcessGroup
{
  W_DISALLOW_COPY_AND_ASSIGN(WProcessGroup);

public:
  /// Creates a process group. The name is only used for debugging purposes.
  WProcessGroup(WStringView sGroupName = {});
  ~WProcessGroup();

  /// Launches a new process in the group.
  WResult Launch(const WProcessOptions& opt);

  /// Waits for all the processes in the group to terminate.
  ///
  /// Returns W_SUCCESS only if all processes have shut down.
  /// In all other cases, e.g. if the optional timeout is reached,
  /// W_FAILURE is returned.
  WResult WaitToFinish(WTime timeout = WTime::MakeZero());

  /// Tries to kill all processes associated with this group.
  ///
  /// Sends a kill command to all processes and then waits indefinitely for them to terminate.
  /// Note: iForcedExitCode is only supported on Windows.
  WResult TerminateAll(WInt32 iForcedExitCode = -2);

  /// Returns the container holding all processes of this group.
  ///
  /// This can be used to query per-process information such as exit codes.
  const WHybridArray<WProcess, 8>& GetProcesses() const;

private:
  WUniquePtr<struct WProcessGroupImpl> m_pImpl;

  WHybridArray<WProcess, 8> m_Processes;
};
#endif
