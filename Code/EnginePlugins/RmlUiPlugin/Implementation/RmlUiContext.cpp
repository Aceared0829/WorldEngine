#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>


WRmlUiContext::WRmlUiContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
  : Rml::Context(sName, pRenderManager, pTextInputHandler)
{
}

WRmlUiContext::~WRmlUiContext() = default;

WResult WRmlUiContext::LoadDocumentFromResource(const WRmlUiResourceHandle& hResource)
{
  return WRmlUi::GetSingleton()->LoadDocumentFromResource(*this, hResource);
}

WResult WRmlUiContext::LoadDocumentFromString(const WStringView& sContent)
{
  return WRmlUi::GetSingleton()->LoadDocumentFromString(*this, sContent);
}

void WRmlUiContext::UnloadDocument()
{
  WRmlUi::GetSingleton()->UnloadDocument(*this);
}

WResult WRmlUiContext::ReloadDocumentFromResource(const WRmlUiResourceHandle& hResource)
{
  WRmlUi::GetSingleton()->ClearCaches();

  W_SUCCEED_OR_RETURN(LoadDocumentFromResource(hResource));

  RequestNextUpdate(0.0);

  return W_SUCCESS;
}

void WRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    W_LOCK(WRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Show();
  }
}

void WRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    W_LOCK(WRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Hide();
  }

  m_bWantsInput = false;
}

bool WRmlUiContext::UpdateInput(const WVec2& vMousePos, const WRmlUiInputProvider& input)
{
  bool bMouseInputConsumed = false;
  bool bKeyboardInputConsumed = false;

  int modifierState = 0;
  modifierState |= input.IsButtonDown(WRmlUiInputButtons::Alt) ? Rml::Input::KM_ALT : 0;
  modifierState |= input.IsButtonDown(WRmlUiInputButtons::Ctrl) ? Rml::Input::KM_CTRL : 0;
  modifierState |= input.IsButtonDown(WRmlUiInputButtons::Shift) ? Rml::Input::KM_SHIFT : 0;

  // Mouse
  {
    bMouseInputConsumed |= !ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(WRmlUiInputButtons::s_MouseButtonMappings); ++i)
    {
      WRmlUiInputButtons::MouseButtonMapping mbm = WRmlUiInputButtons::s_MouseButtonMappings[i];
      WKeyState::Enum state = input.GetButtonState(mbm.uiEzButton);
      if (state == WKeyState::Pressed)
      {
        bMouseInputConsumed |= !ProcessMouseButtonDown(mbm.uiRmlButton, modifierState);
      }
      else if (state == WKeyState::Released)
      {
        bMouseInputConsumed |= !ProcessMouseButtonUp(mbm.uiRmlButton, modifierState);
      }
    }

    if (input.IsButtonDown(WRmlUiInputButtons::MouseWheelDown))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (input.IsButtonDown(WRmlUiInputButtons::MouseWheelUp))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    WStringBuilder sFiltered;
    for (auto it = input.m_sLastCharacters.GetIteratorFront(); it.IsValid(); ++it)
    {
      const WUInt32 uiChar = it.GetCharacter();
      if (uiChar >= 32 || uiChar == '\n') // >= space (+ enter/return)
      {
        sFiltered.Append(uiChar);
      }
    }

    if (!sFiltered.IsEmpty())
    {
      bKeyboardInputConsumed |= !ProcessTextInput(sFiltered.GetData());
    }

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(WRmlUiInputButtons::s_KeyMappings); ++i)
    {
      WRmlUiInputButtons::KeyMapping km = WRmlUiInputButtons::s_KeyMappings[i];
      WKeyState::Enum state = input.GetButtonState(km.uiEzKey);
      if (state == WKeyState::Pressed)
      {
        bKeyboardInputConsumed |= !ProcessKeyDown(km.uiRmlKey, modifierState);
      }
      else if (state == WKeyState::Released)
      {
        bKeyboardInputConsumed |= !ProcessKeyUp(km.uiRmlKey, modifierState);
      }
    }
  }

  m_bWantsInput = bMouseInputConsumed || bKeyboardInputConsumed;

  return bMouseInputConsumed || bKeyboardInputConsumed;
}

void WRmlUiContext::SetSize(const WVec2U32& vSize)
{
  SetDimensions(Rml::Vector2i(vSize.x, vSize.y));
}

void WRmlUiContext::SetDpiScale(float fScale)
{
  SetDensityIndependentPixelRatio(fScale);
}

void WRmlUiContext::RegisterEventHandler(const char* szIdentifier, EventHandler handler)
{
  WHashedString sIdentifier;
  sIdentifier.Assign(szIdentifier);

  m_EventHandler.Insert(sIdentifier, std::move(handler));
}

void WRmlUiContext::DeregisterEventHandler(const char* szIdentifier)
{
  m_EventHandler.Remove(WTempHashedString(szIdentifier));
}

void WRmlUiContext::RegisterFallbackEventHandler(FallbackEventHandler handler)
{
  m_FallbackEventHandler = std::move(handler);
}

void WRmlUiContext::DeregisterFallbackEventHandler()
{
  m_FallbackEventHandler.Invalidate();
}

void WRmlUiContext::Update()
{
  W_LOCK(WRmlUi::GetSingleton()->GetContextMutex());

  Rml::Context::Update();

  m_uiUpdatedFrame = WRenderWorld::GetFrameCounter();
}

void WRmlUiContext::ExtractRenderData(WRmlUiInternal::RenderInterface& renderInterface, WGALTextureHandle hTexture)
{
  if (m_uiExtractedFrame != m_uiUpdatedFrame)
  {
    WHashedString sName;
    sName.Assign(WRmlUiConversionUtils::ToStringView(GetName()));

    renderInterface.BeginExtraction(sName, hTexture);

    Render();

    renderInterface.EndExtraction();

    m_uiExtractedFrame = m_uiUpdatedFrame;
  }
}

void WRmlUiContext::ProcessEvent(const WHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
  else if (m_FallbackEventHandler.IsValid())
  {
    m_FallbackEventHandler(sIdentifier, event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr WRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
{
  return Rml::ContextPtr(W_DEFAULT_NEW(WRmlUiContext, sName, pRenderManager, pTextInputHandler));
}

void WRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

void WRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
