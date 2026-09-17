#include <PacManGame/PacManGame.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// this injects the C++ main() function
W_APPLICATION_ENTRY_POINT(WPacManGame);

WPacManGame::WPacManGame()
  : WGameApplication("PacMan", nullptr)
{
}

WResult WPacManGame::TryProjectFolder(WStringView sPath)
{
  WStringBuilder sProjDir = sPath;
  sProjDir.MakeCleanPath();

  WStringBuilder sProjFile;
  sProjFile.SetPath(sProjDir, "WProject");

  if (sProjFile.IsAbsolutePath() && WOSFile::ExistsFile(sProjFile))
  {
    m_sAppProjectPath = sProjDir;
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WPacManGame::DetermineProjectPath()
{
  // IMPORTANT!
  //
  // The project path has to be set for the WGameApplication to know where the main 'project' data directory is.
  // Without it, nothing will work (the game plugin won't be loaded etc).
  //
  // The path can be relative to the '>SDK' directory (the root folder where W is located).
  // It may also be absolute (though this isn't portable across machines).
  // Or it can be relative to WOSFile::GetApplicationDirectory() (where the Game.exe is).
  //
  // If your project is inside the W directory, use a relative path from there.
  // If it is somewhere outside, you either need to use an absolute path or some other way to locate it.
  //
  // Note that in a final exported build the project folder is always merged with the W data folders into one package.

#ifdef GAME_PROJECT_FOLDER
  // this absolute path will only work on the machine where the game is compiled,
  // but it works for projects that are located outside the WorldEngine folder
  if (TryProjectFolder(W_PP_STRINGIFY(GAME_PROJECT_FOLDER)).Succeeded())
    return;
#endif

  // this path works for exported projects, because during export the project folder is always copied there
  WStringBuilder sProjDir;
  if (WFileSystem::ResolveSpecialDirectory(">sdk/Data/project", sProjDir).Succeeded())
  {
    if (TryProjectFolder(sProjDir).Succeeded())
      return;
  }

  // in other cases, try this relative path
  m_sAppProjectPath = "Data/Samples/PacMan";
}

WUniquePtr<WGameStateBase> WPacManGame::CreateGameState()
{
  // usually we should only have a single non-fallback gamestate which is automatically picked
  // but if necessary, we can override this here
  return SUPER::CreateGameState();
}

WResult WPacManGame::BeforeCoreSystemsStartup()
{
  WStartup::AddApplicationTag("game");

  W_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return W_SUCCESS;
}

void WPacManGame::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  WStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, WFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the WFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, WTransform::MakeIdentity());
}
