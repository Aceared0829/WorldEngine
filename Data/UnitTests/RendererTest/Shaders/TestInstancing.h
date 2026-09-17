#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

struct W_SHADER_STRUCT WTestShaderData
{
  FLOAT4(InstanceColor);
  TRANSFORM(InstanceTransform);
};

// this is only defined during shader compilation
#if W_ENABLED(PLATFORM_SHADER)

StructuredBuffer<WTestShaderData> instancingData;

#else // C++

static_assert(sizeof(WTestShaderData) == 64);

#endif
