
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
W_ALWAYS_INLINE WDelegate<Function> WMakeDelegate(Function* pFunction)
{
  return WDelegate<Function>(pFunction);
}

template <typename Method, typename Class>
W_ALWAYS_INLINE typename WMakeDelegateHelper<Method>::DelegateType WMakeDelegate(Method method, Class* pClass)
{
  return typename WMakeDelegateHelper<Method>::DelegateType(method, pClass);
}
