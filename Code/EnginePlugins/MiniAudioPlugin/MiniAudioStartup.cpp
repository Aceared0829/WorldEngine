#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/Startup.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(MiniAudio, MiniAudioPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&WMiniAudioSingleton::GameApplicationEventHandler);

    WMiniAudioSingleton::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&WMiniAudioSingleton::GameApplicationEventHandler);

    WMiniAudioSingleton::GetSingleton()->Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on
