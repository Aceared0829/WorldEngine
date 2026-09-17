#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WGALDeviceEvent;
class WRenderGraph;

using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;
using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;
using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

/// Renders the custom mouse cursor that was set through WInputManager::SetMouseCursor().
///
/// The cursor identifier (WMouseCursorDesc::m_sCursor) may be the GUID or path of either
///  * a 2D texture asset - it is then rendered with the built-in Shaders/MouseCursor/MouseCursor.WShader, or
///  * a material asset - which allows for arbitrary custom cursor effects.
///
/// The cursor is drawn as a single quad that is generated from SV_VertexID, so it needs neither a
/// vertex nor an index buffer. A custom cursor material has to follow these rules:
///  * Its vertex shader has to `#include <Shaders/MouseCursor/MouseCursorCommon.h>` and return
///    `WMouseCursorVertex(VertexID)`, or do the equivalent math itself, using WMouseCursorConstants.
///  * It must set up its own render state (no depth test, no culling, alpha blending).
///  * It must not use constant buffer slot 3 in the W_GAL_BIND_GROUP_FRAME bind group,
///    that one holds WMouseCursorConstants.
///  * It must not declare permutation variables and it must not read WGlobalConstants,
///    except for the time constants. Outside of a render pipeline neither is in a defined state.
class W_GAMEENGINE_DLL WMouseCursorRenderer
{
  W_DECLARE_SINGLETON(WMouseCursorRenderer);

public:
  WMouseCursorRenderer();
  ~WMouseCursorRenderer();

  /// Sets which swap-chain to render the cursor into. Without this, nothing is rendered.
  ///
  /// WGameState::SetupMainView() sets this up for the main window automatically.
  void SetSwapChain(WGALSwapChainHandle hSwapChain) { m_hSwapChain = hSwapChain; }

private:
  struct ResolvedCursor
  {
    WMaterialResourceHandle m_hMaterial;
    WTexture2DResourceHandle m_hTexture;
  };

  void OnGALDeviceEvent(const WGALDeviceEvent& e);
  void ResolveCursor(WStringView sIdentifier);
  WResult EnsureGpuResourcesExist();
  void ReleaseGpuResources();

  WGALSwapChainHandle m_hSwapChain;

  WUInt32 m_uiLastIdentifierChangeCounter = 0;
  bool m_bIdentifierResolved = false;
  WHashTable<WString, ResolvedCursor> m_ResolvedCursors;
  ResolvedCursor m_Current;

  WShaderResourceHandle m_hDefaultShader;
  WConstantBufferStorageHandle m_hConstantBuffer;
  WSharedPtr<WRenderGraph> m_pRenderGraph;
};
