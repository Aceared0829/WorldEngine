#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

#if defined(LIVEPP_ENABLED)
#  include <LPP_API_x64_CPP.h>
inline bool allow_hotreload = false;
inline lpp::LppDefaultAgent lppAgent;
#endif

WResult WRun_Startup(WApplication* pApplicationInstance)
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && defined(LIVEPP_ENABLED)
  // create a synchronized agent, loading the Live++ agent from the given path, e.g. "ThirdParty/LivePP"
  lppAgent = lpp::LppCreateDefaultAgent(nullptr, L"LivePP");
  // bail out in case the agent is not valid
  if (!lpp::LppIsValidDefaultAgent(&lppAgent))
  {
    WLog::Warning("Failed to create Live++ agent.");
  }
  else
  {
    WLog::Info("Live++ agent created.");
    allow_hotreload = true;
    lppAgent.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_NONE, nullptr, nullptr);
    // make Live++ handle dynamically loaded modules automatically, enabling them on load, disabling them on unload
    lppAgent.EnableAutomaticHandlingOfDynamicallyLoadedModules(nullptr, nullptr);
  }
#endif
  W_ASSERT_ALWAYS(pApplicationInstance != nullptr, "WRun() requires a valid non-null application instance pointer.");
  W_ASSERT_ALWAYS(WApplication::s_pApplicationInstance == nullptr, "There can only be one WApplication.");

  // Set application instance pointer to the supplied instance
  WApplication::s_pApplicationInstance = pApplicationInstance;

  W_SUCCEED_OR_RETURN(pApplicationInstance->BeforeCoreSystemsStartup());

  // this will startup all base and core systems
  // 'StartupHighLevelSystems' must not be done before a window is available (if at all)
  // so we don't do that here
  WStartup::StartupCoreSystems();

  pApplicationInstance->AfterCoreSystemsStartup();
  return W_SUCCESS;
}

void WRun_MainLoop(WApplication* pApplicationInstance)
{
  while (!pApplicationInstance->ShouldApplicationQuit())
  {
    pApplicationInstance->Run();
  }
}

void WRun_Shutdown(WApplication* pApplicationInstance)
{
  // high level systems shutdown
  // may do nothing, if the high level systems were never initialized
  {
    pApplicationInstance->BeforeHighLevelSystemsShutdown();
    WStartup::ShutdownHighLevelSystems();
    pApplicationInstance->AfterHighLevelSystemsShutdown();
  }

  // core systems shutdown
  {
    pApplicationInstance->BeforeCoreSystemsShutdown();
    WStartup::ShutdownCoreSystems();
    pApplicationInstance->AfterCoreSystemsShutdown();
  }

  // Flush standard output to make log available.
  fflush(stdout);
  fflush(stderr);

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  WApplication::s_pApplicationInstance = nullptr;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && defined(LIVEPP_ENABLED)
  // destroy the Live++ agent
  lpp::LppDestroyDefaultAgent(&lppAgent);
#endif

  // memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs
  // to be freed first
}

void WRun(WApplication* pApplicationInstance)
{
  if (WRun_Startup(pApplicationInstance).Succeeded())
  {
    WRun_MainLoop(pApplicationInstance);
    WRun_Shutdown(pApplicationInstance);
  }
  else
  {
    // nothing was started up, so only the bookkeeping at the end of WRun_Shutdown() applies
    fflush(stdout);
    fflush(stderr);

    WApplication::s_pApplicationInstance = nullptr;
  }
}
