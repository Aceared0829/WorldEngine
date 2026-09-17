#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

W_WARNING_PUSH()
W_WARNING_DISABLE_CLANG("-Wunused-local-typedef")
W_WARNING_DISABLE_GCC("-Wunused-local-typedefs")

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType* d = WStaticCast<DerivedType*>(pObj);
template <typename T>
W_ALWAYS_INLINE T WStaticCast(WReflectedClass* pObject)
{
  using NonPointerT = typename WTypeTraits<T>::NonPointerType;
  W_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), WGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType* d = WStaticCast<const DerivedType*>(pConstObj);
template <typename T>
W_ALWAYS_INLINE T WStaticCast(const WReflectedClass* pObject)
{
  using NonPointerT = typename WTypeTraits<T>::NonConstReferencePointerType;
  W_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), WGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType& d = WStaticCast<DerivedType&>(obj);
template <typename T>
W_ALWAYS_INLINE T WStaticCast(WReflectedClass& in_object)
{
  using NonReferenceT = typename WTypeTraits<T>::NonReferenceType;
  W_ASSERT_DEV(in_object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    in_object.GetDynamicRTTI()->GetTypeName(), WGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(in_object);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType& d = WStaticCast<const DerivedType&>(constObj);
template <typename T>
W_ALWAYS_INLINE T WStaticCast(const WReflectedClass& object)
{
  using NonReferenceT = typename WTypeTraits<T>::NonConstReferenceType;
  W_ASSERT_DEV(object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    object.GetDynamicRTTI()->GetTypeName(), WGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(object);
}

/// Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. DerivedType* d = WDynamicCast<DerivedType*>(pObj);
template <typename T>
W_ALWAYS_INLINE T WDynamicCast(WReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename WTypeTraits<T>::NonPointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

/// Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. const DerivedType* d = WDynamicCast<const DerivedType*>(pConstObj);
template <typename T>
W_ALWAYS_INLINE T WDynamicCast(const WReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename WTypeTraits<T>::NonConstReferencePointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

W_WARNING_POP()
