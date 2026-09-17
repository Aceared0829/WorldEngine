#pragma once

#include "BaseParticleShaderData.h"

struct W_SHADER_STRUCT WTangentQuadParticleShaderData
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if W_ENABLED(PLATFORM_SHADER)

StructuredBuffer<WTangentQuadParticleShaderData> particleTangentQuadData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(WTangentQuadParticleShaderData) == 48);

#endif
