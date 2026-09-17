#include <Player/Player.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>

// this injects the main function
W_APPLICATION_ENTRY_POINT(WPlayerApplication);

// these command line options may not all be directly used in WPlayer, but the WFallbackGameState reads those options to determine which scene to load
WCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the W SDK directory.", "");
WCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");


WPlayerApplication::WPlayerApplication()
  : WGameApplication("WPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

WResult WPlayerApplication::BeforeCoreSystemsStartup()
{
  // show the command line options, if help is requested
  {
    // since this is a GUI application (not a console app), printf has no effect
    // therefore we have to show the command line options with a message box

    WStringBuilder cmdHelp;
    if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      WLog::OsMessageBox(cmdHelp);
      SetReturnCode(-1);
      return W_FAILURE;
    }
  }

  WStartup::AddApplicationTag("player");

  W_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return W_SUCCESS;
}

void WPlayerApplication::DetermineProjectPath()
{
  WStringBuilder sProjectPath = opt_Project.GetOptionValue(WCommandLineOption::LogMode::FirstTime);

#if W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by WFileserve.
  // WFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../WorldEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const WStringBuilder sScenePath = opt_Scene.GetOptionValue(WCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = WFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (WFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "WProject", "WSdkRoot.txt").Failed())
    {
      // couldn't find the 'WProject' file in any parent folder of the scene
      m_sAppProjectPath = WFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!WPathUtils::IsAbsolutePath(sProjectPath))
  {
    // project path is not absolute, so must be relative to the SDK directory
    sProjectPath.Prepend(WFileSystem::GetSdkRootDirectory(), "/");
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/WProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = WFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;
}
