#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(WTestPerFrame, 0)
{
  FLOAT1(Time);
  FLOAT1(Unused1);
  FLOAT1(Unused2);
  FLOAT1(Unused3);
};

CONSTANT_BUFFER(WTestColors, 2)
{
  FLOAT4(VertexColor);
};

CONSTANT_BUFFER(WTestPositions, 3)
{
  FLOAT4(Vertex0);
  FLOAT4(Vertex1);
  FLOAT4(Vertex2);
};
