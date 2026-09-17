#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

/// [internal] Worker task for loading resources (typically from disk).
class W_CORE_DLL WResourceManagerWorkerDataLoad final : public WTask
{
public:
  ~WResourceManagerWorkerDataLoad();

private:
  friend class WResourceManager;
  friend class WResourceManagerState;

  WResourceManagerWorkerDataLoad();

  virtual void Execute() override;
};

/// [internal] Worker task for uploading resource data.
/// Depending on the resource type, this may get scheduled to run on the main thread or on any thread.
class W_CORE_DLL WResourceManagerWorkerUpdateContent final : public WTask
{
public:
  ~WResourceManagerWorkerUpdateContent();

  WResourceLoadData m_LoaderData;
  WResource* m_pResourceToLoad = nullptr;
  WResourceTypeLoader* m_pLoader = nullptr;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  WUniquePtr<WResourceTypeLoader> m_pCustomLoader;

private:
  friend class WResourceManager;
  friend class WResourceManagerState;
  friend class WResourceManagerWorkerDataLoad;
  WResourceManagerWorkerUpdateContent();

  virtual void Execute() override;
};
