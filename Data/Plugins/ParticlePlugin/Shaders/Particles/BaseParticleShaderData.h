#pragma once

#include "ParticleSystemConstants.h"

struct W_SHADER_STRUCT WBaseParticleShaderData
{
  PACKEDCOLOR4H(Color);
  PACKEDHALF2(Life, Size, LifeAndSize); // Life: 1 to 0
  UINT1(Variation);                     // only lower 8 bit
};

// this is only defined during shader compilation
#if W_ENABLED(PLATFORM_SHADER)

StructuredBuffer<WBaseParticleShaderData> particleBaseData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(WBaseParticleShaderData) == 16);

#endif
