#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/Progress.h>
#include <GameEngine/GameEngineDLL.h>

using WCollectionResourceHandle = WTypedResourceHandle<class WCollectionResource>;

/// This class allows to load a scene in the background and switch to it, once loading has finished.
class W_GAMEENGINE_DLL WSceneLoadUtility
{
  W_DISALLOW_COPY_AND_ASSIGN(WSceneLoadUtility);

public:
  WSceneLoadUtility();
  ~WSceneLoadUtility();

  /// Redirects a scene path to the actual binary scene file, if necessary.
  static WStatus FindRedirectedSceneFile(WStringBuilder& ref_sFinalPath, WStringView sSceneFile);

  /// Loads a scene immediately into the given target world.
  ///
  /// Doesn't clear the world beforehand.
  /// Does call FindRedirectedSceneFile() on the scene path first.
  static WStatus LoadSceneImmediate(WWorld& inout_targetWorld, WStringView sSceneFile);

  enum class LoadingState
  {
    NotStarted,
    Ongoing,
    FinishedSuccessfully,
    FinishedAndRetrieved, ///< Loading succeeded and someone already called RetrieveLoadedScene()
    Failed,
  };

  /// Returns whether loading is still ongoing or finished.
  LoadingState GetLoadingState() const { return m_LoadingState; }

  /// Returns a loading progress value in 0 to 1 range.
  float GetLoadingProgress() const { return m_fLoadingProgress; }

  /// In case loading failed, this returns what went wrong.
  WStringView GetLoadingFailureReason() const { return m_sFailureReason; }

  /// Starts loading a scene. If provided, the assets in the collection are loaded first and then the scene is instantiated.
  ///
  /// Using a collection will make loading in the background much smoother. Without it, most assets will be loaded once the scene gets updated
  /// for the first time, resulting in very long delays.
  void StartSceneLoading(WStringView sSceneFile, WStringView sPreloadCollectionFile);

  /// This has to be called periodically (usually once per frame) to progress the scene loading.
  ///
  /// Call GetLoadingState() afterwards to check whether loading has finished or failed.
  void TickSceneLoading();

  /// Once loading is finished successfully, call this to take ownership of the loaded scene.
  ///
  /// Afterwards there is no point in keeping the WSceneLoadUtility around anymore and it should be deleted.
  WUniquePtr<WWorld> RetrieveLoadedScene();

  /// Returns the path to the scene file as it was originally requested.
  WStringView GetRequestedScene() const { return m_sRequestedFile; }

  /// Returns the path to the scene file after it was redirected.
  WStringView GetRedirectedScene() const { return m_sRedirectedFile; }

private:
  void LoadingFailed(const WFormatString& reason);

  LoadingState m_LoadingState = LoadingState::NotStarted;
  float m_fLoadingProgress = 0.0f;
  WString m_sFailureReason;

  WString m_sRequestedFile;
  WString m_sRedirectedFile;
  WCollectionResourceHandle m_hPreloadCollection;
  WFileReader m_FileReader;
  WWorldReader m_WorldReader;
  WUniquePtr<WWorld> m_pWorld;
  WUniquePtr<WWorldReader::InstantiationContextBase> m_pInstantiationContext;
  WProgress m_InstantiationProgress;
};
