#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class W_FOUNDATION_DLL WPlatformDesc : public WEnumerable<WPlatformDesc>
{
  W_DECLARE_ENUMERABLE_CLASS(WPlatformDesc);

public:
  WPlatformDesc(const char* szName, const char* szType)
  {
    m_szName = szName;
    m_szType = szType;
  }

  const char* GetName() const
  {
    return m_szName;
  }

  const char* GetType() const
  {
    return m_szType;
  }

  static const WPlatformDesc& GetThisPlatformDesc()
  {
    return *s_pThisPlatform;
  }

private:
  static const WPlatformDesc* s_pThisPlatform;

  const char* m_szName;
  const char* m_szType;
};
