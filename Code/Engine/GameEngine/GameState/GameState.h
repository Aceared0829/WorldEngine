#pragma once

#include <Core/GameState/GameStateBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Utils/SceneLoadUtil.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WWindow;
class WWindowOutputTargetBase;
class WView;
class WWindowOutputTargetGAL;
class WDummyXR;
struct WWindowEvent;

using WRenderPipelineResourceHandle = WTypedResourceHandle<class WRenderPipelineResource>;

/// WGameState implements the WGameStateBase interface and adds several convenience features.
///
/// For an explanation what game states are, see the online documentation:
/// https://ezengine.net/pages/docs/runtime/application/game-state.html
///
/// The WGameState adds some default functionality:
/// * Creation of a main window and render pipeline
/// * A main view handle
/// * A main camera object
/// * A main world that is currently active
/// * Background loading of scenes
/// * A separate world used as a loading screen
/// * automatic player prefab spawning if a WPlayerStartPointComponent is part of the scene
/// * automatically applies the state of the "Main View" WCameraComponent in the scene
/// * Many additional hooks to customize only specific parts, such as the window creation
///
/// Typically you would derive from WGameState and then override functions like
/// `ProcessInput()` and `ConfigureMainCamera()`. Take a look at `WFallbackGameState` for inspiration.
class W_GAMEENGINE_DLL WGameState : public WGameStateBase
{
  W_ADD_DYNAMIC_REFLECTION(WGameState, WGameStateBase)

protected:
  /// This class cannot be instantiated directly.
  WGameState();

public:
  virtual ~WGameState();

  /// Returns the active WGameState. Only one WGameState is allowed to exist.
  static WGameState* GetActiveGameState();

  /// Returns the WWorld that is currently the active one.
  WWorld* GetMainWorld() { return m_pMainWorld; }

  /// Gives access to the game state's main camera object.
  WCamera* GetMainCamera() { return &m_MainCamera; }

  /// Returns the WView that is currently the one used for rendering the main output.
  WView* GetMainView();

  /// Whether a scene is currently being loaded.
  bool IsLoadingSceneInBackground(float* out_pProgress = nullptr) const;

  /// Whether the game state currently displays a loading screen. This usually implies that a scene is being loaded as well.
  bool IsInLoadingScreen() const;

  /// Called upon game startup.
  ///
  /// Calls CreateActors() to create the game's main window and setup input devices.
  /// Calls ConfigureInputActions() to setup input actions.
  /// Finally switches to pWorld (if available) or starts loading the scene that GetStartupOptions() returns.
  ///
  /// Override any of the above functions to customize them.
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;

  /// Cleans up the main window before the game is shut down.
  virtual void OnDeactivation() override;

  /// Makes sure m_hMainView gets rendered. Mainly needed by the editor.
  virtual void AddMainViewsToRender() override;

  /// Simply stores that the game should stop.
  ///
  /// Override this to add more elaborate logic, if necessary.
  virtual void RequestQuit(WStringView sRequestedBy) override;

  /// Whether RequestQuit() was called before.
  virtual bool WasQuitRequested() const override;

  /// The WGameState doesn't implement any input logic, but it forwards to UpdateBackgroundSceneLoading().
  virtual void ProcessInput() override;

  /// Immediately switches to a loading screen and starts loading a level.
  ///
  /// When the level is finished loading, `OnBackgroundSceneLoadingFinished()` is called, which switches to it immediately, unless overridden.
  /// If the scene was already fully preloaded, the switch happens immediately, without showing a loading screen.
  void LoadScene(WStringView sSceneFile, WStringView sPreloadCollection, WStringView sStartPosition, const WTransform& startPositionOffset);

  /// Convenience function to switch to a loading screen.
  ///
  /// Nothing actually gets loaded. Without further logic, the app will stay in the loading screen indefinitely.
  /// sTargetSceneFile is only passed in, so that the loading screen can be customized accordingly,
  /// for example it may show a screenshot of the target scene.
  void SwitchToLoadingScreen(WStringView sTargetSceneFile);

  /// Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  ///
  /// Calls OnChangedMainWorld() afterwards, so that you can follow up on a scene change as needed.
  /// Calls ConfigureMainCamera() as well.
  void ChangeMainWorld(WWorld* pNewMainWorld, WStringView sStartPosition, const WTransform& startPositionOffset);

  /// Starts loading a scene in the background.
  ///
  /// If available, a collection can be provided. Resources referenced in the collection will be fully preloaded first and then
  /// the scene is loaded. This is the only way to get a proper estimation of loading progress and is necessary to get a smooth
  /// start, otherwise the engine will have to load resources on-demand, many of which will be needed during the first frame.
  ///
  /// Once finished, one of these hooks is executed:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  ///   `OnBackgroundSceneLoadingCanceled()`
  void StartBackgroundSceneLoading(WStringView sSceneFile, WStringView sPreloadCollection);

  /// If a scene is currently being loaded in the background, cancel the loading.
  ///
  /// Calls `OnBackgroundSceneLoadingCanceled()` if a scene was loading.
  void CancelBackgroundSceneLoading();

protected:
  /// Creates a default WWindow and adds it to the application
  ///
  /// The base implementation calls CreateMainWindow(), CreateMainOutputTarget() and SetupMainView() to configure the main window.
  virtual void CreateWindows();

  /// Adds custom input actions, if necessary.
  /// Unless overridden OnActivation() will call this.
  virtual void ConfigureInputActions();

  /// Overrideable function that may create a player object.
  ///
  /// By default called by OnChangedMainWorld() when switching to a non-loading screen world.
  /// The default implementation will search the world for WPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the WPlayerStartComponent will be used.
  ///
  /// sStartPosition allows to spawn the player at another named location, but there is no default implementation for such logic.
  ///
  /// Returns W_SUCCESS if a prefab was spawned, W_FAILURE if nothing was done.
  virtual WResult SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset);

  /// Creates an XR Actor, if XR is configured and available for the project.
  WRegisteredWndHandle CreateXRWindow();

  /// Creates a default main view.
  WView* CreateMainView();

  /// Executed when ChangeMainWorld() is used to switch to a new world.
  ///
  /// Override this to be informed about scene changes.
  /// This happens right at startup (both for given worlds and custom created ones)
  /// and when the game needs to switch to a new level.
  virtual void OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset);

  /// Searches for a "Main View" WCameraComponent in the world and uses that for the camera position, if available.
  ///
  /// Override this for custom camera logic.
  virtual void ConfigureMainCamera() override;

  /// Override this to modify the default window creation behavior. Called by CreateActors().
  virtual WUniquePtr<WWindow> CreateMainWindow();

  /// Override this to modify the default output target creation behavior. Called by CreateActors().
  virtual WUniquePtr<WWindowOutputTargetGAL> CreateMainOutputTarget(WWindow* pMainWindow);

  /// Creates a default render view. Unless overridden, OnActivation() will do this for the main window.
  virtual void SetupMainView(WGALSwapChainHandle hSwapChain, WSizeU32 viewportSize);

  /// Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Called by CreateActors() with the result of CreateMainWindow().
  virtual void ConfigureMainWindowInputDevices(WWindow* pWindow);

  /// Returns the path to the scene file and the corresponding preload collection to load at startup.
  ///
  /// By default this is taken from the command line '-scene' option.
  /// Override this function to define a custom startup scene (e.g. for the main menu) or load a saved state.
  virtual void GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection);

  /// Called by SwitchToLoadingScreen() to set up a new loading screen world.
  ///
  /// A loading screen uses a separate WWorld. It can be fully set up in code or loaded from disk,
  /// but it should be very light-weight, so that it is quick to set up.
  virtual WUniquePtr<WWorld> CreateLoadingScreenWorld(WStringView sTargetSceneFile);

  /// If a scene is being loaded in the background, this advanced the loading.
  ///
  /// Upon success or failure, executes either of these:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  void UpdateBackgroundSceneLoading();

  /// Called by `UpdateBackgroundSceneLoading()` when a scene is finished loading.
  ///
  /// May switch to the scene immediately or wait, for example for a user to confirm.
  virtual void OnBackgroundSceneLoadingFinished(WUniquePtr<WWorld>&& pWorld);

  /// Called by `UpdateBackgroundSceneLoading()` when a scene failed to load.
  virtual void OnBackgroundSceneLoadingFailed(WStringView sReason);

  /// Called by `CancelBackgroundSceneLoading()` when scene loading gets canceled.
  virtual void OnBackgroundSceneLoadingCanceled();

  virtual void OnWindowEvent(const WWindowEvent& e);

  static WGameState* s_pActiveGameState;

  WViewHandle m_hMainView;

  WWorld* m_pMainWorld = nullptr;

  WCamera m_MainCamera;
  WUniquePtr<WDummyXR> m_pDummyXR;
  bool m_bStateWantsToQuit = false;
  bool m_bXREnabled = false;

  bool m_bTransitionWhenReady = false;
  WUniquePtr<WSceneLoadUtility> m_pBackgroundSceneLoad;
  WUniquePtr<WWorld> m_pLoadingScreenWorld;
  WUniquePtr<WWorld> m_pLoadedWorld;

  WString m_sTargetSceneSpawnPoint;
  WTransform m_TargetSceneSpawnOffset = WTransform::MakeIdentity();
};
