#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Containers/IdTable.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Memory/CommonAllocators.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Pipeline/Declarations.h>

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;

struct ImGuiContext;
struct WGameApplicationExecutionEvent;

using WImguiConfigFontCallback = WDelegate<void(ImFontAtlas&)>;
using WImguiConfigStyleCallback = WDelegate<void(ImGuiStyle&)>;

/// Singleton class through which one can control the third-party library 'Dear Imgui'
///
/// Instance has to be manually created and destroyed. Do this for example in WGameState::OnActivation()
/// and WGameState::OnDeactivation().
/// You need to call SetCurrentContextForView before you can use the Imgui functions directly.
/// E.g. 'ImGui::Text("Hello, world!");'
/// To prevent Imgui from using mouse and keyboard input (but still do rendering) use SetPassInputToImgui().
/// To prevent your app from using mouse and keyboard input when Imgui has focus, query WantsInput().
///
/// \note Don't forget that to see the GUI on screen, your render pipeline must contain an WImguiExtractor
/// and you need to have an WImguiRenderer set (typically on an WSimpleRenderPass).
class W_GAMEENGINE_DLL WImgui
{
  W_DECLARE_SINGLETON(WImgui);

public:
  WImgui(WImguiConfigFontCallback configFontCallback = WImguiConfigFontCallback(),
    WImguiConfigStyleCallback configStyleCallback = WImguiConfigStyleCallback());
  ~WImgui();

  /// Sets the ImGui context for the given view
  void SetCurrentContextForView(const WViewHandle& hView);

  /// Returns the value that was passed to BeginFrame(). Useful for positioning UI elements.
  WSizeU32 GetCurrentWindowResolution() const { return m_CurrentWindowResolution; }

  /// When this is disabled, the GUI will be rendered, but it will not react to any input.
  ///
  /// Useful if something else shall get exclusive input.
  /// Be aware that this is global state, that affects ALL ImGui elements for the entire frame.
  void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }
  bool GetPassInputToImgui() const { return m_bPassInputToImgui; }

  /// If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  bool WantsInput() const { return m_bImguiWantsInput; }

  /// Returns the shared font atlas
  ImFontAtlas& GetFontAtlas() { return *m_pSharedFontAtlas; }

  /// Registers a texture resource and returns an ImTextureID that can be used with raw ImGui image calls or passed to RegisterImage(). The texture stays registered until UnregisterResource() is called.
  ImTextureID RegisterTexture(const WTexture2DResourceHandle& hTexture);

  /// \copydoc RegisterTexture(const WTexture2DResourceHandle&)
  ImTextureID RegisterTexture(WGALTextureHandle hTexture);

  /// Registers a material resource and returns an ImTextureID. Works like RegisterTexture() but renders using the full material rather than just a texture.
  ImTextureID RegisterMaterial(const WMaterialResourceHandle& hMaterial);

  /// Removes a previously registered texture or material. The ImTextureID must not be used afterwards.
  void UnregisterResource(ImTextureID id);

  struct Image
  {
    ImTextureID m_Id;
    WVec2 m_UV0;
    WVec2 m_UV1;
  };

  /// Associates a named image with a registered texture and UV coordinates. This allows the AddImage() and AddImageButton() convenience functions to look up the image by name instead of requiring the caller to pass the ImTextureID and UVs every time. Call RegisterTexture() first to obtain the texture ID.
  void RegisterImage(WTempHashedString sImgId, ImTextureID texId, const WVec2& vUv0, const WVec2& vUv1);

  /// Renders a clickable image button using a previously registered image name.
  bool AddImageButton(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, const WColor& backgroundColor = WColor::MakeZero(), const WColor& tintColor = WColor::White) const;

  /// Renders an image using a previously registered image name.
  void AddImage(WTempHashedString sImgId, const WVec2& vImageSize, const WColor& tintColor = WColor::White, const WColor& borderColor = WColor::MakeZero()) const;

  /// Like AddImageButton(), but draws a colored overlay from \a fProgress (0 to 1) to the right edge, which can be used to indicate a loading progress.
  bool AddImageButtonWithProgress(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, float fProgress, const WColor& overlayColor, const WColor& tintColor = WColor::White) const;

  /// Like AddImage(), but draws a colored overlay from \a fProgress (0 to 1) to the right edge, which can be used to indicate a loading progress.
  void AddImageWithProgress(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, float fProgress, const WColor& overlayColor, const WColor& tintColor = WColor::White) const;

private:
  friend class WImguiExtractor;
  friend class WImguiRenderer;

  using WImGuiTextureIdData = WGenericId<16, 16>;

  struct WImGuiTextureRegistration
  {
    enum class Type : WUInt8
    {
      Texture2D,
      GALTexture,
      Material,
    };

    Type m_Type = Type::Texture2D;
    WTexture2DResourceHandle m_hTexture2D;
    WGALTextureHandle m_hGALTexture;
    WMaterialResourceHandle m_hMaterial;
  };

  void Startup(WImguiConfigFontCallback configFontCallback);
  void Shutdown();

  ImGuiContext* CreateContext();
  void BeginFrame(const WViewHandle& hView);
  void GameApplicationEventHandler(const WGameApplicationExecutionEvent& e);

  WProxyAllocator m_Allocator;

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  WSizeU32 m_CurrentWindowResolution;
  WIdTable<WImGuiTextureIdData, WImGuiTextureRegistration> m_RegisteredTextures;

  WImguiConfigStyleCallback m_ConfigStyleCallback;

  WUniquePtr<ImFontAtlas> m_pSharedFontAtlas;

  struct Context
  {
    ImGuiContext* m_pImGuiContext = nullptr;
    WUInt64 m_uiFrameBeginCounter = -1;
    WUInt64 m_uiFrameRenderCounter = -1;
  };

  WMutex m_ViewToContextTableMutex;
  WHashTable<WViewHandle, Context> m_ViewToContextTable;
  WHashTable<WTempHashedString, Image> m_Images;
  WCVarFloat* m_pTextScaleCVar = nullptr;
};

#endif
