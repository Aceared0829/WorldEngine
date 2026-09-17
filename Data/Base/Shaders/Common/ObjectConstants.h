#pragma once

#include "ConstantBufferMacros.h"
#include "Platforms.h"

struct W_SHADER_STRUCT WPerInstanceData
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  FLOAT1(BoundingSphereRadius);
  UINT1(GameObjectID);
  UINT1(RandomSeed);

  INT1(Reserved);
  COLOR4F(Color);
  FLOAT4(CustomData);
};

#if W_ENABLED(PLATFORM_SHADER)
#  include "Common.h"
StructuredBuffer<WPerInstanceData> perInstanceData BIND_GROUP(BG_DRAW_CALL);

#  if defined(USE_SKINNING)
StructuredBuffer<Transform> skinningTransforms BIND_GROUP(BG_DRAW_CALL);
#  endif

#else // C++

W_DEFINE_AS_POD_TYPE(WPerInstanceData);

static_assert(sizeof(WPerInstanceData) == 144);
#endif

#if W_ENABLED(PLATFORM_SHADER)

// Access to instance should usually go through this macro!
// It's a macro so it can work with arbitrary input structs (for VS/GS/PS...)
#  define GetInstanceData() perInstanceData[G.Input.DataOffsets.x]
#  define GetCustomInstanceData() perInstanceData[G.Input.DataOffsets.y]

#endif
