#pragma once

#include <RendererCore/Pipeline/RenderData.h>

class WRmlUiRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiRenderData, WRenderData);

public:
  WGALTextureHandle m_hTexture;
  WVec2 m_vOffset = WVec2::MakeZero();
};
