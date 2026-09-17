#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

// *****************************************
// ***** Runtime Type Information Data *****

struct WRTTIAllocator;
class WAbstractProperty;
class WAbstractFunctionProperty;
class WAbstractMessageHandler;
struct WMessageSenderInfo;
class WPropertyAttribute;
class WMessage;
using WMessageId = WUInt16;

/// Core class of WorldEngine's reflection system that holds complete runtime type information.
///
/// Each WRTTI instance represents one reflected type and contains all metadata needed for runtime
/// introspection, serialization, and object creation. The reflection system enables:
///
/// - Runtime type queries and inheritance checks
/// - Dynamic property access and modification
/// - Object serialization and deserialization
/// - Message dispatching and handling
/// - Dynamic object creation and destruction
/// - Editor property grids and tools
///
/// Key features:
/// - Complete type hierarchy information with fast inheritance checks
/// - Property metadata including types, attributes, and access methods
/// - Message handling system for decoupled communication
/// - Allocator integration for controlled object creation
/// - Plugin-aware type registration for modular systems
/// - Version tracking for data migration and compatibility
///
/// Performance considerations:
/// - Type lookups are O(log n) using hash tables
/// - Inheritance checks are O(1) using pre-computed hierarchy arrays
/// - Property access involves virtual calls and type conversions
/// - Message dispatching uses optimized jump tables
///
/// Usage: Types are typically registered using macros (W_BEGIN_STATIC_REFLECTED_TYPE, etc.)
/// rather than creating WRTTI instances manually.
class W_FOUNDATION_DLL WRTTI
{
public:
  /// The constructor requires all the information about the type that this object represents.
  WRTTI(WStringView sName, const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType,
    WBitflags<WTypeFlags> flags, WRTTIAllocator* pAllocator, WArrayPtr<const WAbstractProperty*> properties, WArrayPtr<const WAbstractFunctionProperty*> functions,
    WArrayPtr<const WPropertyAttribute*> attributes, WArrayPtr<WAbstractMessageHandler*> messageHandlers,
    WArrayPtr<WMessageSenderInfo> messageSenders, const WRTTI* (*fnVerifyParent)());


  ~WRTTI();

  /// Can be called in debug builds to check that all reflected objects are correctly set up.
  void VerifyCorrectness() const;

  /// Calls VerifyCorrectness() on all WRTTI objects.
  static void VerifyCorrectnessForAllTypes();

  /// Returns the name of this type.
  W_ALWAYS_INLINE WStringView GetTypeName() const { return m_sTypeName; } // [tested]

  /// Returns the hash of the name of this type.
  W_ALWAYS_INLINE WUInt64 GetTypeNameHash() const { return m_uiTypeNameHash; } // [tested]

  /// Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  W_ALWAYS_INLINE const WRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// Returns the corresponding variant type for this type or Invalid if there is none.
  W_ALWAYS_INLINE WVariantType::Enum GetVariantType() const { return static_cast<WVariantType::Enum>(m_uiVariantType); }

  /// Fast O(1) check if this type is derived from the given type (or is the same type).
  ///
  /// Uses pre-computed parent hierarchy arrays for constant-time inheritance checks.
  /// Returns true if this type inherits from pBaseType or if they are the same type.
  /// This is the preferred method for inheritance testing in performance-critical code.
  W_ALWAYS_INLINE bool IsDerivedFrom(const WRTTI* pBaseType) const // [tested]
  {
    const WUInt32 thisGeneration = m_ParentHierarchy.GetCount();
    const WUInt32 baseGeneration = pBaseType->m_ParentHierarchy.GetCount();
    W_ASSERT_DEBUG(thisGeneration > 0 && baseGeneration > 0, "SetupParentHierarchy() has not been called");
    return thisGeneration >= baseGeneration && m_ParentHierarchy.GetData()[thisGeneration - baseGeneration] == pBaseType;
  }

  /// Returns true if this type is derived from or identical to the given type.
  template <typename BASE>
  W_ALWAYS_INLINE bool IsDerivedFrom() const // [tested]
  {
    return IsDerivedFrom(WGetStaticRTTI<BASE>());
  }

  /// Returns the object through which instances of this type can be allocated.
  W_ALWAYS_INLINE WRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// Returns the array of properties that this type has. Does NOT include properties from base classes.
  W_ALWAYS_INLINE WArrayPtr<const WAbstractProperty* const> GetProperties() const { return m_Properties; } // [tested]

  W_ALWAYS_INLINE WArrayPtr<const WAbstractFunctionProperty* const> GetFunctions() const { return m_Functions; }

  W_ALWAYS_INLINE WArrayPtr<const WPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

  /// Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(WDynamicArray<const WAbstractProperty*>& out_properties) const; // [tested]

  /// Returns the size (in bytes) of an instance of this type.
  W_ALWAYS_INLINE WUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// Returns the version number of this type.
  W_ALWAYS_INLINE WUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// Returns the type flags.
  W_ALWAYS_INLINE const WBitflags<WTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// Searches all WRTTI instances for the one with the given name, or nullptr if no such type exists.
  static const WRTTI* FindTypeByName(WStringView sName); // [tested]

  /// Searches all WRTTI instances for the one with the given hashed name, or nullptr if no such type exists.
  static const WRTTI* FindTypeByNameHash(WUInt64 uiNameHash); // [tested]
  static const WRTTI* FindTypeByNameHash32(WUInt32 uiNameHash);

  using PredicateFunc = WDelegate<bool(const WRTTI*), 48>;
  /// Searches all WRTTI instances for one where the given predicate function returns true
  static const WRTTI* FindTypeIf(PredicateFunc func);

  /// Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  const WAbstractProperty* FindPropertyByName(WStringView sName, bool bSearchBaseTypes = true) const; // [tested]

  /// Returns the name of the plugin which this type is declared in.
  W_ALWAYS_INLINE WStringView GetPluginName() const { return m_sPluginName; } // [tested]

  /// Returns the array of message handlers that this type has.
  W_ALWAYS_INLINE const WArrayPtr<WAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// Dispatches a message to the appropriate handler for this type.
  ///
  /// Uses optimized message dispatch tables for fast O(1) message routing. Returns true if a handler
  /// was found and the message was processed, false if no handler exists for this message type.
  /// The message system enables decoupled communication between components.
  bool DispatchMessage(void* pInstance, WMessage& ref_msg) const;

  /// Dispatches a message to the appropriate handler (const version).
  ///
  /// Same as the non-const version but for read-only message handlers. Some messages may only
  /// be handled by const handlers for safety reasons.
  bool DispatchMessage(const void* pInstance, WMessage& ref_msg) const;

  /// Returns whether this type can handle the given message type.
  template <typename MessageType>
  W_ALWAYS_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetTypeMsgId());
  }

  /// Returns whether this type can handle the message type with the given id.
  inline bool CanHandleMessage(WMessageId id) const
  {
    W_ASSERT_DEBUG(m_uiMsgIdOffset != WSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                            "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                            "you may have forgotten to instantiate an WPlugin object inside your plugin DLL.");

    const WUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers.GetData()[uiIndex] != nullptr;
  }

  W_ALWAYS_INLINE const WArrayPtr<WMessageSenderInfo>& GetMessageSender() const { return m_MessageSenders; }

  struct ForEachOptions
  {
    using StorageType = WUInt8;

    enum Enum
    {
      None = 0,
      ExcludeNonAllocatable = W_BIT(0), ///< Excludes all types that cannot be allocated through WRTTI. They may still be creatable through regular C++, though.
      ExcludeAbstract = W_BIT(1),       ///< Excludes all types that are marked as 'abstract'. They may not be abstract in the C++ sense, though.
      ExcludeNotConcrete = ExcludeNonAllocatable | ExcludeAbstract,

      Default = None
    };

    struct Bits
    {
      WUInt8 ExcludeNonAllocatable : 1;
      WUInt8 ExcludeAbstract : 1;
    };
  };

  using VisitorFunc = WDelegate<void(const WRTTI*), 48>;
  static void ForEachType(VisitorFunc func, WBitflags<ForEachOptions> options = ForEachOptions::Default); // [tested]

  static void ForEachDerivedType(const WRTTI* pBaseType, VisitorFunc func, WBitflags<ForEachOptions> options = ForEachOptions::Default);

  template <typename T>
  static W_ALWAYS_INLINE void ForEachDerivedType(VisitorFunc func, WBitflags<ForEachOptions> options = ForEachOptions::Default)
  {
    ForEachDerivedType(WGetStaticRTTI<T>(), func, options);
  }

protected:
  WStringView m_sPluginName;
  WStringView m_sTypeName;
  WArrayPtr<const WAbstractProperty* const> m_Properties;
  WArrayPtr<const WAbstractFunctionProperty* const> m_Functions;
  WArrayPtr<const WPropertyAttribute* const> m_Attributes;
  void UpdateType(const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType, WBitflags<WTypeFlags> flags);
  void RegisterType();
  void UnregisterType();

  void GatherDynamicMessageHandlers();
  void SetupParentHierarchy();

  const WRTTI* m_pParentType = nullptr;
  WRTTIAllocator* m_pAllocator = nullptr;

  WUInt32 m_uiTypeSize = 0;
  WUInt32 m_uiTypeVersion = 0;
  WUInt64 m_uiTypeNameHash = 0;
  WUInt32 m_uiTypeIndex = 0;
  WBitflags<WTypeFlags> m_TypeFlags;
  WUInt8 m_uiVariantType = 0;
  WUInt16 m_uiMsgIdOffset = WSmallInvalidIndex;

  const WRTTI* (*m_VerifyParent)();

  WArrayPtr<WAbstractMessageHandler*> m_MessageHandlers;
  WSmallArray<WAbstractMessageHandler*, 1, WStaticsAllocatorWrapper> m_DynamicMessageHandlers; // do not track this data, it won't be deallocated before shutdown

  WArrayPtr<WMessageSenderInfo> m_MessageSenders;
  WSmallArray<const WRTTI*, 7, WStaticsAllocatorWrapper> m_ParentHierarchy;

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// Assigns the given plugin name to every WRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(WStringView sPluginName);

  static void SanityCheckType(WRTTI* pType);

  /// Handles events by WPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const WPluginEvent& EventData);
};

W_DECLARE_FLAGS_OPERATORS(WRTTI::ForEachOptions);


// ***********************************
// ***** Object Allocator Struct *****


/// Interface for allocators that create instances of reflected types.
///
/// The RTTI allocator system provides controlled object creation for reflected types,
/// enabling features like custom memory management, object pooling, and creation tracking.
/// Different allocator implementations can optimize for specific use cases:
///
/// - WRTTIDefaultAllocator: Standard heap allocation
/// - WRTTINoAllocator: Prevents dynamic allocation (compile-time types only)
/// - Custom allocators: Object pools, stack allocation, etc.
struct W_FOUNDATION_DLL WRTTIAllocator
{
  virtual ~WRTTIAllocator();

  /// Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; } // [tested]

  /// Allocates one instance.
  template <typename T>
  WInternal::NewInstance<T> Allocate(WAllocator* pAllocator = nullptr)
  {
    return AllocateInternal(pAllocator).Cast<T>();
  }

  /// Clones the given instance.
  template <typename T>
  WInternal::NewInstance<T> Clone(const void* pObject, WAllocator* pAllocator = nullptr)
  {
    return CloneInternal(pObject, pAllocator).Cast<T>();
  }

  /// Deallocates the given instance.
  virtual void Deallocate(void* pObject, WAllocator* pAllocator = nullptr) = 0; // [tested]

private:
  virtual WInternal::NewInstance<void> AllocateInternal(WAllocator* pAllocator) = 0;
  virtual WInternal::NewInstance<void> CloneInternal(const void* pObject, WAllocator* pAllocator)
  {
    W_IGNORE_UNUSED(pObject);
    W_REPORT_FAILURE("Cloning is not supported by this allocator.");
    return WInternal::NewInstance<void>(nullptr, pAllocator);
  }
};

/// Allocator for types that cannot be dynamically allocated through reflection.
///
/// Used for abstract base classes, static utility classes, or types that should only be
/// created through specific factory functions. Attempting to allocate objects through
/// this allocator will trigger assertions in debug builds.
struct W_FOUNDATION_DLL WRTTINoAllocator : public WRTTIAllocator
{
  /// Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const override { return false; } // [tested]

  /// Will trigger an assert.
  virtual WInternal::NewInstance<void> AllocateInternal(WAllocator* pAllocator) override // [tested]
  {
    W_REPORT_FAILURE("This function should never be called.");
    return WInternal::NewInstance<void>(nullptr, pAllocator);
  }

  /// Will trigger an assert.
  virtual void Deallocate(void* pObject, WAllocator* pAllocator) override // [tested]
  {
    W_IGNORE_UNUSED(pObject);
    W_IGNORE_UNUSED(pAllocator);
    W_REPORT_FAILURE("This function should never be called.");
  }
};

/// Standard RTTI allocator that creates instances using WorldEngine's allocator system.
///
/// This is the default allocator used by most reflected types. It provides standard heap allocation
/// with proper integration into WorldEngine's memory management system. The allocator wrapper allows
/// customization of the underlying allocator (default, aligned, frame, etc.).
///
/// Template parameters:
/// - CLASS: The type to allocate (must be copy-constructible for cloning)
/// - AllocatorWrapper: Determines which allocator to use (default: WDefaultAllocatorWrapper)
template <typename CLASS, typename AllocatorWrapper = WDefaultAllocatorWrapper>
struct WRTTIDefaultAllocator : public WRTTIAllocator
{
  /// Returns a new instance that was allocated with the given allocator.
  virtual WInternal::NewInstance<void> AllocateInternal(WAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return W_NEW(pAllocator, CLASS);
  }

  /// Clones the given instance with the given allocator.
  virtual WInternal::NewInstance<void> CloneInternal(const void* pObject, WAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    if constexpr (std::is_copy_constructible_v<CLASS>)
    {
      return W_NEW(pAllocator, CLASS, *static_cast<const CLASS*>(pObject));
    }
    else
    {
      W_REPORT_FAILURE("Clone failed since the type is not copy constructible");
      return WInternal::NewInstance<void>(nullptr, pAllocator);
    }
  }

  /// Deletes the given instance with the given allocator.
  virtual void Deallocate(void* pObject, WAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    CLASS* pPointer = static_cast<CLASS*>(pObject);
    W_DELETE(pAllocator, pPointer);
  }
};
