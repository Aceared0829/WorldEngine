#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include TypeTraits.h directly, but instead include Foundation/Basics.h"
#endif

/// \file

/// Compile-time type classification system for optimizing container operations.
///
/// The WorldEngine type trait system classifies types into three categories to enable
/// different optimization strategies for containers and memory operations:
///
/// - Class (0): Standard types requiring constructor/destructor calls and careful copying
/// - POD (1): Plain Old Data types that can be memcpy'd and don't need destructor calls
/// - MemRelocatable (2): Types that can be moved with memcpy but may need destructor calls
///
/// This classification allows containers to choose the most efficient implementation
/// for construction, destruction, copying, and moving operations.
template <int v>
struct WTraitInt
{
  static constexpr int value = v;
};

using WTypeIsMemRelocatable = WTraitInt<2>; ///< Types that can be moved with memcpy
using WTypeIsPod = WTraitInt<1>;            ///< Plain Old Data types
using WTypeIsClass = WTraitInt<0>;          ///< Standard class types

using WCompileTimeTrueType = char;
using WCompileTimeFalseType = int;

/// Converts a bool condition to CompileTimeTrue/FalseType
template <bool cond>
struct WConditionToCompileTimeBool
{
  using type = WCompileTimeFalseType;
};

template <>
struct WConditionToCompileTimeBool<true>
{
  using type = WCompileTimeTrueType;
};

/// Default % operator for T and TypeIsPod which returns a CompileTimeFalseType.
template <typename T>
WCompileTimeFalseType operator%(const T&, const WTypeIsPod&);

/// If there is an % operator which takes a TypeIsPod and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct WIsPodType : public WTraitInt<(sizeof(*((T*)0) % *((const WTypeIsPod*)0)) == sizeof(WCompileTimeTrueType)) ? 1 : 0>
{
};

/// Pointers are POD types.
template <typename T>
struct WIsPodType<T*> : public WTypeIsPod
{
};

/// arrays are POD types
template <typename T, int N>
struct WIsPodType<T[N]> : public WTypeIsPod
{
};

/// Default % operator for T and WTypeIsMemRelocatable which returns a CompileTimeFalseType.
template <typename T>
WCompileTimeFalseType operator%(const T&, const WTypeIsMemRelocatable&);

/// If there is an % operator which takes a WTypeIsMemRelocatable and returns a CompileTimeTrueType T is Pod. Default % operator
/// return false.
template <typename T>
struct WGetTypeClass
  : public WTraitInt<(sizeof(*((T*)0) % *((const WTypeIsMemRelocatable*)0)) == sizeof(WCompileTimeTrueType)) ? 2 : WIsPodType<T>::value>
{
};

/// Static Conversion Test
template <typename From, typename To>
struct WConversionTest
{
  static WCompileTimeTrueType Test(const To&);
  static WCompileTimeFalseType Test(...);
  static From MakeFrom();

  enum
  {
    exists = sizeof(Test(MakeFrom())) == sizeof(WCompileTimeTrueType),
    sameType = 0
  };
};

/// Specialization for above Type.
template <typename T>
struct WConversionTest<T, T>
{
  enum
  {
    exists = 1,
    sameType = 1
  };
};

// remapping of the 0 (not special) type to 3
template <typename T1, typename T2>
struct WGetStrongestTypeClass : public WTraitInt<(T1::value == 0 || T2::value == 0) ? 0 : W_COMPILE_TIME_MAX(T1::value, T2::value)>
{
};


#ifdef __INTELLISENSE__

/// Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define W_DECLARE_POD_TYPE()

/// Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define W_DECLARE_MEM_RELOCATABLE_TYPE()

/// mark a class as memory relocatable if the passed type is relocatable or pod.
#  define W_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)

// embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define W_DETECT_TYPE_CLASS(...)

#else

/// Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define W_DECLARE_POD_TYPE()                               \
    WCompileTimeTrueType operator%(const WTypeIsPod&) const \
    {                                                         \
      return {};                                              \
    }

/// Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define W_DECLARE_MEM_RELOCATABLE_TYPE()                              \
    WCompileTimeTrueType operator%(const WTypeIsMemRelocatable&) const \
    {                                                                    \
      return {};                                                         \
    }

/// mark a class as memory relocatable if the passed type is relocatable or pod.
#  define W_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)                                                                                       \
    typename WConditionToCompileTimeBool<WGetTypeClass<T>::value == WTypeIsMemRelocatable::value || WIsPodType<T>::value>::type operator%( \
      const WTypeIsMemRelocatable&) const                                                                                                     \
    {                                                                                                                                          \
      return {};                                                                                                                               \
    }

#  define W_DETECT_TYPE_CLASS_1(T1) WGetTypeClass<T1>
#  define W_DETECT_TYPE_CLASS_2(T1, T2) WGetStrongestTypeClass<W_DETECT_TYPE_CLASS_1(T1), W_DETECT_TYPE_CLASS_1(T2)>
#  define W_DETECT_TYPE_CLASS_3(T1, T2, T3) WGetStrongestTypeClass<W_DETECT_TYPE_CLASS_2(T1, T2), W_DETECT_TYPE_CLASS_1(T3)>
#  define W_DETECT_TYPE_CLASS_4(T1, T2, T3, T4) WGetStrongestTypeClass<W_DETECT_TYPE_CLASS_2(T1, T2), W_DETECT_TYPE_CLASS_2(T3, T4)>
#  define W_DETECT_TYPE_CLASS_5(T1, T2, T3, T4, T5) WGetStrongestTypeClass<W_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), W_DETECT_TYPE_CLASS_1(T5)>
#  define W_DETECT_TYPE_CLASS_6(T1, T2, T3, T4, T5, T6) \
    WGetStrongestTypeClass<W_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), W_DETECT_TYPE_CLASS_2(T5, T6)>

// embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define W_DETECT_TYPE_CLASS(...)                                                                                                   \
    WCompileTimeTrueType operator%(                                                                                                  \
      const WTraitInt<W_CALL_MACRO(W_PP_CONCAT(W_DETECT_TYPE_CLASS_, W_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))::value>&) const \
    {                                                                                                                                 \
      return {};                                                                                                                      \
    }
#endif

/// Defines a type T as Pod.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define W_DEFINE_AS_POD_TYPE(T)             \
  template <>                                \
  struct WIsPodType<T> : public WTypeIsPod \
  {                                          \
  }

W_DEFINE_AS_POD_TYPE(bool);
W_DEFINE_AS_POD_TYPE(float);
W_DEFINE_AS_POD_TYPE(double);

W_DEFINE_AS_POD_TYPE(char);
W_DEFINE_AS_POD_TYPE(WInt8);
W_DEFINE_AS_POD_TYPE(WInt16);
W_DEFINE_AS_POD_TYPE(WInt32);
W_DEFINE_AS_POD_TYPE(WInt64);
W_DEFINE_AS_POD_TYPE(WUInt8);
W_DEFINE_AS_POD_TYPE(WUInt16);
W_DEFINE_AS_POD_TYPE(WUInt32);
W_DEFINE_AS_POD_TYPE(WUInt64);
W_DEFINE_AS_POD_TYPE(wchar_t);
W_DEFINE_AS_POD_TYPE(unsigned long);
W_DEFINE_AS_POD_TYPE(long);
W_DEFINE_AS_POD_TYPE(std::byte);

/// Checks inheritance at compile time.
#define W_IS_DERIVED_FROM_STATIC(BaseClass, DerivedClass) \
  (WConversionTest<const DerivedClass*, const BaseClass*>::exists && !WConversionTest<const BaseClass*, const void*>::sameType)

/// Checks whether A and B are the same type
#define W_IS_SAME_TYPE(TypeA, TypeB) WConversionTest<TypeA, TypeB>::sameType

/// Utility template for extracting clean types from decorated types.
template <typename T>
struct WTypeTraits
{
  /// Removes const qualifier: const int -> int
  using NonConstType = typename std::remove_const<T>::type;

  /// Removes reference qualifier: int& -> int, int&& -> int
  using NonReferenceType = typename std::remove_reference<T>::type;

  /// Removes pointer qualifier: int* -> int
  using NonPointerType = typename std::remove_pointer<T>::type;

  /// Removes both reference and const qualifiers: const int& -> int
  using NonConstReferenceType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

  /// Removes both reference and pointer qualifiers: int*& -> int
  using NonReferencePointerType = typename std::remove_pointer<typename std::remove_reference<T>::type>::type;

  /// Removes reference, const, and pointer qualifiers from the pointed-to type.
  ///
  /// Note: This operates on the pointed-to type, not the pointer itself.
  /// Example: const int*& -> int (not int*)
  using NonConstReferencePointerType = typename std::remove_const<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::type;
};

/// generates a template named 'checkerName' which checks for the existence of a member function with
/// the name 'functionName' and the signature 'Signature'
#define W_MAKE_MEMBERFUNCTION_CHECKER(functionName, checkerName)                \
  template <typename T, typename Signature>                                      \
  struct checkerName                                                             \
  {                                                                              \
    template <typename U, U>                                                     \
    struct type_check;                                                           \
    template <typename O>                                                        \
    static WCompileTimeTrueType& chk(type_check<Signature, &O::functionName>*); \
    template <typename>                                                          \
    static WCompileTimeFalseType& chk(...);                                     \
    enum                                                                         \
    {                                                                            \
      value = (sizeof(chk<T>(0)) == sizeof(WCompileTimeTrueType)) ? 1 : 0       \
    };                                                                           \
  }
