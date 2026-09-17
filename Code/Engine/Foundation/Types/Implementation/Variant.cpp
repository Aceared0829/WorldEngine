#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/VariantTypeRegistry.h>

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WVariant) == 24);
#else
static_assert(sizeof(WVariant) == 20);
#endif

/// constructors

WVariant::WVariant(const WMat3& value)
{
  InitShared(value);
}

WVariant::WVariant(const WMat4& value)
{
  InitShared(value);
}

WVariant::WVariant(const WTransform& value)
{
  InitShared(value);
}

WVariant::WVariant(const char* value)
{
  InitShared(value);
}

WVariant::WVariant(const WString& value)
{
  InitShared(value);
}

WVariant::WVariant(const WStringView& value, bool bCopyString)
{
  if (bCopyString)
    InitShared(WString(value));
  else
    InitInplace(value);
}

WVariant::WVariant(const WUntrackedString& value)
{
  InitShared(value);
}

WVariant::WVariant(const WDataBuffer& value)
{
  InitShared(value);
}

WVariant::WVariant(const WVariantArray& value)
{
  using StorageType = typename TypeDeduction<WVariantArray>::StorageType;
  m_Data.shared = W_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<WVariantArray>::value;
  m_bIsShared = true;
}

WVariant::WVariant(const WVariantDictionary& value)
{
  using StorageType = typename TypeDeduction<WVariantDictionary>::StorageType;
  m_Data.shared = W_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<WVariantDictionary>::value;
  m_bIsShared = true;
}

WVariant::WVariant(const WTypedPointer& value)
{
  InitInplace(value);
}

WVariant::WVariant(const WTypedObject& value)
{
  void* ptr = WReflectionSerializer::Clone(value.m_pObject, value.m_pType);
  m_Data.shared = W_DEFAULT_NEW(RTTISharedData, ptr, value.m_pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void WVariant::CopyTypedObject(const void* value, const WRTTI* pType)
{
  Release();
  void* ptr = WReflectionSerializer::Clone(value, pType);
  m_Data.shared = W_DEFAULT_NEW(RTTISharedData, ptr, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void WVariant::MoveTypedObject(void* value, const WRTTI* pType)
{
  Release();
  m_Data.shared = W_DEFAULT_NEW(RTTISharedData, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
W_ALWAYS_INLINE void WVariant::InitShared(const T& value)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  static_assert((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  static_assert(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  const WRTTI* pType = WGetStaticRTTI<T>();

  m_Data.shared = W_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  W_FORCE_INLINE WUInt64 operator()(const WVariant& v, const void* pData, WUInt64 uiSeed)
  {
    W_IGNORE_UNUSED(v);
    static_assert(sizeof(typename WVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                    !WVariant::TypeDeduction<T>::forceSharing,
      "This type requires special handling! Add a specialization below.");
    return WHashingUtils::xxHash64(pData, sizeof(T), uiSeed);
  }
};

template <>
W_ALWAYS_INLINE WUInt64 ComputeHashFunc::operator()<WString>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  auto pString = static_cast<const WString*>(pData);
  return WHashingUtils::xxHash64String(*pString, uiSeed);
}

template <>
W_ALWAYS_INLINE WUInt64 ComputeHashFunc::operator()<WMat3>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  return WHashingUtils::xxHash64(pData, sizeof(WMat3), uiSeed);
}

template <>
W_ALWAYS_INLINE WUInt64 ComputeHashFunc::operator()<WMat4>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  return WHashingUtils::xxHash64(pData, sizeof(WMat4), uiSeed);
}

template <>
W_ALWAYS_INLINE WUInt64 ComputeHashFunc::operator()<WTransform>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  return WHashingUtils::xxHash64(pData, sizeof(WTransform), uiSeed);
}

template <>
W_ALWAYS_INLINE WUInt64 ComputeHashFunc::operator()<WDataBuffer>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  auto pDataBuffer = static_cast<const WDataBuffer*>(pData);
  return WHashingUtils::xxHash64(pDataBuffer->GetData(), pDataBuffer->GetCount(), uiSeed);
}

template <>
W_FORCE_INLINE WUInt64 ComputeHashFunc::operator()<WVariantArray>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);

  auto pVariantArray = static_cast<const WVariantArray*>(pData);

  WUInt64 uiHash = uiSeed;
  for (const WVariant& var : *pVariantArray)
  {
    uiHash = var.ComputeHash(uiHash);
  }

  return uiHash;
}

template <>
WUInt64 ComputeHashFunc::operator()<WVariantDictionary>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);

  auto pVariantDictionary = static_cast<const WVariantDictionary*>(pData);

  WTempHybridArray<WUInt64, 128> hashes;
  hashes.Reserve(pVariantDictionary->GetCount() * 2);

  for (auto& it : *pVariantDictionary)
  {
    hashes.PushBack(WHashingUtils::xxHash64String(it.Key(), uiSeed));
    hashes.PushBack(it.Value().ComputeHash(uiSeed));
  }

  hashes.Sort();

  return WHashingUtils::xxHash64(hashes.GetData(), hashes.GetCount() * sizeof(WUInt64), uiSeed);
}

template <>
W_FORCE_INLINE WUInt64 ComputeHashFunc::operator()<WTypedPointer>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  W_IGNORE_UNUSED(v);
  W_IGNORE_UNUSED(pData);
  W_IGNORE_UNUSED(uiSeed);

  W_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
W_FORCE_INLINE WUInt64 ComputeHashFunc::operator()<WTypedObject>(const WVariant& v, const void* pData, WUInt64 uiSeed)
{
  auto pType = v.GetReflectedType();

  const WVariantTypeInfo* pTypeInfo = WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  W_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", pType->GetTypeName());
  W_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  WUInt32 uiHash32 = pTypeInfo->Hash(pData);

  return WHashingUtils::xxHash64(&uiHash32, sizeof(WUInt32), uiSeed);
}

struct CompareFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()()
  {
    m_bResult = m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const WVariant* m_pThis;
  const WVariant* m_pOther;
  bool m_bResult;
};

template <>
W_FORCE_INLINE void CompareFunc::operator()<WTypedObject>()
{
  m_bResult = false;
  WTypedObject A = m_pThis->Get<WTypedObject>();
  WTypedObject B = m_pOther->Get<WTypedObject>();
  if (A.m_pType == B.m_pType)
  {
    const WVariantTypeInfo* pTypeInfo = WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(A.m_pType);
    W_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", A.m_pType->GetTypeName());
    W_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
    m_bResult = pTypeInfo->Equal(A.m_pObject, B.m_pObject);
  }
}

struct IndexFunc
{
  template <typename T>
  W_FORCE_INLINE WVariant Impl(WTraitInt<1>)
  {
    const WRTTI* pRtti = m_pThis->GetReflectedType();
    const WAbstractMemberProperty* pProp = WReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
    if (!pProp)
      return WVariant();

    if (m_pThis->GetType() == WVariantType::TypedPointer)
    {
      const WTypedPointer& ptr = m_pThis->Get<WTypedPointer>();
      if (ptr.m_pObject)
        return WReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return WVariant();
    }
    return WReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  W_ALWAYS_INLINE WVariant Impl(WTraitInt<0>)
  {
    return WVariant();
  }

  template <typename T>
  W_FORCE_INLINE void operator()()
  {
    m_Result = Impl<T>(WTraitInt<WVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const WVariant* m_pThis;
  WVariant m_Result;
  WUInt32 m_uiIndex;
};

struct KeyFunc
{
  template <typename T>
  W_FORCE_INLINE WVariant Impl(WTraitInt<1>)
  {
    const WRTTI* pRtti = m_pThis->GetReflectedType();
    const WAbstractMemberProperty* pProp = WReflectionUtils::GetMemberProperty(pRtti, m_szKey);
    if (!pProp)
      return WVariant();
    if (m_pThis->GetType() == WVariantType::TypedPointer)
    {
      const WTypedPointer& ptr = m_pThis->Get<WTypedPointer>();
      if (ptr.m_pObject)
        return WReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return WVariant();
    }
    return WReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  W_ALWAYS_INLINE WVariant Impl(WTraitInt<0>)
  {
    return WVariant();
  }

  template <typename T>
  W_ALWAYS_INLINE void operator()()
  {
    m_Result = Impl<T>(WTraitInt<WVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const WVariant* m_pThis;
  WVariant m_Result;
  const char* m_szKey;
};

struct ConvertFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()()
  {
    T result = {};
    WVariantHelper::To(*m_pThis, result, m_bSuccessful);

    if constexpr (std::is_same_v<T, WStringView>)
    {
      m_Result = WVariant(result, false);
    }
    else
    {
      m_Result = result;
    }
  }

  const WVariant* m_pThis;
  WVariant m_Result;
  bool m_bSuccessful;
};

/// public methods

bool WVariant::operator==(const WVariant& other) const
{
  if (m_uiType == Type::Invalid && other.m_uiType == Type::Invalid)
  {
    return true;
  }
  else if ((IsFloatingPoint() && other.IsNumber()) || (other.IsFloatingPoint() && IsNumber()))
  {
    // if either of them is a floating point number, compare them as doubles

    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber() && other.IsNumber())
  {
    return ConvertNumber<WInt64>() == other.ConvertNumber<WInt64>();
  }
  else if (IsString() && other.IsString())
  {
    const WStringView a = IsA<WStringView>() ? Get<WStringView>() : WStringView(Get<WString>().GetData());
    const WStringView b = other.IsA<WStringView>() ? other.Get<WStringView>() : WStringView(other.Get<WString>().GetData());
    return a.IsEqual(b);
  }
  else if (IsHashedString() && other.IsHashedString())
  {
    const WTempHashedString a = IsA<WTempHashedString>() ? Get<WTempHashedString>() : WTempHashedString(Get<WHashedString>());
    const WTempHashedString b = other.IsA<WTempHashedString>() ? other.Get<WTempHashedString>() : WTempHashedString(other.Get<WHashedString>());
    return a == b;
  }
  else if (m_uiType == other.m_uiType)
  {
    CompareFunc compareFunc;
    compareFunc.m_pThis = this;
    compareFunc.m_pOther = &other;

    DispatchTo(compareFunc, GetType());

    return compareFunc.m_bResult;
  }

  return false;
}

WTypedPointer WVariant::GetWriteAccess()
{
  WTypedPointer obj;
  obj.m_pType = GetReflectedType();
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef > 1)
    {
      // We need to make sure we hold the only reference to the shared data to be able to edit it.
      SharedData* pData = m_Data.shared->Clone();
      Release();
      m_Data.shared = pData;
    }
    obj.m_pObject = m_Data.shared->m_Ptr;
  }
  else
  {
    obj.m_pObject = m_uiType == Type::TypedPointer ? Cast<WTypedPointer>().m_pObject : &m_Data;
  }
  return obj;
}

const WVariant WVariant::operator[](WUInt32 uiIndex) const
{
  if (m_uiType == Type::VariantArray)
  {
    const WVariantArray& a = Cast<WVariantArray>();
    if (uiIndex < a.GetCount())
      return a[uiIndex];
  }
  else if (IsValid())
  {
    IndexFunc func;
    func.m_pThis = this;
    func.m_uiIndex = uiIndex;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return WVariant();
}

const WVariant WVariant::operator[](StringWrapper key) const
{
  if (m_uiType == Type::VariantDictionary)
  {
    WVariant result;
    Cast<WVariantDictionary>().TryGetValue(key.m_str, result);
    return result;
  }
  else if (IsValid())
  {
    KeyFunc func;
    func.m_pThis = this;
    func.m_szKey = key.m_str;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return WVariant();
}

bool WVariant::CanConvertTo(Type::Enum type) const
{
  if (m_uiType == type)
    return true;

  if (type == Type::Invalid)
    return false;

  const bool bTargetIsString = (type == Type::String) || (type == Type::HashedString) || (type == Type::TempHashedString);

  if (bTargetIsString && m_uiType == Type::Invalid)
    return true;

  if (bTargetIsString && (m_uiType > Type::FirstStandardType && m_uiType < Type::LastStandardType && m_uiType != Type::DataBuffer))
    return true;
  if (bTargetIsString && (m_uiType == Type::VariantArray || m_uiType == Type::VariantDictionary))
    return true;
  if (type == Type::StringView && (m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;
  if (type == Type::TempHashedString && m_uiType == Type::HashedString)
    return true;

  if (!IsValid())
    return false;

  if (IsNumberStatic(type) && (IsNumber() || m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;

  if (IsVector2Static(type) && (IsVector2Static(m_uiType) || IsNumber()))
    return true;

  if (IsVector3Static(type) && (IsVector3Static(m_uiType) || IsNumber()))
    return true;

  if (IsVector4Static(type) && (IsVector4Static(m_uiType) || IsNumber()))
    return true;

  if (type == Type::Color && m_uiType == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_uiType == Type::Color)
    return true;

  return false;
}

WVariant WVariant::ConvertTo(Type::Enum type, WResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo(type))
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = W_FAILURE;

    return WVariant(); // creates an invalid variant
  }

  if (m_uiType == type)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = W_SUCCESS;

    return *this;
  }

  ConvertFunc convertFunc;
  convertFunc.m_pThis = this;
  convertFunc.m_bSuccessful = true;

  DispatchTo(convertFunc, type);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = convertFunc.m_bSuccessful ? W_SUCCESS : W_FAILURE;

  return convertFunc.m_Result;
}

WUInt64 WVariant::ComputeHash(WUInt64 uiSeed) const
{
  if (!IsValid())
    return uiSeed;

  ComputeHashFunc obj;
  return DispatchTo<ComputeHashFunc>(obj, GetType(), *this, GetData(), uiSeed + GetType());
}


inline WVariant::RTTISharedData::RTTISharedData(void* pData, const WRTTI* pType)
  : SharedData(pData, pType)
{
  W_ASSERT_DEBUG(pType != nullptr && pType->GetAllocator()->CanAllocate(), "");
}

inline WVariant::RTTISharedData::~RTTISharedData()
{
  m_pType->GetAllocator()->Deallocate(m_Ptr);
}


WVariant::WVariant::SharedData* WVariant::RTTISharedData::Clone() const
{
  void* ptr = WReflectionSerializer::Clone(m_Ptr, m_pType);
  return W_DEFAULT_NEW(RTTISharedData, ptr, m_pType);
}

struct GetTypeFromVariantFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()()
  {
    m_pType = WGetStaticRTTI<T>();
  }

  const WVariant* m_pVariant;
  const WRTTI* m_pType;
};

template <>
W_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<WVariantArray>()
{
  m_pType = nullptr;
}
template <>
W_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<WVariantDictionary>()
{
  m_pType = nullptr;
}
template <>
W_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<WTypedPointer>()
{
  m_pType = m_pVariant->Cast<WTypedPointer>().m_pType;
}
template <>
W_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<WTypedObject>()
{
  m_pType = m_pVariant->m_bIsShared ? m_pVariant->m_Data.shared->m_pType : m_pVariant->m_Data.inlined.m_pType;
}

const WRTTI* WVariant::GetReflectedType() const
{
  if (m_uiType != Type::Invalid)
  {
    GetTypeFromVariantFunc func;
    func.m_pVariant = this;
    func.m_pType = nullptr;
    WVariant::DispatchTo(func, GetType());
    return func.m_pType;
  }
  return nullptr;
}

void WVariant::InitTypedPointer(void* value, const WRTTI* pType)
{
  WTypedPointer ptr;
  ptr.m_pObject = value;
  ptr.m_pType = pType;

  WMemoryUtils::CopyConstruct(reinterpret_cast<WTypedPointer*>(&m_Data), ptr, 1);

  m_uiType = TypeDeduction<WTypedPointer>::value;
  m_bIsShared = false;
}

bool WVariant::IsDerivedFrom(const WRTTI* pType1, const WRTTI* pType2)
{
  return pType1->IsDerivedFrom(pType2);
}

WStringView WVariant::GetTypeName(const WRTTI* pType)
{
  return pType->GetTypeName();
}

//////////////////////////////////////////////////////////////////////////

struct AddFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()(const WVariant& a, const WVariant& b, WVariant& out_res)
  {
    if constexpr (std::is_same_v<T, WInt8> || std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt16> || std::is_same_v<T, WUInt16> ||
                  std::is_same_v<T, WInt32> || std::is_same_v<T, WUInt32> ||
                  std::is_same_v<T, WInt64> || std::is_same_v<T, WUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4> ||
                  std::is_same_v<T, WVec2I32> || std::is_same_v<T, WVec3I32> || std::is_same_v<T, WVec4I32> ||
                  std::is_same_v<T, WVec2U32> || std::is_same_v<T, WVec3U32> || std::is_same_v<T, WVec4U32> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      out_res = a.Get<T>() + b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, WString> || std::is_same_v<T, WStringView>)
    {
      WStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());
      out_res = WString(s.GetView());
    }
    else if constexpr (std::is_same_v<T, WHashedString>)
    {
      WStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());

      WHashedString hashedS;
      hashedS.Assign(s);
      out_res = hashedS;
    }
  }
};

WVariant operator+(const WVariant& a, const WVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = WMath::Max(a.GetType(), b.GetType());

    AddFunc func;
    WVariant result;
    WVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    AddFunc func;
    WVariant result;
    WVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return WVariant();
}

//////////////////////////////////////////////////////////////////////////

struct SubFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()(const WVariant& a, const WVariant& b, WVariant& out_res)
  {
    if constexpr (std::is_same_v<T, WInt8> || std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt16> || std::is_same_v<T, WUInt16> ||
                  std::is_same_v<T, WInt32> || std::is_same_v<T, WUInt32> ||
                  std::is_same_v<T, WInt64> || std::is_same_v<T, WUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4> ||
                  std::is_same_v<T, WVec2I32> || std::is_same_v<T, WVec3I32> || std::is_same_v<T, WVec4I32> ||
                  std::is_same_v<T, WVec2U32> || std::is_same_v<T, WVec3U32> || std::is_same_v<T, WVec4U32> ||
                  std::is_same_v<T, WTime> ||
                  std::is_same_v<T, WAngle>)
    {
      out_res = a.Get<T>() - b.Get<T>();
    }
  }
};

WVariant operator-(const WVariant& a, const WVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = WMath::Max(a.GetType(), b.GetType());

    SubFunc func;
    WVariant result;
    WVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    SubFunc func;
    WVariant result;
    WVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return WVariant();
}

//////////////////////////////////////////////////////////////////////////

struct MulFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()(const WVariant& a, const WVariant& b, WVariant& out_res)
  {
    if constexpr (std::is_same_v<T, WInt8> || std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt16> || std::is_same_v<T, WUInt16> ||
                  std::is_same_v<T, WInt32> || std::is_same_v<T, WUInt32> ||
                  std::is_same_v<T, WInt64> || std::is_same_v<T, WUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, WColor> ||
                  std::is_same_v<T, WTime>)
    {
      out_res = a.Get<T>() * b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4> ||
                       std::is_same_v<T, WVec2I32> || std::is_same_v<T, WVec3I32> || std::is_same_v<T, WVec4I32> ||
                       std::is_same_v<T, WVec2U32> || std::is_same_v<T, WVec3U32> || std::is_same_v<T, WVec4U32>)
    {
      out_res = a.Get<T>().CompMul(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, WAngle>)
    {
      out_res = WAngle(a.Get<T>() * b.Get<T>().GetRadian());
    }
  }
};

WVariant operator*(const WVariant& a, const WVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = WMath::Max(a.GetType(), b.GetType());

    MulFunc func;
    WVariant result;
    WVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    MulFunc func;
    WVariant result;
    WVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return WVariant();
}

//////////////////////////////////////////////////////////////////////////

struct DivFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()(const WVariant& a, const WVariant& b, WVariant& out_res)
  {
    if constexpr (std::is_same_v<T, WInt8> || std::is_same_v<T, WUInt8> ||
                  std::is_same_v<T, WInt16> || std::is_same_v<T, WUInt16> ||
                  std::is_same_v<T, WInt32> || std::is_same_v<T, WUInt32> ||
                  std::is_same_v<T, WInt64> || std::is_same_v<T, WUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, WTime>)
    {
      out_res = a.Get<T>() / b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, WVec2> || std::is_same_v<T, WVec3> || std::is_same_v<T, WVec4> ||
                       std::is_same_v<T, WVec2I32> || std::is_same_v<T, WVec3I32> || std::is_same_v<T, WVec4I32> ||
                       std::is_same_v<T, WVec2U32> || std::is_same_v<T, WVec3U32> || std::is_same_v<T, WVec4U32>)
    {
      out_res = a.Get<T>().CompDiv(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, WAngle>)
    {
      out_res = WAngle(a.Get<T>() / b.Get<T>().GetRadian());
    }
  }
};

WVariant operator/(const WVariant& a, const WVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = WMath::Max(a.GetType(), b.GetType());

    DivFunc func;
    WVariant result;
    WVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    DivFunc func;
    WVariant result;
    WVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return WVariant();
}

//////////////////////////////////////////////////////////////////////////

struct LerpFunc
{
  constexpr static bool CanInterpolate(WVariantType::Enum variantType)
  {
    return variantType >= WVariantType::Int8 && variantType <= WVariantType::Vector4;
  }

  template <typename T>
  W_ALWAYS_INLINE void operator()(const WVariant& a, const WVariant& b, double x, WVariant& out_res)
  {
    if constexpr (std::is_same_v<T, WQuat>)
    {
      WQuat q = WQuat::MakeSlerp(a.Get<WQuat>(), b.Get<WQuat>(), static_cast<float>(x));
      out_res = q;
    }
    else if constexpr (CanInterpolate(static_cast<WVariantType::Enum>(WVariantTypeDeduction<T>::value)))
    {
      out_res = WMath::Lerp(a.Get<T>(), b.Get<T>(), static_cast<float>(x));
    }
    else
    {
      out_res = (x < 0.5) ? a : b;
    }
  }
};

namespace WMath
{
  WVariant Lerp(const WVariant& a, const WVariant& b, double fFactor)
  {
    LerpFunc func;
    WVariant result;
    WVariant::DispatchTo(func, a.GetType(), a, b, fFactor, result);
    return result;
  }
} // namespace WMath
