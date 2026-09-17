#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

BEGIN_PUSH_CONSTANTS(WRenderGraphMinMaxConstants)
{
  UINT2(TextureSize);
  INT1(SampleIndex);
  UINT1(SampleCount);
  UINT1(ChannelMask);
}
END_PUSH_CONSTANTS(WRenderGraphMinMaxConstants)
