#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// Adds dynamic reflection capabilities to a class declaration.
///
/// This macro must be placed in the class declaration of every type that needs dynamic reflection.
/// It adds the necessary infrastructure for runtime type identification, including static RTTI
/// storage and helper typedefs. The class must derive from WReflectedClass (directly or indirectly)
/// to provide the virtual GetDynamicRTTI() method.
///
/// Unlike W_ADD_DYNAMIC_REFLECTION, this variant does not automatically implement GetDynamicRTTI(),
/// allowing for custom implementations or abstract base classes.
#define W_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE) \
  W_ALLOW_PRIVATE_PROPERTIES(SELF);                         \
                                                             \
public:                                                      \
  using OWNTYPE = SELF;                                      \
  using SUPER = BASE_TYPE;                                   \
  W_ALWAYS_INLINE static const WRTTI* GetStaticRTTI()      \
  {                                                          \
    return &SELF::s_RTTI;                                    \
  }                                                          \
                                                             \
private:                                                     \
  static WRTTI s_RTTI;                                      \
  W_REFLECTION_DEBUG_CODE


#define W_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)      \
  W_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)  \
public:                                                 \
  virtual const WRTTI* GetDynamicRTTI() const override \
  {                                                     \
    return &SELF::s_RTTI;                               \
  }


#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT) && W_ENABLED(W_COMPILER_MSVC)

#  define W_REFLECTION_DEBUG_CODE                       \
    static const WRTTI* ReflectionDebug_GetParentType() \
    {                                                    \
      return __super::GetStaticRTTI();                   \
    }

#  define W_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#  define W_REFLECTION_DEBUG_CODE /*empty*/
#  define W_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// Begins the implementation block for dynamic reflection of a type.
///
/// This macro starts the definition of the static RTTI object for a type, enabling
/// runtime type information, property access, and dynamic instantiation. Must be
/// paired with W_END_DYNAMIC_REFLECTED_TYPE in the implementation file.
///
/// \param Type
///   The type being reflected. Must have used W_ADD_DYNAMIC_REFLECTION in its declaration.
/// \param Version
///   Version number for serialization compatibility. Increment when changing reflection data.
/// \param AllocatorType
///   Controls dynamic instantiation capability:
///   - WRTTINoAllocator: Type cannot be instantiated dynamically (abstract/interface types)
///   - WRTTIDefaultAllocator<Type>: Standard heap allocation for concrete types
///   - Custom allocator: Specialized allocation strategy for pool/stack allocated types
#define W_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType) \
  W_RTTIINFO_DECL(Type, Type::SUPER, Version)                        \
  WRTTI Type::s_RTTI = GetRTTI((Type*)0);                            \
  W_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, Type::SUPER, AllocatorType)

/// Ends the reflection code block that was opened with W_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define W_END_DYNAMIC_REFLECTED_TYPE                                                                                                \
  return WRTTI(GetTypeName((OwnType*)0), WGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),              \
    WVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, \
    W_REFLECTION_DEBUG_GETPARENTFUNC);                                                                                              \
  }

/// Same as W_BEGIN_DYNAMIC_REFLECTED_TYPE but forces the type to be treated as abstract by reflection even though
/// it might not be abstract from a C++ perspective.
#define W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(Type, Version)     \
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, WRTTINoAllocator) \
    flags.Add(WTypeFlags::Abstract);

#define W_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE W_END_DYNAMIC_REFLECTED_TYPE

/// Base class for all types that support dynamic reflection and runtime type identification.
///
/// This class provides the fundamental virtual interface for runtime type queries and
/// the foundational reflection infrastructure. All types that need dynamic reflection
/// must derive from this class, either directly or through an inheritance chain.
///
/// Key capabilities provided:
/// - Virtual GetDynamicRTTI() for runtime type identification
/// - IsInstanceOf() for type checking and inheritance queries
/// - Foundation for property access, serialization, and other reflection features
class W_FOUNDATION_DLL WReflectedClass : public WNoBase
{
  W_ADD_DYNAMIC_REFLECTION_NO_GETTER(WReflectedClass, WNoBase);

public:
  virtual const WRTTI* GetDynamicRTTI() const { return &WReflectedClass::s_RTTI; }

public:
  W_ALWAYS_INLINE WReflectedClass() = default;
  W_ALWAYS_INLINE virtual ~WReflectedClass() = default;

  /// Returns whether the type of this instance is of the given type or derived from it.
  bool IsInstanceOf(const WRTTI* pType) const;

  /// Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  W_ALWAYS_INLINE bool IsInstanceOf() const
  {
    const WRTTI* pType = WGetStaticRTTI<T>();
    return IsInstanceOf(pType);
  }
};
