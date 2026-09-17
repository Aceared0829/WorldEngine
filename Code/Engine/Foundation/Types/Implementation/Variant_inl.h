
W_WARNING_PUSH()
W_WARNING_DISABLE_MSVC(4702) // Unreachable code for some reason

W_ALWAYS_INLINE WVariant::WVariant()
{
  m_uiType = Type::Invalid;
  m_bIsShared = false;
}

W_WARNING_POP()

W_WARNING_PUSH()
W_WARNING_DISABLE_CLANG("-Wunused-local-typedef")
W_WARNING_DISABLE_GCC("-Wunused-local-typedefs")

W_ALWAYS_INLINE WVariant::WVariant(const WVariant& other)
{
  CopyFrom(other);
}

W_ALWAYS_INLINE WVariant::WVariant(WVariant&& other) noexcept
{
  MoveFrom(std::move(other));
}

W_ALWAYS_INLINE WVariant::WVariant(const bool& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WInt8& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WUInt8& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WInt16& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WUInt16& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WInt32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WUInt32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WInt64& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WUInt64& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const float& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const double& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WColor& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec2& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec3& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec4& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec2I32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec3I32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec4I32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec2U32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec3U32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WVec4U32& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WQuat& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WTime& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WUuid& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WAngle& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WColorGammaUB& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WHashedString& value)
{
  InitInplace(value);
}

W_ALWAYS_INLINE WVariant::WVariant(const WTempHashedString& value)
{
  InitInplace(value);
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int>>
W_ALWAYS_INLINE WVariant::WVariant(const T& value)
{
  const constexpr bool forceSharing = TypeDeduction<T>::forceSharing;
  const constexpr bool inlineSized = sizeof(T) <= InlinedStruct::DataSize;
  const constexpr bool isPOD = WIsPodType<T>::value;
  InitTypedObject(value, WTraitInt < (!forceSharing && inlineSized && isPOD) ? 1 : 0 > ());
}

template <typename T>
W_ALWAYS_INLINE WVariant::WVariant(const T* value)
{
  constexpr bool bla = !std::is_same<T, void>::value;
  static_assert(bla);
  InitTypedPointer(const_cast<T*>(value), WGetStaticRTTI<T>());
}

W_ALWAYS_INLINE WVariant::WVariant(void* value, const WRTTI* pType)
{
  InitTypedPointer(value, pType);
}

W_ALWAYS_INLINE WVariant::~WVariant()
{
  Release();
}

W_ALWAYS_INLINE void WVariant::operator=(const WVariant& other)
{
  if (this != &other)
  {
    Release();
    CopyFrom(other);
  }
}

W_ALWAYS_INLINE void WVariant::operator=(WVariant&& other) noexcept
{
  if (this != &other)
  {
    Release();
    MoveFrom(std::move(other));
  }
}

template <typename T>
W_ALWAYS_INLINE void WVariant::operator=(const T& value)
{
  *this = WVariant(value);
}

template <typename T>
W_FORCE_INLINE bool WVariant::operator==(const T& other) const
{
  if (IsFloatingPoint())
  {
    if constexpr (TypeDeduction<T>::value > Type::Invalid && TypeDeduction<T>::value <= Type::Double)
    {
      return ConvertNumber<double>() == static_cast<double>(other);
    }

    return false;
  }
  else if (IsNumber())
  {
    if constexpr (TypeDeduction<T>::value > Type::Invalid && TypeDeduction<T>::value <= Type::Double)
    {
      return ConvertNumber<WInt64>() == static_cast<WInt64>(other);
    }

    return false;
  }

  if constexpr (std::is_same_v<T, WHashedString>)
  {
    if (m_uiType == Type::TempHashedString)
    {
      return Cast<WTempHashedString>() == other;
    }
  }
  else if constexpr (std::is_same_v<T, WTempHashedString>)
  {
    if (m_uiType == Type::HashedString)
    {
      return Cast<WHashedString>() == other;
    }
  }
  else if constexpr (std::is_same_v<T, WStringView>)
  {
    if (m_uiType == Type::String)
    {
      return Cast<WString>().GetView() == other;
    }
  }
  else if constexpr (std::is_same_v<T, WString>)
  {
    if (m_uiType == Type::StringView)
    {
      return Cast<WStringView>() == other.GetView();
    }
  }

  using StorageType = typename TypeDeduction<T>::StorageType;
  W_ASSERT_DEV(IsA<StorageType>(), "Stored type '{0}' does not match comparison type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<StorageType>() == other;
}

W_ALWAYS_INLINE bool WVariant::IsValid() const
{
  return m_uiType != Type::Invalid;
}

W_ALWAYS_INLINE bool WVariant::IsNumber() const
{
  return IsNumberStatic(m_uiType);
}

W_ALWAYS_INLINE bool WVariant::IsFloatingPoint() const
{
  return IsFloatingPointStatic(m_uiType);
}

W_ALWAYS_INLINE bool WVariant::IsString() const
{
  return IsStringStatic(m_uiType);
}

W_ALWAYS_INLINE bool WVariant::IsHashedString() const
{
  return IsHashedStringStatic(m_uiType);
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int>>
W_ALWAYS_INLINE bool WVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int>>
W_ALWAYS_INLINE bool WVariant::IsA() const
{
  if (m_uiType == TypeDeduction<T>::value)
  {
    const WTypedPointer& ptr = *reinterpret_cast<const WTypedPointer*>(&m_Data);
    // Always allow cast to void*.
    if constexpr (std::is_same<T, void*>::value || std::is_same<T, const void*>::value)
    {
      return true;
    }
    else if (ptr.m_pType)
    {
      using NonPointerT = typename WTypeTraits<T>::NonConstReferencePointerType;
      const WRTTI* pType = WGetStaticRTTI<NonPointerT>();
      return IsDerivedFrom(ptr.m_pType, pType);
    }
    else if (!ptr.m_pObject)
    {
      // nullptr can be converted to anything
      return true;
    }
  }
  return false;
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int>>
W_ALWAYS_INLINE bool WVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int>>
W_ALWAYS_INLINE bool WVariant::IsA() const
{
  using NonRefT = typename WTypeTraits<T>::NonConstReferenceType;
  if (m_uiType == TypeDeduction<T>::value)
  {
    if (const WRTTI* pType = GetReflectedType())
    {
      return IsDerivedFrom(pType, WGetStaticRTTI<NonRefT>());
    }
  }
  return false;
}

W_ALWAYS_INLINE WVariant::Type::Enum WVariant::GetType() const
{
  return static_cast<Type::Enum>(m_uiType);
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int>>
W_ALWAYS_INLINE const T& WVariant::Get() const
{
  W_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int>>
W_ALWAYS_INLINE T WVariant::Get() const
{
  W_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int>>
W_ALWAYS_INLINE const T WVariant::Get() const
{
  W_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int>>
W_ALWAYS_INLINE const T& WVariant::Get() const
{
  W_ASSERT_DEV(m_uiType == TypeDeduction<T>::value, "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int>>
W_ALWAYS_INLINE T& WVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int>>
W_ALWAYS_INLINE T WVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T>(Get<T>());
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int>>
W_ALWAYS_INLINE T& WVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

W_ALWAYS_INLINE const void* WVariant::GetData() const
{
  if (m_uiType == Type::TypedPointer)
  {
    return Cast<WTypedPointer>().m_pObject;
  }
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

template <typename T>
W_ALWAYS_INLINE bool WVariant::CanConvertTo() const
{
  return CanConvertTo(static_cast<Type::Enum>(TypeDeduction<T>::value));
}

template <typename T>
T WVariant::ConvertTo(WResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo<T>())
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = W_FAILURE;

    return T();
  }

  if (m_uiType == TypeDeduction<T>::value)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = W_SUCCESS;

    return Cast<T>();
  }

  T result = {};
  bool bSuccessful = true;
  WVariantHelper::To(*this, result, bSuccessful);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = bSuccessful ? W_SUCCESS : W_FAILURE;

  return result;
}


/// private methods

template <typename T>
W_FORCE_INLINE void WVariant::InitInplace(const T& value)
{
  static_assert(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  static_assert(WGetTypeClass<T>::value <= WTypeIsMemRelocatable::value, "in place data needs to be POD or mem relocatable");
  static_assert(sizeof(T) <= sizeof(m_Data), "value of this type is too big to bestored inline in a Variant");
  WMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);

  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = false;
}

template <typename T>
W_FORCE_INLINE void WVariant::InitTypedObject(const T& value, WTraitInt<0>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  static_assert((sizeof(StorageType) > sizeof(InlinedStruct::DataSize)) || TypeDeduction<T>::forceSharing, "Value should be inplace instead.");
  static_assert(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  const WRTTI* pType = WGetStaticRTTI<T>();
  m_Data.shared = W_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
W_FORCE_INLINE void WVariant::InitTypedObject(const T& value, WTraitInt<1>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  static_assert((sizeof(StorageType) <= InlinedStruct::DataSize) && !TypeDeduction<T>::forceSharing, "Value can't be stored inplace.");
  static_assert(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  static_assert(WIsPodType<T>::value, "in place data needs to be POD");
  WMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_Data.inlined.m_pType = WGetStaticRTTI<T>();
  m_uiType = Type::TypedObject;
  m_bIsShared = false;
}

inline void WVariant::Release()
{
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef.Decrement() == 0)
    {
      W_DEFAULT_DELETE(m_Data.shared);
    }
  }
}

inline void WVariant::CopyFrom(const WVariant& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;

  if (m_bIsShared)
  {
    m_Data.shared = other.m_Data.shared;
    m_Data.shared->m_uiRef.Increment();
  }
  else if (other.IsValid())
  {
    m_Data = other.m_Data;
  }
}

W_ALWAYS_INLINE void WVariant::MoveFrom(WVariant&& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;
  m_Data = other.m_Data;

  other.m_uiType = Type::Invalid;
  other.m_bIsShared = false;
  other.m_Data.shared = nullptr;
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int>>
const T& WVariant::Cast() const
{
  const bool validType = WConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  static_assert(validType, "Invalid Cast, can only cast to storage type");

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int>>
T WVariant::Cast() const
{
  const WTypedPointer& ptr = *reinterpret_cast<const WTypedPointer*>(&m_Data);

  const WRTTI* pType = GetReflectedType();
  W_IGNORE_UNUSED(pType);
  using NonRefPtrT = typename WTypeTraits<T>::NonConstReferencePointerType;
  if constexpr (!std::is_same<T, void*>::value && !std::is_same<T, const void*>::value)
  {
    W_ASSERT_DEV(pType == nullptr || IsDerivedFrom(pType, WGetStaticRTTI<NonRefPtrT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(WGetStaticRTTI<NonRefPtrT>()));
  }
  return static_cast<T>(ptr.m_pObject);
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int>>
const T WVariant::Cast() const
{
  WTypedObject obj;
  obj.m_pObject = GetData();
  obj.m_pType = GetReflectedType();
  return obj;
}

template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int>>
const T& WVariant::Cast() const
{
  const WRTTI* pType = GetReflectedType();
  W_IGNORE_UNUSED(pType);
  using NonRefT = typename WTypeTraits<T>::NonConstReferenceType;
  W_ASSERT_DEV(IsDerivedFrom(pType, WGetStaticRTTI<NonRefT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(WGetStaticRTTI<NonRefT>()));

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

W_ALWAYS_INLINE bool WVariant::IsNumberStatic(WUInt32 type)
{
  return type > Type::FirstStandardType && type <= Type::Double;
}

W_ALWAYS_INLINE bool WVariant::IsFloatingPointStatic(WUInt32 type)
{
  return type == Type::Float || type == Type::Double;
}

W_ALWAYS_INLINE bool WVariant::IsStringStatic(WUInt32 type)
{
  return type == Type::String || type == Type::StringView;
}

W_ALWAYS_INLINE bool WVariant::IsHashedStringStatic(WUInt32 type)
{
  return type == Type::HashedString || type == Type::TempHashedString;
}

W_ALWAYS_INLINE bool WVariant::IsVector2Static(WUInt32 type)
{
  return type == Type::Vector2 || type == Type::Vector2I || type == Type::Vector2U;
}

W_ALWAYS_INLINE bool WVariant::IsVector3Static(WUInt32 type)
{
  return type == Type::Vector3 || type == Type::Vector3I || type == Type::Vector3U;
}

W_ALWAYS_INLINE bool WVariant::IsVector4Static(WUInt32 type)
{
  return type == Type::Vector4 || type == Type::Vector4I || type == Type::Vector4U;
}

template <typename T>
T WVariant::ConvertNumber() const
{
  switch (m_uiType)
  {
    case Type::Bool:
      return static_cast<T>(Cast<bool>());
    case Type::Int8:
      return static_cast<T>(Cast<WInt8>());
    case Type::UInt8:
      return static_cast<T>(Cast<WUInt8>());
    case Type::Int16:
      return static_cast<T>(Cast<WInt16>());
    case Type::UInt16:
      return static_cast<T>(Cast<WUInt16>());
    case Type::Int32:
      return static_cast<T>(Cast<WInt32>());
    case Type::UInt32:
      return static_cast<T>(Cast<WUInt32>());
    case Type::Int64:
      return static_cast<T>(Cast<WInt64>());
    case Type::UInt64:
      return static_cast<T>(Cast<WUInt64>());
    case Type::Float:
      return static_cast<T>(Cast<float>());
    case Type::Double:
      return static_cast<T>(Cast<double>());
  }

  W_REPORT_FAILURE("Variant is not a number");
  return T(0);
}

template <>
struct WHashHelper<WVariant>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WVariant& value)
  {
    WUInt64 uiHash = value.ComputeHash(0);
    return (WUInt32)uiHash;
  }

  W_ALWAYS_INLINE static bool Equal(const WVariant& a, const WVariant& b)
  {
    return a.GetType() == b.GetType() && a == b;
  }
};

W_WARNING_POP()
