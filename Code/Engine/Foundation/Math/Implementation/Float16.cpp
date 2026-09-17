#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

WFloat16::WFloat16(float f)
{
  operator=(f);
}

void WFloat16::operator=(float f)
{
  // source: http://www.ogre3d.org/docs/api/html/OgreBitwise_8h_source.html

  const WUInt32 i = *reinterpret_cast<WUInt32*>(&f);

  const WUInt32 s = (i >> 16) & 0x00008000;
  const WInt32 e = ((i >> 23) & 0x000000ff) - (127 - 15);
  WUInt32 m = i & 0x007fffff;

  if (e <= 0)
  {
    if (e < -10)
    {
      m_uiData = 0;
      return;
    }
    m = (m | 0x00800000) >> (1 - e);

    m_uiData = static_cast<WUInt16>(s | (m >> 13));
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0) // Inf
    {
      m_uiData = static_cast<WUInt16>(s | 0x7c00);
    }
    else // NAN
    {
      m >>= 13;
      m_uiData = static_cast<WUInt16>(s | 0x7c00 | m | (m == 0));
    }
  }
  else
  {
    if (e > 30) // Overflow
    {
      m_uiData = static_cast<WUInt16>(s | 0x7c00);
      return;
    }

    m_uiData = static_cast<WUInt16>(s | (e << 10) | (m >> 13));
  }
}

WFloat16::operator float() const
{
  const WUInt32 s = (m_uiData >> 15) & 0x00000001;
  WUInt32 e = (m_uiData >> 10) & 0x0000001f;
  WUInt32 m = m_uiData & 0x000003ff;

  WUInt32 uiResult;

  if (e == 0)
  {
    if (m == 0) // Plus or minus zero
    {
      uiResult = s << 31;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // Denormalized number -- renormalize it
    {
      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -= 1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0) // Inf
    {
      uiResult = (s << 31) | 0x7f800000;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // NaN
    {
      uiResult = (s << 31) | 0x7f800000 | (m << 13);
      return *reinterpret_cast<float*>(&uiResult);
    }
  }

  e = e + (127 - 15);
  m = m << 13;

  uiResult = (s << 31) | (e << 23) | m;

  return *reinterpret_cast<float*>(&uiResult);
}

//////////////////////////////////////////////////////////////////////////

WFloat16Vec2::WFloat16Vec2(const WVec2& vVec)
{
  operator=(vVec);
}

void WFloat16Vec2::operator=(const WVec2& vVec)
{
  x = vVec.x;
  y = vVec.y;
}

WFloat16Vec2::operator WVec2() const
{
  return WVec2(x, y);
}

//////////////////////////////////////////////////////////////////////////

WFloat16Vec3::WFloat16Vec3(const WVec3& vVec)
{
  operator=(vVec);
}

void WFloat16Vec3::operator=(const WVec3& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
}

WFloat16Vec3::operator WVec3() const
{
  return WVec3(x, y, z);
}

//////////////////////////////////////////////////////////////////////////

WFloat16Vec4::WFloat16Vec4(const WVec4& vVec)
{
  operator=(vVec);
}

void WFloat16Vec4::operator=(const WVec4& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
  w = vVec.w;
}

WFloat16Vec4::operator WVec4() const
{
  return WVec4(x, y, z, w);
}
