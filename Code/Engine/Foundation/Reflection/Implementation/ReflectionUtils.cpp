#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

namespace
{
  // for some reason older MSVC versions do not accept the template keyword here
  // For newer MSVC (e.g. Visual Studio 2026 and later) use the `template` form.
#if W_ENABLED(W_COMPILER_MSVC_PURE) && (_MSC_VER < 1950)
#  define CALL_FUNCTOR(functor, type) functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) functor.template operator()<type>(std::forward<Args>(args)...)
#endif

  template <typename Functor, class... Args>
  void DispatchTo(Functor& ref_functor, const WAbstractProperty* pProp, Args&&... args)
  {
    const bool bIsPtr = pProp->GetFlags().IsSet(WPropertyFlags::Pointer);
    if (bIsPtr)
    {
      CALL_FUNCTOR(ref_functor, WTypedPointer);
      return;
    }
    else if (pProp->GetSpecificType() == WGetStaticRTTI<const char*>())
    {
      CALL_FUNCTOR(ref_functor, const char*);
      return;
    }
    else if (pProp->GetSpecificType() == WGetStaticRTTI<WUntrackedString>())
    {
      CALL_FUNCTOR(ref_functor, WUntrackedString);
      return;
    }
    else if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
    {
      CALL_FUNCTOR(ref_functor, WVariant);
      return;
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
    {
      WVariant::DispatchTo(ref_functor, pProp->GetSpecificType()->GetVariantType(), std::forward<Args>(args)...);
      return;
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::IsEnum))
    {
      CALL_FUNCTOR(ref_functor, WEnumBase);
      return;
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::Bitflags))
    {
      CALL_FUNCTOR(ref_functor, WBitflagsBase);
      return;
    }
    else if (pProp->GetSpecificType()->GetVariantType() == WVariantType::TypedObject)
    {
      CALL_FUNCTOR(ref_functor, WTypedObject);
      return;
    }

    W_REPORT_FAILURE("Unknown dispatch type");
  }

#undef CALL_FUNCTOR

  struct GetTypeFromVariantTypeFunc
  {
    template <typename T>
    W_ALWAYS_INLINE void operator()()
    {
      m_pType = WGetStaticRTTI<T>();
    }
    const WRTTI* m_pType;
  };

  template <>
  W_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<WTypedPointer>()
  {
    m_pType = nullptr;
  }
  template <>
  W_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<WTypedObject>()
  {
    m_pType = nullptr;
  }

  //////////////////////////////////////////////////////////////////////////



  template <typename T>
  struct WPropertyValue
  {
    using Type = T;
    using StorageType = typename WVariantTypeDeduction<T>::StorageType;
  };
  template <>
  struct WPropertyValue<WEnumBase>
  {
    using Type = WInt64;
    using StorageType = WInt64;
  };
  template <>
  struct WPropertyValue<WBitflagsBase>
  {
    using Type = WInt64;
    using StorageType = WInt64;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct WVariantFromProperty
  {
    WVariantFromProperty(WVariant& value, const WAbstractProperty* pProp)
      : m_value(value)
    {
      W_IGNORE_UNUSED(pProp);
    }
    ~WVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = m_tempValue;
    }

    operator void*()
    {
      return &m_tempValue;
    }

    WVariant& m_value;
    typename WPropertyValue<T>::Type m_tempValue = {};
    bool m_bSuccess = true;
  };

  template <>
  struct WVariantFromProperty<WVariant>
  {
    WVariantFromProperty(WVariant& value, const WAbstractProperty* pProp)
      : m_value(value)
    {
      W_IGNORE_UNUSED(pProp);
    }

    operator void*()
    {
      return &m_value;
    }

    WVariant& m_value;
    bool m_bSuccess = true;
  };

  template <>
  struct WVariantFromProperty<WTypedPointer>
  {
    WVariantFromProperty(WVariant& value, const WAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
    }
    ~WVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = WVariant(m_ptr, m_pProp->GetSpecificType());
    }

    operator void*()
    {
      return &m_ptr;
    }

    WVariant& m_value;
    const WAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  template <>
  struct WVariantFromProperty<WTypedObject>
  {
    WVariantFromProperty(WVariant& value, const WAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
      m_ptr = m_pProp->GetSpecificType()->GetAllocator()->Allocate<void>();
    }
    ~WVariantFromProperty()
    {
      if (m_bSuccess)
        m_value.MoveTypedObject(m_ptr, m_pProp->GetSpecificType());
      else
        m_pProp->GetSpecificType()->GetAllocator()->Deallocate(m_ptr);
    }

    operator void*()
    {
      return m_ptr;
    }

    WVariant& m_value;
    const WAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct WVariantToProperty
  {
    WVariantToProperty(const WVariant& value, const WAbstractProperty* pProp)
    {
      W_IGNORE_UNUSED(pProp);
      m_tempValue = value.ConvertTo<typename WPropertyValue<T>::StorageType>();
    }

    operator const void*()
    {
      return &m_tempValue;
    }

    typename WPropertyValue<T>::Type m_tempValue = {};
  };

  template <>
  struct WVariantToProperty<const char*>
  {
    WVariantToProperty(const WVariant& value, const WAbstractProperty* pProp)
    {
      W_IGNORE_UNUSED(pProp);
      m_sData = value.ConvertTo<WString>();
      m_pValue = m_sData;
    }

    operator const void*()
    {
      return &m_pValue;
    }
    WString m_sData;
    const char* m_pValue;
  };

  template <>
  struct WVariantToProperty<WVariant>
  {
    WVariantToProperty(const WVariant& value, const WAbstractProperty* pProp)
      : m_value(value)
    {
      W_IGNORE_UNUSED(pProp);
    }

    operator const void*()
    {
      return const_cast<WVariant*>(&m_value);
    }

    const WVariant& m_value;
  };

  template <>
  struct WVariantToProperty<WTypedPointer>
  {
    WVariantToProperty(const WVariant& value, const WAbstractProperty* pProp)
    {
      W_IGNORE_UNUSED(pProp);

      if (!value.IsValid() || (value.IsString() && value.Get<WString>().IsEmpty()))
      {
        m_ptr.m_pType = nullptr;
        m_ptr.m_pObject = nullptr;
      }
      else
      {
        m_ptr = value.Get<WTypedPointer>();
      }
      W_ASSERT_DEBUG(!m_ptr.m_pType || m_ptr.m_pType->IsDerivedFrom(pProp->GetSpecificType()),
        "Pointer of type '{0}' does not derive from '{}'", m_ptr.m_pType->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }

    operator const void*()
    {
      return &m_ptr.m_pObject;
    }

    WTypedPointer m_ptr;
  };


  template <>
  struct WVariantToProperty<WTypedObject>
  {
    WVariantToProperty(const WVariant& value, const WAbstractProperty* pProp)
    {
      W_IGNORE_UNUSED(pProp);
      m_pPtr = value.GetData();
    }

    operator const void*()
    {
      return m_pPtr;
    }
    const void* m_pPtr = nullptr;
  };

  //////////////////////////////////////////////////////////////////////////

  struct GetValueFunc
  {
    template <typename T>
    W_ALWAYS_INLINE void operator()(const WAbstractMemberProperty* pProp, const void* pObject, WVariant& value)
    {
      WVariantFromProperty<T> getter(value, pProp);
      pProp->GetValuePtr(pObject, getter);
    }
  };

  struct SetValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractMemberProperty* pProp, void* pObject, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->SetValuePtr(pObject, setter);
    }
  };

  struct GetArrayValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractArrayProperty* pProp, const void* pObject, WUInt32 uiIndex, WVariant& value)
    {
      WVariantFromProperty<T> getter(value, pProp);
      pProp->GetValue(pObject, uiIndex, getter);
    }
  };

  struct SetArrayValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->SetValue(pObject, uiIndex, setter);
    }
  };

  struct InsertArrayValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, uiIndex, setter);
    }
  };

  struct InsertSetValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, setter);
    }
  };

  struct RemoveSetValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->Remove(pObject, setter);
    }
  };

  struct GetMapValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractMapProperty* pProp, const void* pObject, const char* szKey, WVariant& value)
    {
      WVariantFromProperty<T> getter(value, pProp);
      getter.m_bSuccess = pProp->GetValue(pObject, szKey, getter);
    }
  };

  struct SetMapValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()(const WAbstractMapProperty* pProp, void* pObject, const char* szKey, const WVariant& value)
    {
      WVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, szKey, setter);
    }
  };

  static bool CompareProperties(const void* pObject, const void* pObject2, const WRTTI* pType)
  {
    if (pType->GetParentType())
    {
      if (!CompareProperties(pObject, pObject2, pType->GetParentType()))
        return false;
    }

    for (auto* pProp : pType->GetProperties())
    {
      if (!WReflectionUtils::IsEqual(pObject, pObject2, pProp))
        return false;
    }

    return true;
  }

  template <typename T>
  struct SetComponentValueImpl
  {
    W_FORCE_INLINE static void impl(WVariant* pVector, WUInt32 uiComponent, double fValue)
    {
      W_IGNORE_UNUSED(pVector);
      W_IGNORE_UNUSED(uiComponent);
      W_IGNORE_UNUSED(fValue);
      W_ASSERT_DEBUG(false, "WReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct SetComponentValueImpl<WVec2Template<T>>
  {
    W_FORCE_INLINE static void impl(WVariant* pVector, WUInt32 uiComponent, double fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 2, "uiComponent out of range");
      auto vec = pVector->Get<WVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<WVec3Template<T>>
  {
    W_FORCE_INLINE static void impl(WVariant* pVector, WUInt32 uiComponent, double fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 3, "uiComponent out of range");
      auto vec = pVector->Get<WVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<WVec4Template<T>>
  {
    W_FORCE_INLINE static void impl(WVariant* pVector, WUInt32 uiComponent, double fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 4, "uiComponent out of range");
      auto vec = pVector->Get<WVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
        case 3:
          vec.w = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  struct SetComponentValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()()
    {
      SetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    WVariant* m_pVector;
    WUInt32 m_iComponent;
    double m_fValue;
  };

  template <typename T>
  struct GetComponentValueImpl
  {
    W_FORCE_INLINE static void impl(const WVariant* pVector, WUInt32 uiComponent, double& out_fValue)
    {
      W_IGNORE_UNUSED(pVector);
      W_IGNORE_UNUSED(uiComponent);
      W_IGNORE_UNUSED(out_fValue);
      W_ASSERT_DEBUG(false, "WReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct GetComponentValueImpl<WVec2Template<T>>
  {
    W_FORCE_INLINE static void impl(const WVariant* pVector, WUInt32 uiComponent, double& out_fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 2, "uiComponent out of range");
      const auto& vec = pVector->Get<WVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<WVec3Template<T>>
  {
    W_FORCE_INLINE static void impl(const WVariant* pVector, WUInt32 uiComponent, double& out_fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 3, "uiComponent out of range");
      const auto& vec = pVector->Get<WVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<WVec4Template<T>>
  {
    W_FORCE_INLINE static void impl(const WVariant* pVector, WUInt32 uiComponent, double& out_fValue)
    {
      W_ASSERT_DEBUG(uiComponent < 4, "uiComponent out of range");
      const auto& vec = pVector->Get<WVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
        case 3:
          out_fValue = static_cast<double>(vec.w);
          break;
      }
    }
  };

  struct GetComponentValueFunc
  {
    template <typename T>
    W_FORCE_INLINE void operator()()
    {
      GetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    const WVariant* m_pVector;
    WUInt32 m_iComponent;
    double m_fValue;
  };
} // namespace

const WRTTI* WReflectionUtils::GetCommonBaseType(const WRTTI* pRtti1, const WRTTI* pRtti2)
{
  if (pRtti2 == nullptr)
    return nullptr;

  while (pRtti1 != nullptr)
  {
    const WRTTI* pRtti2Parent = pRtti2;

    while (pRtti2Parent != nullptr)
    {
      if (pRtti1 == pRtti2Parent)
        return pRtti2Parent;

      pRtti2Parent = pRtti2Parent->GetParentType();
    }

    pRtti1 = pRtti1->GetParentType();
  }

  return nullptr;
}

bool WReflectionUtils::IsBasicType(const WRTTI* pRtti)
{
  W_ASSERT_DEBUG(pRtti != nullptr, "IsBasicType: missing data!");
  WVariant::Type::Enum type = pRtti->GetVariantType();
  return (type >= WVariant::Type::FirstStandardType && type <= WVariant::Type::LastStandardType) || pRtti == WGetStaticRTTI<WVariant>();
}

bool WReflectionUtils::IsValueType(const WAbstractProperty* pProp)
{
  return !pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && (pProp->GetFlags().IsSet(WPropertyFlags::StandardType) || WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProp->GetSpecificType()));
}

const WRTTI* WReflectionUtils::GetTypeFromVariant(const WVariant& value)
{
  return value.GetReflectedType();
}

const WRTTI* WReflectionUtils::GetTypeFromVariant(WVariantType::Enum type)
{
  GetTypeFromVariantTypeFunc func;
  func.m_pType = nullptr;
  WVariant::DispatchTo(func, type);

  return func.m_pType;
}

WUInt32 WReflectionUtils::GetComponentCount(WVariantType::Enum type)
{
  switch (type)
  {
    case WVariant::Type::Vector2:
    case WVariant::Type::Vector2I:
    case WVariant::Type::Vector2U:
      return 2;
    case WVariant::Type::Vector3:
    case WVariant::Type::Vector3I:
    case WVariant::Type::Vector3U:
      return 3;
    case WVariant::Type::Vector4:
    case WVariant::Type::Vector4I:
    case WVariant::Type::Vector4U:
      return 4;
    default:
      W_REPORT_FAILURE("Not a vector type: '{0}'", type);
      return 0;
  }
}

void WReflectionUtils::SetComponent(WVariant& ref_vector, WUInt32 uiComponent, double fValue)
{
  SetComponentValueFunc func;
  func.m_pVector = &ref_vector;
  func.m_iComponent = uiComponent;
  func.m_fValue = fValue;
  WVariant::DispatchTo(func, ref_vector.GetType());
}

double WReflectionUtils::GetComponent(const WVariant& vector, WUInt32 uiComponent)
{
  GetComponentValueFunc func;
  func.m_pVector = &vector;
  func.m_iComponent = uiComponent;
  WVariant::DispatchTo(func, vector.GetType());
  return func.m_fValue;
}

WVariant WReflectionUtils::GetMemberPropertyValue(const WAbstractMemberProperty* pProp, const void* pObject)
{
  WVariant res;
  W_ASSERT_DEBUG(pProp != nullptr, "GetMemberPropertyValue: missing data!");

  GetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, res);

  return res;
}

void WReflectionUtils::SetMemberPropertyValue(const WAbstractMemberProperty* pProp, void* pObject, const WVariant& value)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMemberPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsAnySet(WPropertyFlags::Bitflags | WPropertyFlags::IsEnum))
  {
    auto pEnumerationProp = static_cast<const WAbstractEnumerationProperty*>(pProp);

    // Value can either be an integer or a string (human readable value)
    if (value.IsA<WString>())
    {
      WInt64 iValue;
      WReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<WString>(), iValue);
      pEnumerationProp->SetValue(pObject, iValue);
    }
    else
    {
      pEnumerationProp->SetValue(pObject, value.ConvertTo<WInt64>());
    }
  }
  else
  {
    SetValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, value);
  }
}

WVariant WReflectionUtils::GetArrayPropertyValue(const WAbstractArrayProperty* pProp, const void* pObject, WUInt32 uiIndex)
{
  WVariant res;
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    WLog::Error("GetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    GetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, res);
  }
  return res;
}

void WReflectionUtils::SetArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex, const WVariant& value)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    WLog::Error("SetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    SetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
  }
}

void WReflectionUtils::InsertSetPropertyValue(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  InsertSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

void WReflectionUtils::RemoveSetPropertyValue(const WAbstractSetProperty* pProp, void* pObject, const WVariant& value)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  RemoveSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

WVariant WReflectionUtils::GetMapPropertyValue(const WAbstractMapProperty* pProp, const void* pObject, const char* szKey)
{
  WVariant value;
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetMapPropertyValue: missing data!");

  GetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
  return value;
}

void WReflectionUtils::SetMapPropertyValue(const WAbstractMapProperty* pProp, void* pObject, const char* szKey, const WVariant& value)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMapPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  SetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
}

void WReflectionUtils::InsertArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, const WVariant& value, WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex > uiCount)
  {
    WLog::Error("InsertArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  InsertArrayValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
}

void WReflectionUtils::RemoveArrayPropertyValue(const WAbstractArrayProperty* pProp, void* pObject, WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    WLog::Error("RemoveArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  pProp->Remove(pObject, uiIndex);
}

const WAbstractMemberProperty* WReflectionUtils::GetMemberProperty(const WRTTI* pRtti, WUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  WTempHybridArray<const WAbstractProperty*, 32> props;
  pRtti->GetAllProperties(props);
  if (uiPropertyIndex < props.GetCount())
  {
    const WAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == WPropertyCategory::Member)
      return static_cast<const WAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

const WAbstractMemberProperty* WReflectionUtils::GetMemberProperty(const WRTTI* pRtti, const char* szPropertyName)
{
  if (pRtti == nullptr)
    return nullptr;

  if (const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropertyName))
  {
    if (pProp->GetCategory() == WPropertyCategory::Member)
      return static_cast<const WAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

void WReflectionUtils::GatherTypesDerivedFromClass(const WRTTI* pBaseRtti, WSet<const WRTTI*>& out_types)
{
  WRTTI::ForEachDerivedType(
    pBaseRtti,
    [&](const WRTTI* pRtti)
    {
      out_types.Insert(pRtti);
    });
}

void WReflectionUtils::GatherDependentTypes(const WRTTI* pRtti, WSet<const WRTTI*>& inout_typesAsSet, WDynamicArray<const WRTTI*>* out_pTypesAsStack /*= nullptr*/)
{
  auto AddType = [&](const WRTTI* pNewRtti)
  {
    if (pNewRtti != pRtti && pNewRtti->GetTypeFlags().IsSet(WTypeFlags::StandardType) == false && inout_typesAsSet.Contains(pNewRtti) == false)
    {
      inout_typesAsSet.Insert(pNewRtti);
      if (out_pTypesAsStack != nullptr)
      {
        out_pTypesAsStack->PushBack(pNewRtti);
      }

      GatherDependentTypes(pNewRtti, inout_typesAsSet, out_pTypesAsStack);
    }
  };

  if (const WRTTI* pParentRtti = pRtti->GetParentType())
  {
    AddType(pParentRtti);
  }

  for (const WAbstractProperty* prop : pRtti->GetProperties())
  {
    if (prop->GetCategory() == WPropertyCategory::Constant)
      continue;

    if (prop->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    AddType(prop->GetSpecificType());
  }

  for (const WAbstractFunctionProperty* func : pRtti->GetFunctions())
  {
    WUInt32 uiNumArgs = func->GetArgumentCount();
    for (WUInt32 i = 0; i < uiNumArgs; ++i)
    {
      AddType(func->GetArgumentType(i));
    }
  }

  for (const WPropertyAttribute* attr : pRtti->GetAttributes())
  {
    AddType(attr->GetDynamicRTTI());
  }
}

WResult WReflectionUtils::CreateDependencySortedTypeArray(const WSet<const WRTTI*>& types, WDynamicArray<const WRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  WSet<const WRTTI*> accu;
  WDynamicArray<const WRTTI*> tmpStack;

  for (const WRTTI* pType : types)
  {
    if (accu.Contains(pType))
      continue;

    GatherDependentTypes(pType, accu, &tmpStack);

    while (tmpStack.IsEmpty() == false)
    {
      const WRTTI* pDependentType = tmpStack.PeekBack();
      W_ASSERT_DEBUG(pDependentType != pType, "A type must not be reported as dependency of itself");
      tmpStack.PopBack();

      if (types.Contains(pDependentType) == false)
        return W_FAILURE;

      out_sortedTypes.PushBack(pDependentType);
    }

    accu.Insert(pType);
    out_sortedTypes.PushBack(pType);
  }

  W_ASSERT_DEV(types.GetCount() == out_sortedTypes.GetCount(), "Not all types have been sorted or the sorted list contains duplicates");
  return W_SUCCESS;
}

bool WReflectionUtils::EnumerationToString(const WRTTI* pEnumerationRtti, WInt64 iValue, WStringBuilder& out_sOutput, WEnum<EnumConversionMode> conversionMode)
{
  out_sOutput.Clear();
  if (pEnumerationRtti->IsDerivedFrom<WEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        WVariant value = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant();
        if (value.ConvertTo<WInt64>() == iValue)
        {
          out_sOutput = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : WStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<WBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        WVariant value = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant();
        if ((value.ConvertTo<WInt64>() & iValue) != 0)
        {
          out_sOutput.Append(conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : WStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2, "|");
        }
      }
    }
    out_sOutput.Shrink(0, 1);
    return true;
  }
  else
  {
    W_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

void WReflectionUtils::GetEnumKeysAndValues(const WRTTI* pEnumerationRtti, WDynamicArray<EnumKeyValuePair>& ref_entries, WEnum<EnumConversionMode> conversionMode)
{
  /// \test This is new.

  ref_entries.Clear();

  if (pEnumerationRtti->IsDerivedFrom<WEnumBase>() || pEnumerationRtti->IsDerivedFrom<WBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        WVariant value = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant();

        auto& e = ref_entries.ExpandAndGetRef();
        e.m_sKey = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : WStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
        e.m_iValue = value.ConvertTo<WInt32>();
      }
    }
  }
}

bool WReflectionUtils::StringToEnumeration(const WRTTI* pEnumerationRtti, const char* szValue, WInt64& out_iValue)
{
  out_iValue = 0;
  if (pEnumerationRtti->IsDerivedFrom<WEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        // Testing fully qualified and short value name
        const char* valueNameOnly = WStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
        if (WStringUtils::IsEqual(pProp->GetPropertyName(), szValue) || (valueNameOnly != nullptr && WStringUtils::IsEqual(valueNameOnly + 2, szValue)))
        {
          WVariant value = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant();
          out_iValue = value.ConvertTo<WInt64>();
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<WBitflagsBase>())
  {
    WStringBuilder temp = szValue;
    WTempHybridArray<WStringView, 32> values;
    temp.Split(false, values, "|");
    for (auto sValue : values)
    {
      for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
      {
        if (pProp->GetCategory() == WPropertyCategory::Constant)
        {
          // Testing fully qualified and short value name
          const char* valueNameOnly = WStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
          if (sValue.IsEqual(pProp->GetPropertyName()) || (valueNameOnly != nullptr && sValue.IsEqual(valueNameOnly + 2)))
          {
            WVariant value = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant();
            out_iValue |= value.ConvertTo<WInt64>();
          }
        }
      }
    }
    return true;
  }
  else
  {
    W_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

WInt64 WReflectionUtils::DefaultEnumerationValue(const WRTTI* pEnumerationRtti)
{
  if (pEnumerationRtti->IsDerivedFrom<WEnumBase>() || pEnumerationRtti->IsDerivedFrom<WBitflagsBase>())
  {
    auto pProp = pEnumerationRtti->GetProperties()[0];
    W_ASSERT_DEBUG(pProp->GetCategory() == WPropertyCategory::Constant && WStringUtils::EndsWith(pProp->GetPropertyName(), "::Default"), "First enumeration property must be the default value constant.");
    return static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<WInt64>();
  }
  else
  {
    W_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

WInt64 WReflectionUtils::MakeEnumerationValid(const WRTTI* pEnumerationRtti, WInt64 iValue)
{
  if (pEnumerationRtti->IsDerivedFrom<WEnumBase>())
  {
    // Find current value
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        WInt64 iCurrentValue = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<WInt64>();
        if (iCurrentValue == iValue)
          return iValue;
      }
    }

    // Current value not found, return default value
    return WReflectionUtils::DefaultEnumerationValue(pEnumerationRtti);
  }
  else if (pEnumerationRtti->IsDerivedFrom<WBitflagsBase>())
  {
    WInt64 iNewValue = 0;
    // Filter valid bits
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == WPropertyCategory::Constant)
      {
        WInt64 iCurrentValue = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<WInt64>();
        if ((iCurrentValue & iValue) != 0)
        {
          iNewValue |= iCurrentValue;
        }
      }
    }
    return iNewValue;
  }
  else
  {
    W_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

bool WReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const WAbstractProperty* pProp)
{
  // #VAR TEST
  const WRTTI* pPropType = pProp->GetSpecificType();

  WVariant vTemp;
  WVariant vTemp2;

  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const WAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        vTemp2 = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();
        void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
        if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
          return false;
        if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
          return true;

        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
        {
          return IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
        }
        else
        {
          return pRefrencedObject == pRefrencedObject2;
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags) || bIsValueType)
        {
          vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          vTemp2 = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
          return vTemp == vTemp2;
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);
          void* pSubObject2 = pSpecific->GetPropertyPointer(pObject2);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return IsEqual(pSubObject, pSubObject2, pPropType);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSubObject2 = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pObject, pSubObject);
            pSpecific->GetValuePtr(pObject2, pSubObject2);
            bool bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            pPropType->GetAllocator()->Deallocate(pSubObject);
            pPropType->GetAllocator()->Deallocate(pSubObject2);
            return bEqual;
          }
          else
          {
            // TODO: return false if prop can't be compared?
            return true;
          }
        }
      }
    }
    break;
    case WPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const WAbstractArrayProperty*>(pProp);

      const WUInt32 uiCount = pSpecific->GetCount(pObject);
      const WUInt32 uiCount2 = pSpecific->GetCount(pObject2);
      if (uiCount != uiCount2)
        return false;

      if (pSpecific->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          vTemp2 = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
          if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
            return false;
          if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
            continue;

          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            if (!IsEqual(pRefrencedObject, pRefrencedObject2, pPropType))
              return false;
          }
          else
          {
            if (pRefrencedObject != pRefrencedObject2)
              return false;
          }
        }
        return true;
      }
      else
      {
        if (bIsValueType)
        {
          for (WUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            vTemp2 = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
            if (vTemp != vTemp2)
              return false;
          }
          return true;
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          void* pSubObject2 = pPropType->GetAllocator()->Allocate<void>();

          bool bEqual = true;
          for (WUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            pSpecific->GetValue(pObject2, i, pSubObject2);
            bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            if (!bEqual)
              break;
          }

          pPropType->GetAllocator()->Deallocate(pSubObject);
          pPropType->GetAllocator()->Deallocate(pSubObject2);
          return bEqual;
        }
      }
    }
    break;
    case WPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const WAbstractSetProperty*>(pProp);

      WTempHybridArray<WVariant, 16> values;
      pSpecific->GetValues(pObject, values);
      WTempHybridArray<WVariant, 16> values2;
      pSpecific->GetValues(pObject2, values2);

      const WUInt32 uiCount = values.GetCount();
      const WUInt32 uiCount2 = values2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = values2.Contains(values[i]);
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
        bool bEqual = true;
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            void* pRefrencedObject2 = values2[i].ConvertTo<void*>();
            if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
              return false;
            if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
              continue;

            bEqual = IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
          }
          if (!bEqual)
            break;
        }

        return bEqual;
      }
    }
    break;
    case WPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const WAbstractMapProperty*>(pProp);

      WTempHybridArray<WString, 16> keys;
      pSpecific->GetKeys(pObject, keys);
      WTempHybridArray<WString, 16> keys2;
      pSpecific->GetKeys(pObject2, keys2);

      const WUInt32 uiCount = keys.GetCount();
      const WUInt32 uiCount2 = keys2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;
          WVariant value1 = GetMapPropertyValue(pSpecific, pObject, keys[i]);
          WVariant value2 = GetMapPropertyValue(pSpecific, pObject2, keys[i]);
          bEqual = value1 == value2;
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if ((!pProp->GetFlags().IsSet(WPropertyFlags::Pointer) || pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)) && pProp->GetFlags().IsSet(WPropertyFlags::Class))
      {
        bool bEqual = true;
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;

          if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
          {
            const void* value1 = nullptr;
            const void* value2 = nullptr;
            pSpecific->GetValue(pObject, keys[i], &value1);
            pSpecific->GetValue(pObject2, keys[i], &value2);
            if ((value1 == nullptr) != (value2 == nullptr))
              return false;
            if ((value1 == nullptr) && (value2 == nullptr))
              continue;
            bEqual = IsEqual(value1, value2, pPropType);
          }
          else
          {
            if (pPropType->GetAllocator()->CanAllocate())
            {
              void* value1 = pPropType->GetAllocator()->Allocate<void>();
              W_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value1););
              void* value2 = pPropType->GetAllocator()->Allocate<void>();
              W_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value2););

              bool bRes1 = pSpecific->GetValue(pObject, keys[i], value1);
              bool bRes2 = pSpecific->GetValue(pObject2, keys[i], value2);
              if (bRes1 != bRes2)
                return false;
              if (!bRes1 && !bRes2)
                continue;
              bEqual = IsEqual(value1, value2, pPropType);
            }
            else
            {
              WLog::Error("The property '{0}' can not be compared as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
            }
          }
          if (!bEqual)
            break;
        }
        return bEqual;
      }
    }
    break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }
  return true;
}

bool WReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const WRTTI* pType)
{
  W_ASSERT_DEV(pObject && pObject2 && pType, "invalid type.");
  if (pType->IsDerivedFrom<WReflectedClass>())
  {
    const WReflectedClass* pRefObject = static_cast<const WReflectedClass*>(pObject);
    const WReflectedClass* pRefObject2 = static_cast<const WReflectedClass*>(pObject2);
    pType = pRefObject->GetDynamicRTTI();
    if (pType != pRefObject2->GetDynamicRTTI())
      return false;
  }

  return CompareProperties(pObject, pObject2, pType);
}


void WReflectionUtils::DeleteObject(void* pObject, const WAbstractProperty* pOwnerProperty)
{
  if (!pObject)
    return;

  const WRTTI* pType = pOwnerProperty->GetSpecificType();
  if (pType->IsDerivedFrom<WReflectedClass>())
  {
    WReflectedClass* pRefObject = static_cast<WReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  if (!pType->GetAllocator()->CanAllocate())
  {
    WLog::Error("Tried to deallocate object of type '{0}', but it has no allocator.", pType->GetTypeName());
    return;
  }
  pType->GetAllocator()->Deallocate(pObject);
}

WVariant WReflectionUtils::GetDefaultVariantFromType(WVariant::Type::Enum type)
{
  switch (type)
  {
    case WVariant::Type::Invalid:
      return WVariant();
    case WVariant::Type::Bool:
      return WVariant(false);
    case WVariant::Type::Int8:
      return WVariant((WInt8)0);
    case WVariant::Type::UInt8:
      return WVariant((WUInt8)0);
    case WVariant::Type::Int16:
      return WVariant((WInt16)0);
    case WVariant::Type::UInt16:
      return WVariant((WUInt16)0);
    case WVariant::Type::Int32:
      return WVariant((WInt32)0);
    case WVariant::Type::UInt32:
      return WVariant((WUInt32)0);
    case WVariant::Type::Int64:
      return WVariant((WInt64)0);
    case WVariant::Type::UInt64:
      return WVariant((WUInt64)0);
    case WVariant::Type::Float:
      return WVariant(0.0f);
    case WVariant::Type::Double:
      return WVariant(0.0);
    case WVariant::Type::Color:
      return WVariant(WColor(1.0f, 1.0f, 1.0f));
    case WVariant::Type::ColorGamma:
      return WVariant(WColorGammaUB(255, 255, 255));
    case WVariant::Type::Vector2:
      return WVariant(WVec2(0.0f, 0.0f));
    case WVariant::Type::Vector3:
      return WVariant(WVec3(0.0f, 0.0f, 0.0f));
    case WVariant::Type::Vector4:
      return WVariant(WVec4(0.0f, 0.0f, 0.0f, 0.0f));
    case WVariant::Type::Vector2I:
      return WVariant(WVec2I32(0, 0));
    case WVariant::Type::Vector3I:
      return WVariant(WVec3I32(0, 0, 0));
    case WVariant::Type::Vector4I:
      return WVariant(WVec4I32(0, 0, 0, 0));
    case WVariant::Type::Vector2U:
      return WVariant(WVec2U32(0, 0));
    case WVariant::Type::Vector3U:
      return WVariant(WVec3U32(0, 0, 0));
    case WVariant::Type::Vector4U:
      return WVariant(WVec4U32(0, 0, 0, 0));
    case WVariant::Type::Quaternion:
      return WVariant(WQuat(0.0f, 0.0f, 0.0f, 1.0f));
    case WVariant::Type::Matrix3:
      return WVariant(WMat3::MakeIdentity());
    case WVariant::Type::Matrix4:
      return WVariant(WMat4::MakeIdentity());
    case WVariant::Type::Transform:
      return WVariant(WTransform::MakeIdentity());
    case WVariant::Type::String:
      return WVariant(WString());
    case WVariant::Type::StringView:
      return WVariant(WStringView(), false);
    case WVariant::Type::DataBuffer:
      return WVariant(WDataBuffer());
    case WVariant::Type::Time:
      return WVariant(WTime());
    case WVariant::Type::Uuid:
      return WVariant(WUuid());
    case WVariant::Type::Angle:
      return WVariant(WAngle());
    case WVariant::Type::HashedString:
      return WVariant(WHashedString());
    case WVariant::Type::TempHashedString:
      return WVariant(WTempHashedString());
    case WVariant::Type::VariantArray:
      return WVariantArray();
    case WVariant::Type::VariantDictionary:
      return WVariantDictionary();
    case WVariant::Type::TypedPointer:
      return WVariant(static_cast<void*>(nullptr), nullptr);

    default:
      W_REPORT_FAILURE("Invalid case statement");
      return WVariant();
  }
}

WVariant WReflectionUtils::GetDefaultValue(const WAbstractProperty* pProperty, WVariant index)
{
  const bool isValueType = WReflectionUtils::IsValueType(pProperty);
  const WVariantType::Enum type = pProperty->GetFlags().IsSet(WPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(WPropertyFlags::Class) && !isValueType) ? WVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();
  const WDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<WDefaultValueAttribute>();

  switch (pProperty->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pProperty->GetSpecificType() == WGetStaticRTTI<WVariant>())
            return pAttrib->GetValue();
          if (pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
      {
        WInt64 iValue = WReflectionUtils::DefaultEnumerationValue(pProperty->GetSpecificType());
        if (pAttrib)
        {
          if (pAttrib->GetValue().CanConvertTo(WVariantType::Int64))
            iValue = pAttrib->GetValue().ConvertTo<WInt64>();
        }
        return WReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue);
      }
      else // Class
      {
        return WUuid();
      }
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<WVariantArray>())
          {
            if (!index.IsValid())
              return pAttrib->GetValue();

            WUInt32 iIndex = index.ConvertTo<WUInt32>();
            const auto& defaultArray = pAttrib->GetValue().Get<WVariantArray>();
            if (iIndex < defaultArray.GetCount())
            {
              return defaultArray[iIndex];
            }
            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return WVariantArray();

        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return WVariantArray();

        return WUuid();
      }
      break;
    case WPropertyCategory::Map:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<WVariantDictionary>())
          {
            if (!index.IsValid())
            {
              return pAttrib->GetValue();
            }
            WString sKey = index.ConvertTo<WString>();
            const auto& defaultDict = pAttrib->GetValue().Get<WVariantDictionary>();
            if (auto it = defaultDict.Find(sKey); it.IsValid())
              return it.Value();

            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return WVariantDictionary();
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return WVariantDictionary();

        return WUuid();
      }
      break;
    default:
      break;
  }

  W_REPORT_FAILURE("Don't reach here");
  return WVariant();
}

WVariant WReflectionUtils::GetDefaultVariantFromType(const WRTTI* pRtti)
{
  WVariantType::Enum type = pRtti->GetVariantType();
  switch (type)
  {
    case WVariant::Type::TypedObject:
    {
      WVariant val;
      val.MoveTypedObject(pRtti->GetAllocator()->Allocate<void>(), pRtti);
      return val;
    }
    break;

    default:
      return GetDefaultVariantFromType(type);
  }
}

void WReflectionUtils::SetAllMemberPropertiesToDefault(const WRTTI* pRtti, void* pObject)
{
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == WPropertyCategory::Member)
    {
      const WVariant defValue = WReflectionUtils::GetDefaultValue(pProp);

      WReflectionUtils::SetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pObject, defValue);
    }
  }
}

namespace
{
  template <class C>
  struct WClampCategoryType
  {
    enum
    {
      value = (((WVariant::TypeDeduction<C>::value >= WVariantType::Int8 && WVariant::TypeDeduction<C>::value <= WVariantType::Double) || (WVariant::TypeDeduction<C>::value == WVariantType::Time) || (WVariant::TypeDeduction<C>::value == WVariantType::Angle))) + ((WVariant::TypeDeduction<C>::value >= WVariantType::Vector2 && WVariant::TypeDeduction<C>::value <= WVariantType::Vector4U) * 2)
    };
  };

  template <typename T, int V = WClampCategoryType<T>::value>
  struct ClampVariantFuncImpl
  {
    static W_ALWAYS_INLINE WResult Func(WVariant& value, const WClampValueAttribute* pAttrib)
    {
      W_IGNORE_UNUSED(value);
      W_IGNORE_UNUSED(pAttrib);
      return W_FAILURE;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 1> // scalar types
  {
    static W_ALWAYS_INLINE WResult Func(WVariant& value, const WClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = WMath::Max(value.ConvertTo<T>(), pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = WMath::Min(value.ConvertTo<T>(), pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return W_SUCCESS;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 2> // vector types
  {
    static W_ALWAYS_INLINE WResult Func(WVariant& value, const WClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMax(pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMin(pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return W_SUCCESS;
    }
  };

  struct ClampVariantFunc
  {
    template <typename T>
    W_ALWAYS_INLINE WResult operator()(WVariant& value, const WClampValueAttribute* pAttrib)
    {
      return ClampVariantFuncImpl<T>::Func(value, pAttrib);
    }
  };
} // namespace

WResult WReflectionUtils::ClampValue(WVariant& value, const WClampValueAttribute* pAttrib)
{
  WVariantType::Enum type = value.GetType();
  if (type == WVariantType::Invalid || pAttrib == nullptr)
    return W_SUCCESS; // If there is nothing to clamp or no clamp attribute we call it a success.

  ClampVariantFunc func;
  return WVariant::DispatchTo(func, type, value, pAttrib);
}
