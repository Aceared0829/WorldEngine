#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Time.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Time)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    WTime::Initialize();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

W_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Time);
