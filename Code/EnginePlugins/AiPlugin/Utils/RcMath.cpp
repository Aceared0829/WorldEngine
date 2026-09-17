#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Utils/RcMath.h>
#include <Foundation/Math/Vec3.h>

WRcPos::WRcPos()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  m_Pos[0] = WMath::NaN<float>();
  m_Pos[1] = WMath::NaN<float>();
  m_Pos[2] = WMath::NaN<float>();
#endif
}

WRcPos::WRcPos(const WVec3& v)
{
  *this = v;
}

WRcPos::WRcPos(const float* pPos)
{
  *this = pPos;
}

WRcPos::operator const float*() const
{
  return &m_Pos[0];
}

WRcPos::operator float*()
{
  return &m_Pos[0];
}

WRcPos::operator WVec3() const
{
  return WVec3(m_Pos[0], m_Pos[2], m_Pos[1]);
}

void WRcPos::operator=(const float* pPos)
{
  m_Pos[0] = pPos[0];
  m_Pos[1] = pPos[1];
  m_Pos[2] = pPos[2];
}

void WRcPos::operator=(const WVec3& v)
{
  m_Pos[0] = v.x;
  m_Pos[1] = v.z;
  m_Pos[2] = v.y;
}
