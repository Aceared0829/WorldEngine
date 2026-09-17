#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

class WCameraComponent;

/// WFallbackGameState is an WGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 'Fallback' in DeterminePriority() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for WEditor and WPlayer which make sure that a world already exists.
class W_GAMEENGINE_DLL WFallbackGameState : public WGameState
{
  W_ADD_DYNAMIC_REFLECTION(WFallbackGameState, WGameState)

public:
  WFallbackGameState();

  virtual void ProcessInput() override;

  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;

  /// Reports true for WFallbackGameState only, not for derived types.
  virtual bool IsFallbackGameState() const override;

protected:
  /// Called by SwitchToLoadingScreen() to setup a new world that acts as the loading screen while waiting for another scene to finish loading.
  virtual void ConfigureInputActions() override;
  virtual WResult SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset) override;

  virtual const WCameraComponent* FindActiveCameraComponent();

  WInt32 m_iActiveCameraComponentIndex = -3;

  //////////////////////////////////////////////////////////////////////////

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  State m_State = State::Ok;
  bool m_bShowMenu = false;
  bool m_bAllowOpenMenu = true;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool m_bCheckedForScenes = false;
  WDynamicArray<WString> m_AvailableScenes;
  WUInt32 m_uiSelectedScene = 0;
  WString m_sTitleOfScene;

  virtual void OnBackgroundSceneLoadingFinished(WUniquePtr<WWorld>&& pWorld) override;
  virtual void OnBackgroundSceneLoadingFailed(WStringView sReason) override;

  virtual void ConfigureMainCamera() override;
};
