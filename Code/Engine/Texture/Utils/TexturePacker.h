#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

struct stbrp_node;
struct stbrp_rect;

class W_TEXTURE_DLL WTexturePacker
{
  W_DISALLOW_COPY_AND_ASSIGN(WTexturePacker);

public:
  struct Texture
  {
    W_DECLARE_POD_TYPE();

    WVec2U32 m_Size;
    WVec2U32 m_Position;
  };

  WTexturePacker();
  ~WTexturePacker();

  void SetTextureSize(WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiReserveTextures = 0);

  void AddTexture(WUInt32 uiWidth, WUInt32 uiHeight);

  const WDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  WResult PackTextures();

private:
  WUInt32 m_uiWidth = 0;
  WUInt32 m_uiHeight = 0;

  WDynamicArray<Texture> m_Textures;

  WDynamicArray<stbrp_node> m_Nodes;
  WDynamicArray<stbrp_rect> m_Rects;
};
