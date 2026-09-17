#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>

struct WOffscreenTest_SharedTexture
{
  W_DECLARE_POD_TYPE();
  WUInt32 m_uiCurrentTextureIndex = 0;
  WUInt64 m_uiCurrentSemaphoreValue = 0;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WOffscreenTest_SharedTexture)


class WOffscreenTest_OpenMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WOffscreenTest_OpenMsg, WProcessMessage);

public:
  WGALTextureCreationDescription m_TextureDesc;
  WHybridArray<WGALPlatformSharedHandle, 2> m_TextureHandles;
};

class WOffscreenTest_CloseMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WOffscreenTest_CloseMsg, WProcessMessage);
};

class WOffscreenTest_RenderMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WOffscreenTest_RenderMsg, WProcessMessage);

public:
  WOffscreenTest_SharedTexture m_Texture;
};

class WOffscreenTest_RenderResponseMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WOffscreenTest_RenderResponseMsg, WProcessMessage);

public:
  WOffscreenTest_SharedTexture m_Texture;
};

class WOffscreenRendererTest : public WApplication
{
public:
  using SUPER = WApplication;

  WOffscreenRendererTest();
  ~WOffscreenRendererTest();

  virtual void Run() override;
  void OnPresent(WUInt32 uiCurrentTexture, WUInt64 uiCurrentSemaphoreValue);

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  void MessageFunc(const WIpcProcessMessageProtocol::Event& msg);


private:
  WLogWriter::HTML m_LogHTML;
  WGALDevice* m_pDevice = nullptr;
  WGALSwapChainHandle m_hSwapChain;
  WShaderResourceHandle m_hScreenShader;

  WInt64 m_iHostPID = 0;
  WUniquePtr<WIpcChannel> m_pChannel;
  WUniquePtr<WIpcProcessMessageProtocol> m_pProtocol;

  bool m_bExiting = false;
  WHybridArray<WOffscreenTest_RenderMsg, 2> m_RequestedFrames;
};
