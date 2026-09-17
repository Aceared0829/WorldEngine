#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Types/TypedPointer.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/VariantType.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Utilities/ConversionUtils.h>

class WRTTI;

/// Defines a reference to an immutable object owned by an WVariant.
///
/// Used to store custom types inside an WVariant. As lifetime is governed by the WVariant, it is generally not safe to store an WTypedObject.
/// This class is needed to be able to differentiate between WVariantType::TypedPointer and WVariantType::TypedObject e.g. in WVariant::DispatchTo.
/// \sa WVariant, W_DECLARE_CUSTOM_VARIANT_TYPE
struct WTypedObject
{
  W_DECLARE_POD_TYPE();
  const void* m_pObject = nullptr;
  const WRTTI* m_pType = nullptr;

  bool operator==(const WTypedObject& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WTypedObject&);
};

/// WVariant is a class that can store different types of variables, which is useful in situations where it is not clear up front,
/// which type of data will be passed around.
///
/// The variant supports a fixed list of types that it can store (\see WVariant::Type). All types of 16 bytes or less in size can be stored
/// without requiring a heap allocation. For larger types memory is allocated on the heap. In general variants should be used for code that
/// needs to be flexible. Although WVariant is implemented very efficiently, it should be avoided to use WVariant in code that needs to be
/// fast.
class W_FOUNDATION_DLL WVariant
{
public:
  using Type = WVariantType;
  template <typename T>
  using TypeDeduction = WVariantTypeDeduction<T>;

  /// helper struct to wrap a string pointer
  struct StringWrapper
  {
    W_ALWAYS_INLINE StringWrapper(const char* szStr)
      : m_str(szStr)
    {
    }
    const char* m_str;
  };

  /// Initializes the variant to be 'Invalid'
  WVariant(); // [tested]

  /// Copies the data from the other variant.
  ///
  /// \note If the data of the variant needed to be allocated on the heap, it will be shared among variants.
  /// Thus, once you have stored such a type inside a variant, you can copy it to other variants, without introducing
  /// additional memory allocations.
  WVariant(const WVariant& other); // [tested]

  /// Moves the data from the other variant.
  WVariant(WVariant&& other) noexcept; // [tested]

  WVariant(const bool& value);
  WVariant(const WInt8& value);
  WVariant(const WUInt8& value);
  WVariant(const WInt16& value);
  WVariant(const WUInt16& value);
  WVariant(const WInt32& value);
  WVariant(const WUInt32& value);
  WVariant(const WInt64& value);
  WVariant(const WUInt64& value);
  WVariant(const float& value);
  WVariant(const double& value);
  WVariant(const WColor& value);
  WVariant(const WVec2& value);
  WVariant(const WVec3& value);
  WVariant(const WVec4& value);
  WVariant(const WVec2I32& value);
  WVariant(const WVec3I32& value);
  WVariant(const WVec4I32& value);
  WVariant(const WVec2U32& value);
  WVariant(const WVec3U32& value);
  WVariant(const WVec4U32& value);
  WVariant(const WQuat& value);
  WVariant(const WMat3& value);
  WVariant(const WMat4& value);
  WVariant(const WTransform& value);
  WVariant(const char* value);
  WVariant(const WString& value);
  WVariant(const WUntrackedString& value);
  WVariant(const WStringView& value, bool bCopyString = true);
  WVariant(const WDataBuffer& value);
  WVariant(const WTime& value);
  WVariant(const WUuid& value);
  WVariant(const WAngle& value);
  WVariant(const WColorGammaUB& value);
  WVariant(const WHashedString& value);
  WVariant(const WTempHashedString& value);

  WVariant(const WVariantArray& value);
  WVariant(const WVariantDictionary& value);

  WVariant(const WTypedPointer& value);
  WVariant(const WTypedObject& value);

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int> = 0>
  WVariant(const T& value);

  template <typename T>
  WVariant(const T* value);

  /// Initializes to a TypedPointer of the given object and type.
  WVariant(void* value, const WRTTI* pType);

  /// Initializes to a TypedObject by cloning the given object and type.
  void CopyTypedObject(const void* value, const WRTTI* pType); // [tested]

  /// Initializes to a TypedObject by taking ownership of the given object and type.
  void MoveTypedObject(void* value, const WRTTI* pType); // [tested]

  /// If necessary, this will deallocate any heap memory that is not in use any more.
  ~WVariant();

  /// Copies the data from the \a other variant into this one.
  void operator=(const WVariant& other); // [tested]

  /// Moves the data from the \a other variant into this one.
  void operator=(WVariant&& other) noexcept; // [tested]

  /// Deduces the type of \a T and stores \a value.
  ///
  /// If the type to be stored in the variant is not supported, a compile time error will occur.
  template <typename T>
  void operator=(const T& value); // [tested]

  /// Will compare the value of this variant to that of \a other.
  ///
  /// If both variants store 'numbers' (float, double, int types) the comparison will work, even if the types are not identical.
  ///
  /// \note If the two types are not numbers and not equal, an assert will occur. So be careful to only compare variants
  /// that can either both be converted to double (\see CanConvertTo()) or whose types are equal.
  bool operator==(const WVariant& other) const; // [tested]

  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WVariant&);

  /// See non-templated operator==
  template <typename T>
  bool operator==(const T& other) const; // [tested]

#if W_DISABLED(W_USE_CPP20_OPERATORS)
  /// See non-templated operator!=
  template <typename T>
  bool operator!=(const T& other) const // [tested]
  {
    return !(*this == other);
  }
#endif

  /// Returns whether this variant stores any other type than 'Invalid'.
  bool IsValid() const; // [tested]

  /// Returns whether the stored type is numerical type either integer or floating point.
  ///
  /// Bool counts as number.
  bool IsNumber() const; // [tested]

  /// Returns whether the stored type is floating point (float or double).
  bool IsFloatingPoint() const; // [tested]

  /// Returns whether the stored type is a string (WString or WStringView).
  bool IsString() const; // [tested]

  /// Returns whether the stored type is a hashed string (WHashedString or WTempHashedString).
  bool IsHashedString() const;

  /// Returns whether the stored type is exactly the given type.
  ///
  /// \note This explicitly also differentiates between the different integer types.
  /// So when the variant stores an Int32, IsA<Int64>() will return false, even though the types could be converted.
  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int> = 0>
  bool IsA() const; // [tested]

  /// Returns the exact WVariant::Type value.
  Type::Enum GetType() const; // [tested]

  /// Returns the variants value as the provided type.
  ///
  /// \note This function does not do ANY type of conversion from the stored type to the given type. Not even integer conversions!
  /// If the types don't match, this function will assert!
  /// So be careful to use this function only when you know exactly that the stored type matches the expected type.
  ///
  /// Prefer to use ConvertTo() when you can instead.
  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int> = 0>
  const T& Get() const; // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int> = 0>
  T Get() const;        // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int> = 0>
  const T Get() const;  // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int> = 0>
  const T& Get() const; // [tested]

  /// Returns an writable WTypedPointer to the internal data.
  /// If the data is currently shared a clone will be made to ensure we hold the only reference.
  WTypedPointer GetWriteAccess(); // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int> = 0>
  T& GetWritable();                // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int> = 0>
  T GetWritable();                 // [tested]

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int> = 0>
  T& GetWritable();                // [tested]


  /// Returns a const void* to the internal data.
  /// For TypedPointer and TypedObject this will return a pointer to the target object.
  const void* GetData() const; // [tested]

  /// Returns the WRTTI type of the held value.
  /// For TypedPointer and TypedObject this will return the type of the target object.
  const WRTTI* GetReflectedType() const; // [tested]

  /// Returns the sub value at iIndex. This could be an element in an array or a member property inside a reflected type.
  ///
  /// Out of bounds access is handled gracefully and will return an invalid variant.
  const WVariant operator[](WUInt32 uiIndex) const; // [tested]

  /// Returns the sub value with szKey. This could be a value in a dictionary or a member property inside a reflected type.
  ///
  /// This function will return an invalid variant if no corresponding sub value is found.
  const WVariant operator[](StringWrapper key) const; // [tested]

  /// Returns whether the stored type can generally be converted to the desired type.
  ///
  /// This function will return true for all number conversions, as float / double / int / etc. can generally be converted into each
  /// other. It will also return true for all conversion from string to number types, and from all 'simple' types (not array or dictionary)
  /// to string.
  ///
  /// \note This function only returns whether a conversion between the stored TYPE and the desired TYPE is generally possible. It does NOT
  /// return whether the stored VALUE is indeed convertible to the desired type. For example, a string is generally convertible to float, if
  /// it stores a string representation of a float value. If, however, it stores anything else, the conversion can still fail.
  ///
  /// The only way to figure out whether the stored data can be converted to some type, is to actually convert it, using ConvertTo(), and
  /// then to check the conversion status.
  template <typename T>
  bool CanConvertTo() const; // [tested]

  /// Same as the templated CanConvertTo function.
  bool CanConvertTo(Type::Enum type) const; // [tested]

  /// Tries to convert the stored value to the given type. The optional status parameter can be used to check whether the conversion
  /// succeeded.
  ///
  /// When CanConvertTo() returns false, ConvertTo() will also always fail. However, when CanConvertTo() returns true, this is no guarantee
  /// that ConvertTo() will succeed. Conversion between numbers and to strings will generally succeed. However, converting from a string to
  /// another type can fail or succeed, depending on the exact string value.
  template <typename T>
  T ConvertTo(WResult* out_pConversionStatus = nullptr) const; // [tested]

  /// Same as the templated function.
  WVariant ConvertTo(Type::Enum type, WResult* out_pConversionStatus = nullptr) const; // [tested]

  /// This will call the overloaded operator() (function call operator) of the provided functor.
  ///
  /// This allows to implement a functor that overloads operator() for different types and then call the proper version of that operator,
  /// depending on the provided runtime type. Note that the proper overload of operator() is selected by providing a dummy type, but it will
  /// contain no useful value. Instead, store the other necessary data inside the functor object, before calling this function. For example,
  /// store a pointer to a variant inside the functor object and then call DispatchTo to execute the function that will handle the given
  /// type of the variant.
  template <typename Functor, class... Args>
  static auto DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args); // [tested]

  /// Computes the hash value of the stored data. Returns uiSeed (unchanged) for an invalid Variant.
  WUInt64 ComputeHash(WUInt64 uiSeed = 0) const;

private:
  friend class WVariantHelper;
  friend struct CompareFunc;
  friend struct GetTypeFromVariantFunc;

  struct SharedData
  {
    void* m_Ptr;
    const WRTTI* m_pType;
    WAtomicInteger32 m_uiRef = 1;
    W_ALWAYS_INLINE SharedData(void* pPtr, const WRTTI* pType)
      : m_Ptr(pPtr)
      , m_pType(pType)
    {
    }
    virtual ~SharedData() = default;
    virtual SharedData* Clone() const = 0;
  };

  template <typename T>
  class TypedSharedData : public SharedData
  {
  private:
    T m_t;

  public:
    W_ALWAYS_INLINE TypedSharedData(const T& value, const WRTTI* pType = nullptr)
      : SharedData(&m_t, pType)
      , m_t(value)
    {
    }

    virtual SharedData* Clone() const override
    {
      return W_DEFAULT_NEW(TypedSharedData<T>, m_t, m_pType);
    }
  };

  class RTTISharedData : public SharedData
  {
  public:
    RTTISharedData(void* pData, const WRTTI* pType);

    ~RTTISharedData();

    virtual SharedData* Clone() const override;
  };

  struct InlinedStruct
  {
    constexpr static int DataSize = 4 * sizeof(float) - sizeof(void*);
    WUInt8 m_Data[DataSize];
    const WRTTI* m_pType;
  };

  union Data
  {
    float f[4];
    SharedData* shared;
    InlinedStruct inlined;
  } m_Data;

  WUInt32 m_uiType : 31;
  WUInt32 m_bIsShared : 1; // NOLINT(W*)

  template <typename T>
  void InitInplace(const T& value);

  template <typename T>
  void InitShared(const T& value);

  template <typename T>
  void InitTypedObject(const T& value, WTraitInt<0>);
  template <typename T>
  void InitTypedObject(const T& value, WTraitInt<1>);

  void InitTypedPointer(void* value, const WRTTI* pType);

  void Release();
  void CopyFrom(const WVariant& other);
  void MoveFrom(WVariant&& other);

  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::DirectCast, int> = 0>
  const T& Cast() const;
  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::PointerCast, int> = 0>
  T Cast() const;
  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::TypedObject, int> = 0>
  const T Cast() const;
  template <typename T, typename std::enable_if_t<WVariantTypeDeduction<T>::classification == WVariantClass::CustomTypeCast, int> = 0>
  const T& Cast() const;

  static bool IsNumberStatic(WUInt32 type);
  static bool IsFloatingPointStatic(WUInt32 type);
  static bool IsStringStatic(WUInt32 type);
  static bool IsHashedStringStatic(WUInt32 type);
  static bool IsVector2Static(WUInt32 type);
  static bool IsVector3Static(WUInt32 type);
  static bool IsVector4Static(WUInt32 type);

  // Needed to prevent including WRTTI in WVariant.h
  static bool IsDerivedFrom(const WRTTI* pType1, const WRTTI* pType2);
  static WStringView GetTypeName(const WRTTI* pType);

  template <typename T>
  T ConvertNumber() const;
};

/// An overload of WDynamicCast for dynamic casting a variant to a pointer type.
///
/// If the WVariant stores an WTypedPointer pointer, this pointer will be dynamically cast to T*.
/// If the WVariant stores any other type (or nothing), nullptr is returned.
template <typename T>
W_ALWAYS_INLINE T WDynamicCast(const WVariant& variant)
{
  if (variant.IsA<T>())
  {
    return variant.Get<T>();
  }

  return nullptr;
}

// Simple math operator overloads. An invalid variant is returned if the given variants have incompatible types.
W_FOUNDATION_DLL WVariant operator+(const WVariant& a, const WVariant& b);
W_FOUNDATION_DLL WVariant operator-(const WVariant& a, const WVariant& b);
W_FOUNDATION_DLL WVariant operator*(const WVariant& a, const WVariant& b);
W_FOUNDATION_DLL WVariant operator/(const WVariant& a, const WVariant& b);

namespace WMath
{
  /// An overload of WMath::Lerp to interpolate variants. A and b must have the same type.
  ///
  /// If the type can't be interpolated like e.g. strings, a is returned for a fFactor less than 0.5, b is returned for a fFactor greater or equal to 0.5.
  W_FOUNDATION_DLL WVariant Lerp(const WVariant& a, const WVariant& b, double fFactor);
} // namespace WMath

#include <Foundation/Types/Implementation/VariantHelper_inl.h>

#include <Foundation/Types/Implementation/Variant_inl.h>
