#include <Core/CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

WVec3 WAmbientCubeBasis::s_Dirs[NumDirs] = {WVec3(1.0f, 0.0f, 0.0f), WVec3(-1.0f, 0.0f, 0.0f), WVec3(0.0f, 1.0f, 0.0f),
  WVec3(0.0f, -1.0f, 0.0f), WVec3(0.0f, 0.0f, 1.0f), WVec3(0.0f, 0.0f, -1.0f)};
