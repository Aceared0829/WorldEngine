
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class WGALRenderTargetViewVulkan : public WGALRenderTargetView
{
public:
  vk::ImageView GetImageView() const;
  bool IsFullRange() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class WGALDeviceVulkan;
  friend class WMemoryUtils;

  WGALRenderTargetViewVulkan(WGALTexture* pTexture, const WGALRenderTargetViewCreationDescription& Description);
  virtual ~WGALRenderTargetViewVulkan();

  virtual WResult InitPlatform(WGALDevice* pDevice) override;
  virtual WResult DeInitPlatform(WGALDevice* pDevice) override;

  vk::ImageView m_ImageView;
  bool m_bBfullRange = false;
  vk::ImageSubresourceRange m_Range;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
