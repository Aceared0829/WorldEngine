#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <Foundation/Strings/HashedString.h>

WAsStringFactory* WAsStringFactory::s_pFactory = nullptr;

WAsStringFactory::WAsStringFactory()
{
  s_pFactory = this;
}

WAsStringFactory::~WAsStringFactory()
{
  s_pFactory = nullptr;
}

const void* WAsStringFactory::GetStringConstant(const char* szData, asUINT length)
{
  const WString str(WStringView(szData, length));

  // we need to give out a pointer to a StringView that doesn't vanish
  W_LOCK(m_Mutex);
  auto itStr = m_Strings.Insert(str);
  auto itView = m_StringViews.Insert(itStr.Key().GetView());
  const WStringView& view = itView.Key();

  return &view;
}

int WAsStringFactory::ReleaseStringConstant(const void* pStr)
{
  // we don't clean up the strings
  return 0;
}

int WAsStringFactory::GetRawStringData(const void* pStr, char* szData, asUINT* pLength) const
{
  const WStringView* pView = (const WStringView*)pStr;

  *pLength = pView->GetElementCount();

  if (szData)
  {
    WStringUtils::Copy(szData, *pLength + 1, pView->GetStartPointer());
  }

  return 0;
}

const WString& WAsStringFactory::StoreString(const WString& sStr)
{
  W_LOCK(m_Mutex);
  auto itStr = m_Strings.Insert(sStr);

  return itStr.Key();
}
