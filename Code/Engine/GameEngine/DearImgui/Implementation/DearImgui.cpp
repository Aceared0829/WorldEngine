#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  define IMGUI_DEFINE_MATH_OPERATORS
#  include <Imgui/imgui.h>

#  include <Core/Input/InputManager.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Time/Clock.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/GameApplication/GameApplication.h>
#  include <Imgui/imgui_internal.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Textures/Texture2DResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GameEngine, ImGui)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (WImgui::GetSingleton() != nullptr)
    {
      WImgui* pImgui = WImgui::GetSingleton();
      W_DEFAULT_DELETE(pImgui);
    }
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  void* WImguiAllocate(size_t uiSize, void* pUserData)
  {
    WAllocator* pAllocator = static_cast<WAllocator*>(pUserData);
    return pAllocator->Allocate(uiSize, W_ALIGNMENT_MINIMUM);
  }

  void WImguiDeallocate(void* pPtr, void* pUserData)
  {
    if (pPtr != nullptr)
    {
      WAllocator* pAllocator = static_cast<WAllocator*>(pUserData);
      pAllocator->Deallocate(pPtr);
    }
  }
} // namespace

W_IMPLEMENT_SINGLETON(WImgui);

WImgui::WImgui(WImguiConfigFontCallback configFontCallback, WImguiConfigStyleCallback configStyleCallback)
  : m_SingletonRegistrar(this)
  , m_Allocator("ImGui", WFoundation::GetDefaultAllocator())
  , m_ConfigStyleCallback(configStyleCallback)
{
  Startup(configFontCallback);
}

WImgui::~WImgui()
{
  Shutdown();
}

void WImgui::SetCurrentContextForView(const WViewHandle& hView)
{
  W_LOCK(m_ViewToContextTableMutex);

  Context& context = m_ViewToContextTable[hView];
  if (context.m_pImGuiContext == nullptr)
  {
    context.m_pImGuiContext = CreateContext();
  }

  ImGui::SetCurrentContext(context.m_pImGuiContext);

  WUInt64 uiCurrentFrameCounter = WRenderWorld::GetFrameCounter();
  if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
  {
    // Last frame was not rendered. This can happen if a render pipeline with dear imgui renderer is used.
    if (context.m_uiFrameRenderCounter != context.m_uiFrameBeginCounter)
    {
      ImGui::EndFrame();
    }

    BeginFrame(hView);
    context.m_uiFrameBeginCounter = uiCurrentFrameCounter;
  }
}

ImTextureID WImgui::RegisterTexture(const WTexture2DResourceHandle& hTexture)
{
  WImGuiTextureRegistration reg;
  reg.m_Type = WImGuiTextureRegistration::Type::Texture2D;
  reg.m_hTexture2D = hTexture;
  WImGuiTextureIdData handle = m_RegisteredTextures.Insert(reg);
  return *reinterpret_cast<ImTextureID*>(&handle);
}

ImTextureID WImgui::RegisterTexture(WGALTextureHandle hTexture)
{
  WImGuiTextureRegistration reg;
  reg.m_Type = WImGuiTextureRegistration::Type::GALTexture;
  reg.m_hGALTexture = hTexture;
  WImGuiTextureIdData handle = m_RegisteredTextures.Insert(reg);
  return *reinterpret_cast<ImTextureID*>(&handle);
}

ImTextureID WImgui::RegisterMaterial(const WMaterialResourceHandle& hMaterial)
{
  WImGuiTextureRegistration reg;
  reg.m_Type = WImGuiTextureRegistration::Type::Material;
  reg.m_hMaterial = hMaterial;
  WImGuiTextureIdData handle = m_RegisteredTextures.Insert(reg);
  return *reinterpret_cast<ImTextureID*>(&handle);
}

void WImgui::UnregisterResource(ImTextureID id)
{
  WImGuiTextureIdData handle = *reinterpret_cast<WImGuiTextureIdData*>(&id);
  if (!m_RegisteredTextures.Contains(handle))
    return;

  m_RegisteredTextures.Remove(handle);
}

void WImgui::RegisterImage(WTempHashedString sName, ImTextureID texId, const WVec2& vUv0, const WVec2& vUv1)
{
  auto& img = m_Images[sName];
  img.m_Id = texId;
  img.m_UV0 = vUv0;
  img.m_UV1 = vUv1;
}

bool WImgui::AddImageButton(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, const WColor& backgroundColor, const WColor& tintColor) const
{
  Image* pImg;
  if (!m_Images.TryGetValue(sImgId, pImg))
  {
    W_ASSERT_DEBUG(false, "Unknown image identifier");
    return false;
  }

  return ImGui::ImageButton(szImguiID, pImg->m_Id, reinterpret_cast<const ImVec2&>(vImageSize), reinterpret_cast<const ImVec2&>(pImg->m_UV0), reinterpret_cast<const ImVec2&>(pImg->m_UV1), reinterpret_cast<const ImVec4&>(backgroundColor), reinterpret_cast<const ImVec4&>(tintColor));
}


void WImgui::AddImage(WTempHashedString sImgId, const WVec2& vImageSize, const WColor& tintColor, const WColor& borderColor) const
{
  Image* pImg;
  if (!m_Images.TryGetValue(sImgId, pImg))
  {
    W_ASSERT_DEBUG(false, "Unknown image identifier");
    return;
  }

  return ImGui::Image(pImg->m_Id, reinterpret_cast<const ImVec2&>(vImageSize), reinterpret_cast<const ImVec2&>(pImg->m_UV0), reinterpret_cast<const ImVec2&>(pImg->m_UV1), reinterpret_cast<const ImVec4&>(tintColor), reinterpret_cast<const ImVec4&>(borderColor));
}


bool WImgui::AddImageButtonWithProgress(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, float fProgress, const WColor& overlayColor, const WColor& tintColor) const
{
  Image* pImg;
  if (!m_Images.TryGetValue(sImgId, pImg))
  {
    W_ASSERT_DEBUG(false, "Unknown image identifier");
    return false;
  }

  ImGuiContext& g = *ImGui::GetCurrentContext();
  ImGuiWindow* window = g.CurrentWindow;
  if (window->SkipItems)
    return false;

  ImTextureID user_texture_id = pImg->m_Id;
  const ImVec2 image_size = reinterpret_cast<const ImVec2&>(vImageSize);
  const ImVec2 vUv0 = reinterpret_cast<const ImVec2&>(pImg->m_UV0);
  const ImVec2 vUv1 = reinterpret_cast<const ImVec2&>(pImg->m_UV1);
  ImGuiID id = window->GetID(szImguiID);

  const ImVec2 padding = g.Style.FramePadding;
  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size + padding * 2.0f);
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return false;

  const ImGuiButtonFlags flags = 0;

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

  // Render
  const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered
                                                                                           : ImGuiCol_Button);
  ImGui::RenderNavHighlight(bb, id);
  ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));

  // if (bg_col.w > 0.0f)
  //   window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, ImGui::GetColorU32(bg_col));

  const ImVec4 tintCol = reinterpret_cast<const ImVec4&>(tintColor);
  window->DrawList->AddImage(user_texture_id, bb.Min + padding, bb.Max - padding, vUv0, vUv1, ImGui::GetColorU32(tintCol));

  ImVec2 min = bb.Min;
  ImVec2 max = bb.Max;

  min.x = WMath::Lerp(min.x, max.x, fProgress);

  const ImVec4 overlayCol = reinterpret_cast<const ImVec4&>(overlayColor);
  window->DrawList->AddRectFilled(min, max, ImGui::GetColorU32(overlayCol));

  return pressed;
}

void WImgui::AddImageWithProgress(WTempHashedString sImgId, const char* szImguiID, const WVec2& vImageSize, float fProgress, const WColor& overlayColor, const WColor& tintColor) const
{
  Image* pImg;
  if (!m_Images.TryGetValue(sImgId, pImg))
  {
    W_ASSERT_DEBUG(false, "Unknown image identifier");
    return;
  }

  ImGuiContext& g = *ImGui::GetCurrentContext();
  ImGuiWindow* window = g.CurrentWindow;
  if (window->SkipItems)
    return;

  ImTextureID user_texture_id = pImg->m_Id;
  const ImVec2 image_size = reinterpret_cast<const ImVec2&>(vImageSize);
  const ImVec2 vUv0 = reinterpret_cast<const ImVec2&>(pImg->m_UV0);
  const ImVec2 vUv1 = reinterpret_cast<const ImVec2&>(pImg->m_UV1);
  ImGuiID id = window->GetID(szImguiID);

  const ImVec2 padding = g.Style.FramePadding;
  const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size + padding * 2.0f);
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, id))
    return;

  // Render
  const ImU32 col = ImGui::GetColorU32(ImGuiCol_Button);
  ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));

  const ImVec4 tintCol = reinterpret_cast<const ImVec4&>(tintColor);
  window->DrawList->AddImage(user_texture_id, bb.Min + padding, bb.Max - padding, vUv0, vUv1, ImGui::GetColorU32(tintCol));

  ImVec2 min = bb.Min;
  ImVec2 max = bb.Max;

  min.x = WMath::Lerp(min.x, max.x, fProgress);

  const ImVec4 overlayCol = reinterpret_cast<const ImVec4&>(overlayColor);
  window->DrawList->AddRectFilled(min, max, ImGui::GetColorU32(overlayCol));
}

void WImgui::Startup(WImguiConfigFontCallback configFontCallback)
{
  ImGui::SetAllocatorFunctions(&WImguiAllocate, &WImguiDeallocate, &m_Allocator);

  m_pSharedFontAtlas = W_DEFAULT_NEW(ImFontAtlas);

  if (configFontCallback.IsValid())
  {
    configFontCallback(*m_pSharedFontAtlas);
  }

  unsigned char* pixels;
  int width, height;
  m_pSharedFontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height); // Load as RGBA 32-bits (75% of the memory is wasted, but default font
                                                                    // is so small) because it is more likely to be compatible with user's
                                                                    // existing shaders. If your ImTextureId represent a higher-level
                                                                    // concept than just a GL texture id, consider calling
                                                                    // GetTexDataAsAlpha8() instead to save on GPU memory.

  WTexture2DResourceHandle hFont = WResourceManager::GetExistingResource<WTexture2DResource>("ImguiFont");

  if (!hFont.IsValid())
  {
    WGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = WMakeByteBlobPtr(pixels, WUInt32(width * height * 4));
    memoryDesc.m_uiRowPitch = width * 4;
    memoryDesc.m_uiSlicePitch = width * height * 4;

    WTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = width;
    desc.m_DescGAL.m_uiHeight = height;
    desc.m_DescGAL.m_Format = WGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = WMakeArrayPtr(&memoryDesc, 1);

    hFont = WResourceManager::GetOrCreateResource<WTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_pSharedFontAtlas->TexID = RegisterTexture(hFont);

  WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(WMakeDelegate(&WImgui::GameApplicationEventHandler, this));
}

void WImgui::Shutdown()
{
  if (m_pSharedFontAtlas)
  {
    UnregisterResource(m_pSharedFontAtlas->TexID);
  }
  m_pSharedFontAtlas = nullptr;

  WGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(WMakeDelegate(&WImgui::GameApplicationEventHandler, this));
  W_ASSERT_DEV(m_RegisteredTextures.IsEmpty(), "Not all registered textures were unregistered. You need to call 'UnregisterResource' before shutdown.");
  m_RegisteredTextures.Clear();



  for (auto it = m_ViewToContextTable.GetIterator(); it.IsValid(); ++it)
  {
    Context& context = it.Value();
    ImGui::DestroyContext(context.m_pImGuiContext);
    context.m_pImGuiContext = nullptr;
  }
  m_ViewToContextTable.Clear();
}

ImGuiContext* WImgui::CreateContext()
{
  // imgui reads the global context pointer WHILE creating a new context
  // so if we don't reset it to null here, it will try to access it, and crash
  // if imgui was active on the same thread before
  ImGui::SetCurrentContext(nullptr);
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());

  m_pTextScaleCVar = (WCVarFloat*)WCVar::FindCVarByName("App.TextScale");

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  cfg.DisplaySize.x = 1920;
  cfg.DisplaySize.y = 1080;

  if (m_ConfigStyleCallback.IsValid())
  {
    m_ConfigStyleCallback(ImGui::GetStyle());
  }

  ImGui::SetCurrentContext(context);
  return context;
}

void WImgui::BeginFrame(const WViewHandle& hView)
{
  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(hView, pView))
  {
    return;
  }

  auto viewport = pView->GetViewport();
  m_CurrentWindowResolution = WSizeU32(static_cast<WUInt32>(viewport.width), static_cast<WUInt32>(viewport.height));

  ImGuiIO& cfg = ImGui::GetIO();

  if (m_pTextScaleCVar)
  {
    cfg.FontGlobalScale = *m_pTextScaleCVar;
  }

  cfg.DisplaySize.x = viewport.width;
  cfg.DisplaySize.y = viewport.height;
  cfg.DeltaTime = (float)WClock::GetGlobalClock()->GetTimeDiff().GetSeconds();

  if (m_bPassInputToImgui)
  {
    const WString sChars = WInputManager::RetrieveLastCharacters(false);
    cfg.AddInputCharactersUTF8(sChars.GetData());

    float mousex, mousey;
    if (WInputManager::GetInputSlotState(WInputSlot_TouchPoint0) != WKeyState::Up)
    {
      WInputManager::GetInputSlotState(WInputSlot_TouchPoint0_PositionX, &mousex);
      WInputManager::GetInputSlotState(WInputSlot_TouchPoint0_PositionY, &mousey);
      cfg.AddMousePosEvent(cfg.DisplaySize.x * mousex, cfg.DisplaySize.y * mousey);
      cfg.AddMouseButtonEvent(0, WInputManager::GetInputSlotState(WInputSlot_TouchPoint0) >= WKeyState::Pressed);
      cfg.AddMouseButtonEvent(1, false);
      cfg.AddMouseButtonEvent(2, false);
    }
    else
    {
      WInputManager::GetInputSlotState(WInputSlot_MousePositionX, &mousex);
      WInputManager::GetInputSlotState(WInputSlot_MousePositionY, &mousey);
      cfg.AddMousePosEvent(cfg.DisplaySize.x * mousex, cfg.DisplaySize.y * mousey);
      cfg.AddMouseButtonEvent(0, WInputManager::GetInputSlotState(WInputSlot_MouseButton0) >= WKeyState::Pressed);
      cfg.AddMouseButtonEvent(1, WInputManager::GetInputSlotState(WInputSlot_MouseButton1) >= WKeyState::Pressed);
      cfg.AddMouseButtonEvent(2, WInputManager::GetInputSlotState(WInputSlot_MouseButton2) >= WKeyState::Pressed);
    }

    float fMouseWheel = 0;
    if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelDown) == WKeyState::Pressed)
      fMouseWheel = -1;
    if (WInputManager::GetInputSlotState(WInputSlot_MouseWheelUp) == WKeyState::Pressed)
      fMouseWheel = +1;
    cfg.AddMouseWheelEvent(0, fMouseWheel);



    cfg.AddKeyEvent(ImGuiKey_LeftAlt, WInputManager::GetInputSlotState(WInputSlot_KeyLeftAlt) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightAlt, WInputManager::GetInputSlotState(WInputSlot_KeyRightAlt) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftCtrl, WInputManager::GetInputSlotState(WInputSlot_KeyLeftCtrl) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightCtrl, WInputManager::GetInputSlotState(WInputSlot_KeyRightCtrl) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftShift, WInputManager::GetInputSlotState(WInputSlot_KeyLeftShift) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightShift, WInputManager::GetInputSlotState(WInputSlot_KeyRightShift) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftSuper, WInputManager::GetInputSlotState(WInputSlot_KeyLeftWin) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightSuper, WInputManager::GetInputSlotState(WInputSlot_KeyRightWin) >= WKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Tab, WInputManager::GetInputSlotState(WInputSlot_KeyTab) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftArrow, WInputManager::GetInputSlotState(WInputSlot_KeyLeft) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightArrow, WInputManager::GetInputSlotState(WInputSlot_KeyRight) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_UpArrow, WInputManager::GetInputSlotState(WInputSlot_KeyUp) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_DownArrow, WInputManager::GetInputSlotState(WInputSlot_KeyDown) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageUp, WInputManager::GetInputSlotState(WInputSlot_KeyPageUp) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageDown, WInputManager::GetInputSlotState(WInputSlot_KeyPageDown) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Home, WInputManager::GetInputSlotState(WInputSlot_KeyHome) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_End, WInputManager::GetInputSlotState(WInputSlot_KeyEnd) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Delete, WInputManager::GetInputSlotState(WInputSlot_KeyDelete) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Backspace, WInputManager::GetInputSlotState(WInputSlot_KeyBackspace) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Enter, WInputManager::GetInputSlotState(WInputSlot_KeyReturn) >= WKeyState::Pressed ||
                                      WInputManager::GetInputSlotState(WInputSlot_KeyNumpadEnter) >= WKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Escape, WInputManager::GetInputSlotState(WInputSlot_KeyEscape) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_A, WInputManager::GetInputSlotState(WInputSlot_KeyA) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_C, WInputManager::GetInputSlotState(WInputSlot_KeyC) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_V, WInputManager::GetInputSlotState(WInputSlot_KeyV) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_X, WInputManager::GetInputSlotState(WInputSlot_KeyX) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Y, WInputManager::GetInputSlotState(WInputSlot_KeyY) >= WKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Z, WInputManager::GetInputSlotState(WInputSlot_KeyZ) >= WKeyState::Pressed);
  }
  else
  {
    cfg.ClearInputKeys();
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

void WImgui::GameApplicationEventHandler(const WGameApplicationExecutionEvent& e)
{
  if (e.m_Type == WGameApplicationExecutionEvent::Type::AfterUpdatePlugins)
  {
    ImGuiContext* pContext = ImGui::GetCurrentContext();
    if (pContext && pContext->Initialized && pContext->WithinFrameScope)
    {
      ImGui::EndFrame();
    }
  }
}

#endif


W_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImgui);
