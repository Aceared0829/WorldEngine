#include <Foundation/Logging/Log.h>

template <typename T>
T WObjectAccessorBase::Get(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  WVariant value;
  WStatus res = GetValue(pObject, pProp, value, index);
  if (res.Failed())
    WLog::Error("GetValue failed: {0}", res.GetMessageString());
  return value.ConvertTo<T>();
}

template <typename T>
T WObjectAccessorBase::GetByName(const WDocumentObject* pObject, WStringView sProp, WVariant index /*= WVariant()*/)
{
  WVariant value;
  WStatus res = GetValueByName(pObject, sProp, value, index);
  if (res.Failed())
    WLog::Error("GetValue failed: {0}", res.GetMessageString());
  return value.ConvertTo<T>();
}

inline WInt32 WObjectAccessorBase::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  WInt32 iCount = 0;
  WStatus res = GetCount(pObject, pProp, iCount);
  if (res.Failed())
    WLog::Error("GetCount failed: {0}", res.GetMessageString());
  return iCount;
}

inline WInt32 WObjectAccessorBase::GetCountByName(const WDocumentObject* pObject, WStringView sProp)
{
  WInt32 iCount = 0;
  WStatus res = GetCountByName(pObject, sProp, iCount);
  if (res.Failed())
    WLog::Error("GetCount failed: {0}", res.GetMessageString());
  return iCount;
}
