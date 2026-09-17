#pragma once

#include "BaseParticleShaderData.h"
#include "ParticleSystemConstants.h"

struct W_SHADER_STRUCT WTrailParticleShaderData
{
  INT1(NumPoints);

  // some shader compilers can't combine INT1 with INT3
  INT1(dummy1);
  INT1(dummy2);
  INT1(dummy3);
};

struct W_SHADER_STRUCT WTrailParticlePointsData8
{
  FLOAT4(Positions[8]);
};

struct W_SHADER_STRUCT WTrailParticlePointsData16
{
  FLOAT4(Positions[16]);
};

struct W_SHADER_STRUCT WTrailParticlePointsData32
{
  FLOAT4(Positions[32]);
};

struct W_SHADER_STRUCT WTrailParticlePointsData64
{
  FLOAT4(Positions[64]);
};

// this is only defined during shader compilation
#if W_ENABLED(PLATFORM_SHADER)

StructuredBuffer<WTrailParticleShaderData> particleTrailData BIND_GROUP(BG_DRAW_CALL);

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT8
StructuredBuffer<WTrailParticlePointsData8> particlePointsData BIND_GROUP(BG_DRAW_CALL);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT16
StructuredBuffer<WTrailParticlePointsData16> particlePointsData BIND_GROUP(BG_DRAW_CALL);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT32
StructuredBuffer<WTrailParticlePointsData32> particlePointsData BIND_GROUP(BG_DRAW_CALL);
#  endif

#  if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT64
StructuredBuffer<WTrailParticlePointsData64> particlePointsData BIND_GROUP(BG_DRAW_CALL);
#  endif

#else // C++

#endif
