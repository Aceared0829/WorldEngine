#include <Texture/TexturePCH.h>

#include <Texture/Utils/TexturePacker.h>

#include <stb/stb_rect_pack.h>

W_DEFINE_AS_POD_TYPE(stbrp_node);
W_DEFINE_AS_POD_TYPE(stbrp_rect);

WTexturePacker::WTexturePacker() = default;

WTexturePacker::~WTexturePacker() = default;

void WTexturePacker::SetTextureSize(WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiReserveTextures /*= 0*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  m_Textures.Clear();
  m_Textures.Reserve(uiReserveTextures);
}

void WTexturePacker::AddTexture(WUInt32 uiWidth, WUInt32 uiHeight)
{
  Texture& tex = m_Textures.ExpandAndGetRef();
  tex.m_Size.x = uiWidth;
  tex.m_Size.y = uiHeight;
}

WResult WTexturePacker::PackTextures()
{
  m_Nodes.SetCountUninitialized(m_uiWidth);

  stbrp_context context;
  stbrp_init_target(&context, m_uiWidth, m_uiHeight, m_Nodes.GetData(), m_Nodes.GetCount());

  m_Rects.SetCountUninitialized(m_Textures.GetCount());
  for (uint32_t i = 0; i < m_Textures.GetCount(); ++i)
  {
    m_Rects[i] = {int(i), stbrp_coord(m_Textures[i].m_Size.x), stbrp_coord(m_Textures[i].m_Size.y), 0, 0, 0};
  }

  if (!stbrp_pack_rects(&context, m_Rects.GetData(), m_Rects.GetCount()))
  {
    return W_FAILURE;
  }

  for (uint32_t i = 0; i < m_Textures.GetCount(); ++i)
  {
    m_Textures[i].m_Position.x = m_Rects[i].x;
    m_Textures[i].m_Position.y = m_Rects[i].y;
  }

  return W_SUCCESS;
}
