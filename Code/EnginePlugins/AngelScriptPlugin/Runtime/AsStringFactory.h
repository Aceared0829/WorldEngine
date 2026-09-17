#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <AngelScript/include/angelscript.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Mutex.h>

class WAsStringFactory : public asIStringFactory
{
public:
  WAsStringFactory();
  ~WAsStringFactory();

  const void* GetStringConstant(const char* szData, asUINT length) override;
  int ReleaseStringConstant(const void* pStr) override;
  int GetRawStringData(const void* pStr, char* szData, asUINT* pLength) const override;

  static WAsStringFactory* GetFactory() { return s_pFactory; }

  const WString& StoreString(const WString& sStr);

private:
  static WAsStringFactory* s_pFactory;
  WMutex m_Mutex;
  WSet<WString> m_Strings;
  WSet<WStringView> m_StringViews;
};
