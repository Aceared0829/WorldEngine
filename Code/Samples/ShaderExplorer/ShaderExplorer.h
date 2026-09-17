#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// define this to force usage of fileserve functionality
// #define USE_FILESERVE W_ON

// to use fileserve, run the WFileServe application with a command line that tells it where the ":project"
// data directory is located on the PC, for example:
//
// WFileServe.exe -fs_start -specialdirs project "C:\W\Data\Samples\ShaderExplorer"

#if !defined(USE_FILESERVE)

#  if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && W_DISABLED(W_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
// on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
// Right now, the only platform that hits this code path is Android, but right now, port forwarding for enet does not seem to work in the emulator so for now this is disabled until this can be debugged further.
// #    define USE_FILESERVE W_ON
#    define USE_FILESERVE W_OFF
#  else
#    define USE_FILESERVE W_OFF
#  endif

#endif


#if W_DISABLED(USE_FILESERVE) && W_ENABLED(W_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER W_ON
#else
#  define USE_DIRECTORY_WATCHER W_OFF
#endif

class WWindow;
class WCamera;
class WGALDevice;
class WDirectoryWatcher;
class WVirtualThumbStick;

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class WShaderExplorerApp : public WApplication
{
public:
  using SUPER = WApplication;

  WShaderExplorerApp();

  virtual void Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void CreateScreenQuad();

#if W_ENABLED(USE_DIRECTORY_WATCHER)
  WUniquePtr<WDirectoryWatcher> m_pDirectoryWatcher;
  void OnFileChanged(WStringView sFilename, WDirectoryWatcherAction action, WDirectoryWatcherType type);
#endif

  WWindow* m_pWindow = nullptr;
  WGALDevice* m_pDevice = nullptr;

  WGALSwapChainHandle m_hSwapChain;
  WGALTextureHandle m_hDepthStencilTexture;

  WMaterialResourceHandle m_hMaterial;
  WMeshBufferResourceHandle m_hQuadMeshBuffer;

  WUniquePtr<WCamera> m_pCamera;
  WUniquePtr<WVirtualThumbStick> m_pLeftStick;
  WUniquePtr<WVirtualThumbStick> m_pRightStick;

  bool m_bStuffChanged;
};
