#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>

const WHybridArray<WProcess, 8>& WProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif
