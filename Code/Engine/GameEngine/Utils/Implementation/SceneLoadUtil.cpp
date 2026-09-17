#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

// preloading assets is considered to be the vast majority of scene loading
constexpr float fCollectionPreloadPiece = 0.9f;

WSceneLoadUtility::WSceneLoadUtility() = default;
WSceneLoadUtility::~WSceneLoadUtility() = default;

WStatus WSceneLoadUtility::FindRedirectedSceneFile(WStringBuilder& ref_sFinalPath, WStringView sSceneFile)
{
  ref_sFinalPath = sSceneFile;

  if (ref_sFinalPath.IsEmpty())
  {
    return WStatus("No scene file specified.");
  }

  if (ref_sFinalPath.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    ref_sFinalPath.MakeRelativeTo(WGameApplication::GetGameApplicationInstance()->GetAppProjectPath()).IgnoreResult();
  }

  if (ref_sFinalPath.HasExtension("WScene") || ref_sFinalPath.HasExtension("WPrefab"))
  {
    if (ref_sFinalPath.IsAbsolutePath())
    {
      if (WFileSystem::ResolvePath(ref_sFinalPath, nullptr, &ref_sFinalPath).Failed())
      {
        return WStatus(WFmt("Scene path is not located in any data directory: '{}'", ref_sFinalPath));
      }
    }

    // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
    ref_sFinalPath.Prepend("AssetCache/Common/");

    if (ref_sFinalPath.HasExtension("WScene"))
      ref_sFinalPath.ChangeFileExtension("WBinScene");
    else
      ref_sFinalPath.ChangeFileExtension("WBinPrefab");
  }

  return W_SUCCESS;
}

WStatus WSceneLoadUtility::LoadSceneImmediate(WWorld& inout_targetWorld, WStringView sSceneFile)
{
  WStringBuilder ref_sFinalPath;
  W_SUCCEED_OR_RETURN(FindRedirectedSceneFile(ref_sFinalPath, sSceneFile));

  WFileReader fileReader;

  if (fileReader.Open(ref_sFinalPath).Failed())
    return WStatus("Failed to open the file.");

  // Read and skip the asset file header
  WAssetFileHeader header;
  header.Read(fileReader).AssertSuccess();

  char szSceneTag[16];
  fileReader.ReadBytes(szSceneTag, sizeof(char) * 16);

  if (!WStringUtils::IsEqualN(szSceneTag, "[WEBinaryScene]", 16))
    return WStatus("The given file isn't an object-graph file.");

  WWorldReader worldReader;
  if (worldReader.ReadWorldDescription(fileReader).Failed())
    return WStatus("Error reading world description.");

  worldReader.InstantiateWorld(inout_targetWorld, nullptr);

  return W_SUCCESS;
}

void WSceneLoadUtility::StartSceneLoading(WStringView sSceneFile, WStringView sPreloadCollectionFile)
{
  W_ASSERT_DEV(m_LoadingState == LoadingState::NotStarted, "Can't reuse an WSceneLoadUtility.");

  W_LOG_BLOCK("StartSceneLoading");

  WLog::Info("Loading scene '{}'.", sSceneFile);

  m_LoadingState = LoadingState::Ongoing;

  m_sRequestedFile = sSceneFile;

  WStringBuilder ref_sFinalPath;
  auto res = FindRedirectedSceneFile(ref_sFinalPath, sSceneFile);

  if (res.Failed())
  {
    LoadingFailed(res.GetMessageString().GetView());
    return;
  }

  if (ref_sFinalPath != sSceneFile)
  {
    WLog::Dev("Redirecting scene file from '{}' to '{}'", sSceneFile, ref_sFinalPath);
  }

  m_sRedirectedFile = ref_sFinalPath;

  if (!sPreloadCollectionFile.IsEmpty())
  {
    m_hPreloadCollection = WResourceManager::LoadResource<WCollectionResource>(WString(sPreloadCollectionFile));
  }
}

WUniquePtr<WWorld> WSceneLoadUtility::RetrieveLoadedScene()
{
  W_ASSERT_DEV(m_LoadingState == LoadingState::FinishedSuccessfully, "Can't retrieve a scene when loading hasn't finished successfully.");

  m_LoadingState = LoadingState::FinishedAndRetrieved;

  m_pWorld->SetWorldSimulationEnabled(true);

  return std::move(m_pWorld);
}

void WSceneLoadUtility::LoadingFailed(const WFormatString& reason)
{
  W_ASSERT_DEV(m_LoadingState == LoadingState::Ongoing, "Invalid loading state");
  m_LoadingState = LoadingState::Failed;

  WStringBuilder tmp;
  m_sFailureReason = reason.GetText(tmp);
}

void WSceneLoadUtility::TickSceneLoading()
{
  switch (m_LoadingState)
  {
    case LoadingState::FinishedSuccessfully:
    case LoadingState::Failed:
      return;

    default:
      break;
  }

  W_PROFILE_SCOPE("TickSceneLoading");

  // update our current loading progress
  {
    m_fLoadingProgress = fCollectionPreloadPiece;

    // if we have a collection, preload that first
    if (m_hPreloadCollection.IsValid())
    {
      m_fLoadingProgress = 0.0f;

      WResourceLock<WCollectionResource> pCollection(m_hPreloadCollection, WResourceAcquireMode::AllowLoadingFallback_NeverFail);

      if (pCollection.GetAcquireResult() == WResourceAcquireResult::Final)
      {
        if (pCollection->PreloadResources())
        {
          W_REPORT_FAILURE("Failed to start preloading all resources.");
        }

        float progress = 0.0f;
        if (pCollection->IsLoadingFinished(&progress))
        {
          m_fLoadingProgress = fCollectionPreloadPiece;
        }
        else
        {
          m_fLoadingProgress = progress * fCollectionPreloadPiece;
        }
      }
    }

    // if preloading the collection is finished (or we just don't have one) add the world instantiation progress
    if (m_fLoadingProgress == fCollectionPreloadPiece)
    {
      m_fLoadingProgress += m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
    }
  }

  // as long as we are still pre-loading assets from the collection, don't do anything else
  if (m_fLoadingProgress < fCollectionPreloadPiece)
    return;

  // if we haven't created a world yet, do so now, and set up an instantiation context
  if (m_pWorld == nullptr)
  {
    W_LOG_BLOCK("LoadObjectGraph", m_sRedirectedFile);

    WWorldDesc desc(m_sRedirectedFile);
    m_pWorld = W_DEFAULT_NEW(WWorld, desc);
    m_pWorld->SetWorldSimulationEnabled(false);

    W_LOCK(m_pWorld->GetWriteMarker());

    if (m_FileReader.Open(m_sRedirectedFile).Failed())
    {
      LoadingFailed("Failed to open the file.");
      return;
    }
    else
    {
      // Read and skip the asset file header
      WAssetFileHeader header;
      header.Read(m_FileReader).AssertSuccess();

      char szSceneTag[16];
      m_FileReader.ReadBytes(szSceneTag, sizeof(char) * 16);

      if (!WStringUtils::IsEqualN(szSceneTag, "[WEBinaryScene]", 16))
      {
        LoadingFailed("The given file isn't an object-graph file.");
        return;
      }

      if (m_WorldReader.ReadWorldDescription(m_FileReader).Failed())
      {
        LoadingFailed("Error reading world description.");
        return;
      }

      // TODO: make frame time configurable ?
      m_pInstantiationContext = m_WorldReader.InstantiateWorld(*m_pWorld, nullptr, WTime::MakeFromMilliseconds(1), &m_InstantiationProgress);
    }
  }
  else if (m_pInstantiationContext)
  {
    WWorldReader::InstantiationContextBase::StepResult res = m_pInstantiationContext->Step();

    if (res == WWorldReader::InstantiationContextBase::StepResult::ContinueNextFrame)
    {
      // TODO: can we finish the world instantiation without updating the entire world?
      // E.g. only finish component instantiation?
      // also we may want to step the world only with a very small (and fixed!) time-step

      W_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();
    }
    else if (res == WWorldReader::InstantiationContextBase::StepResult::Finished)
    {
      // TODO: ticking twice seems to fix some Jolt physics issues
      W_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();

      m_pInstantiationContext = nullptr;
      m_LoadingState = LoadingState::FinishedSuccessfully;
    }

    m_fLoadingProgress = fCollectionPreloadPiece + m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
  }
  else
  {
    W_REPORT_FAILURE("Invalid code path.");
  }
}
