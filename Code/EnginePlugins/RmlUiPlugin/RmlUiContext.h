#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInput.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WBlackboard;

namespace WRmlUiInternal
{
  class RenderInterface;
  class EventListener;
} // namespace WRmlUiInternal

class W_RMLUIPLUGIN_DLL WRmlUiContext final : public Rml::Context
{
public:
  WRmlUiContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler);
  ~WRmlUiContext();

public:
  WResult LoadDocumentFromResource(const WRmlUiResourceHandle& hResource);
  WResult LoadDocumentFromString(const WStringView& sContent);

  void UnloadDocument();
  WResult ReloadDocumentFromResource(const WRmlUiResourceHandle& hResource);

  bool HasDocument() { return GetNumDocuments() > 0; }

  void ShowDocument();
  void HideDocument();

  /// Returns true if the input was consumed
  bool UpdateInput(const WVec2& vMousePos, const WRmlUiInputProvider& input);
  bool WantsInput() const { return m_bWantsInput; }

  void SetSize(const WVec2U32& vSize);
  void SetDpiScale(float fScale);

  using EventHandler = WDelegate<void(Rml::Event&)>;

  /// Registers an event handler for a RmlUI event (such as 'onclick')
  ///
  /// There can only be one event handler for each event type.
  /// If called multiple times, the existing event handler is overridden.
  void RegisterEventHandler(const char* szIdentifier, EventHandler handler);

  /// Removes a previously registered RmlUI event handler.
  void DeregisterEventHandler(const char* szIdentifier);

  using FallbackEventHandler = WDelegate<void(const WHashedString&, Rml::Event&)>;

  /// Registers a fallback event handler for RmlUI events which is called when no specific event handler is registered for the event's type.
  void RegisterFallbackEventHandler(FallbackEventHandler handler);

  /// Removes the previously registered fallback event handler.
  void DeregisterFallbackEventHandler();

  void Update();

private:
  friend class WRmlUi;
  void ExtractRenderData(WRmlUiInternal::RenderInterface& renderInterface, WGALTextureHandle hTexture);

  friend class WRmlUiInternal::EventListener;
  void ProcessEvent(const WHashedString& sIdentifier, Rml::Event& event);

  WHashTable<WHashedString, EventHandler> m_EventHandler;
  FallbackEventHandler m_FallbackEventHandler;

  WUInt64 m_uiUpdatedFrame = WUInt64(-1);
  WUInt64 m_uiExtractedFrame = WUInt64(-1);

  bool m_bWantsInput = false;
};

namespace WRmlUiInternal
{
  class ContextInstancer : public Rml::ContextInstancer
  {
  public:
    virtual Rml::ContextPtr InstanceContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler) override;
    virtual void ReleaseContext(Rml::Context* pContext) override;

  private:
    virtual void Release() override;
  };
} // namespace WRmlUiInternal
