#pragma once

#include "../Common/ConstantBufferMacros.h"

BEGIN_PUSH_CONSTANTS(WBlendConstants)
{
  FLOAT1(BlendFactor);
}
END_PUSH_CONSTANTS(WBlendConstants)