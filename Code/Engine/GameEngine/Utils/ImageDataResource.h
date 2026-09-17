#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Texture/Image/Image.h>

struct WImageDataResourceDescriptor
{
  WImage m_Image;

  // WResult Serialize(WStreamWriter& stream) const;
  // WResult Deserialize(WStreamReader& stream);
};

class W_GAMEENGINE_DLL WImageDataResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WImageDataResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WImageDataResource);
  W_RESOURCE_DECLARE_CREATEABLE(WImageDataResource, WImageDataResourceDescriptor);

public:
  WImageDataResource();
  ~WImageDataResource();

  const WImageDataResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WUniquePtr<WImageDataResourceDescriptor> m_pDescriptor;
};

using WImageDataResourceHandle = WTypedResourceHandle<WImageDataResource>;
