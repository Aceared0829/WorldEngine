#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(WReflectionFilteredSpecularConstants, 3)
{
  UINT1(MipLevel);
  UINT1(OutputWidth);
  UINT1(OutputHeight);
  FLOAT1(Intensity);
};
