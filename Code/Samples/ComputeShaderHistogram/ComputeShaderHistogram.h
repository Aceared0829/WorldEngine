#pragma once

#include <Foundation/Basics.h>

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)
#  include <Foundation/IO/DirectoryWatcher.h>
#endif
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class WWindow;
#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)
class WDirectoryWatcher;
#endif

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class WComputeShaderHistogramApp : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  WComputeShaderHistogramApp();
  ~WComputeShaderHistogramApp();

  virtual void Run() override;

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void CreateHistogramQuad();

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)
  void OnFileChanged(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type);
#endif

  WGALTextureHandle m_hScreenTexture;
  WGALRenderTargetViewHandle m_hScreenRTV;

  // Could use buffer, but access and organisation with texture is more straight forward.
  WGALTextureHandle m_hHistogramTexture;

  WWindowBase* m_pWindow = nullptr;
  WGALSwapChainHandle m_hSwapChain;

  WShaderResourceHandle m_hRenderScreenShader;
  WShaderResourceHandle m_hDisplayScreenShader;
  WShaderResourceHandle m_hClearHistogramShader;
  WShaderResourceHandle m_hComputeHistogramShader;
  WShaderResourceHandle m_hDisplayHistogramShader;

  WMeshBufferResourceHandle m_hHistogramQuadMeshBuffer;

#if W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)
  WUniquePtr<WDirectoryWatcher> m_pDirectoryWatcher;
#endif
  bool m_bStuffChanged = false;
};
