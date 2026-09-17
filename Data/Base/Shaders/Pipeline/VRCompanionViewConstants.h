#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(WVRCompanionViewConstants, 2)
{
  FLOAT2(TargetSize);
};