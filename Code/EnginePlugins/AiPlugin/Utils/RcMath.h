#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Math/Vec3.h>

/// Helper class to convert between Recast's convention (float[3] and Y is up) and WVec3 (Z up)
///
/// Will automatically swap Y and Z when assigning between the different types.
struct W_AIPLUGIN_DLL WRcPos
{
  float m_Pos[3];

  WRcPos();
  WRcPos(const float* pPos);
  WRcPos(const WVec3& v);

  void operator=(const WVec3& v);
  void operator=(const float* pPos);

  operator const float*() const;
  operator float*();
  operator WVec3() const;
};
