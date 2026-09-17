#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <RendererFoundation/Device/SwapChain.h>

class WXRInterface;

class W_GAMEENGINE_DLL WGALXRSwapChain : public WGALSwapChain
{
public:
  using Functor = WDelegate<WGALSwapChainHandle(WXRInterface*)>;
  static void SetFactoryMethod(Functor factory);

  static WGALSwapChainHandle Create(WXRInterface* pXrInterface);

public:
  WGALXRSwapChain(WXRInterface* pXrInterface);
  virtual WResult UpdateSwapChain(WGALDevice* pDevice, WEnum<WGALPresentMode> newPresentMode) override;

protected:
  static Functor s_Factory;

protected:
  WXRInterface* m_pXrInterface = nullptr;
};
W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WGALXRSwapChain);
