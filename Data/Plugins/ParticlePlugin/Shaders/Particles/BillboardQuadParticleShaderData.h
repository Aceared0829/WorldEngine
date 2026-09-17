#pragma once

#include "BaseParticleShaderData.h"
#include "ParticleSystemConstants.h"

struct W_SHADER_STRUCT WBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if W_ENABLED(PLATFORM_SHADER)

StructuredBuffer<WBillboardQuadParticleShaderData> particleBillboardQuadData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(WBillboardQuadParticleShaderData) == 16);

#endif
