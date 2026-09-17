#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class WWorld;

/// Defines the different phases during world updates for module execution ordering.
struct WWorldUpdatePhase
{
  using StorageType = WUInt8;

  enum Enum
  {
    PreAsync,      ///< Synchronous phase before parallel processing
    Async,         ///< Parallel processing phase (thread-safe operations only)
    PostAsync,     ///< Synchronous phase after parallel processing
    PostTransform, ///< Synchronous phase after transform updates
    COUNT,

    Default = PreAsync
  };
};

/// Base class for world modules that extend world functionality.
///
/// World modules provide additional functionality to worlds such as component management,
/// physics simulation, or rendering. They can register update functions that are called
/// during different phases of the world update cycle and manage resources and state.
class W_CORE_DLL WWorldModule : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WWorldModule, WReflectedClass);

protected:
  WWorldModule(WWorld* pWorld);
  virtual ~WWorldModule();

public:
  /// Returns the corresponding world to this module.
  WWorld* GetWorld();

  /// Returns the corresponding world to this module.
  const WWorld* GetWorld() const;

  /// Same as GetWorld()->GetIndex(). Needed to break circular include dependencies.
  WUInt32 GetWorldIndex() const;

protected:
  friend class WWorld;
  friend class WInternal::WorldData;
  friend class WMemoryUtils;

  /// Context passed to update functions containing information about component range to process.
  struct UpdateContext
  {
    WUInt32 m_uiFirstComponentIndex = 0; ///< Index of the first component to process in this batch
    WUInt32 m_uiComponentCount = 0;      ///< Number of components to process in this batch
  };

  /// Update function delegate.
  using UpdateFunction = WDelegate<void(const UpdateContext&)>;

  /// Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    UpdateFunctionDesc(const UpdateFunction& function, WStringView sFunctionName)
      : m_Function(function)
    {
      m_sFunctionName.Assign(sFunctionName);
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    WHashedString m_sFunctionName;               ///< Name of the function. Use the W_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                                  ///< with the correct name.
    WHybridArray<WHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    WEnum<WWorldUpdatePhase> m_Phase;           ///< The update phase in which this update function should be called. See WWorld for a description on the different phases.
    bool m_bOnlyUpdateWhenSimulating = false;     ///< The update function is only called when the world simulation is enabled.
    WUInt16 m_uiAsyncPhaseBatchSize = 0;         ///< 0 means m_Function is called once per frame, to update all components, but still in parallel with other world modules.
                                                  ///< >0 means m_Function is called multiple times (in parallel) with batches of roughly this size.
    float m_fPriority = 0.0f;                     ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// Returns the allocator used by the world.
  WAllocator* GetAllocator();

  /// Returns the block allocator used by the world.
  WInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
  /// This method is called after the constructor. A derived type can override this method to do initialization work. Typically this
  /// is the method where updates function are registered.
  virtual void Initialize() {}

  /// This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual void Deinitialize() {}

  /// This method is called at the start of the next world update when the world is simulated. This method will be called after the
  /// initialization method.
  virtual void OnSimulationStarted() {}

  /// Called by WWorld::Clear(). Can be used to clear cached data when a world is completely cleared of objects (but not deleted).
  virtual void WorldClear() {}

  WWorld* m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// Helper class to get component type ids and create new instances of world modules from rtti.
class W_CORE_DLL WWorldModuleFactory
{
public:
  static WWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  WWorldModuleTypeId RegisterWorldModule();

  /// Returns the module type id to the given rtti module/component type.
  WWorldModuleTypeId GetTypeId(const WRTTI* pRtti);

  /// Creates a new instance of the world module with the given type id and world.
  WWorldModule* CreateWorldModule(WUInt16 uiTypeId, WWorld* pWorld);

  /// Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(WStringView sInterfaceName, WStringView sImplementationName);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  using CreatorFunc = WWorldModule* (*)(WAllocator*, WWorld*);

  WWorldModuleFactory();
  WWorldModuleTypeId RegisterWorldModule(const WRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const WPluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();
  void AdjustBaseTypeId(const WRTTI* pParentRtti, const WRTTI* pRtti, WUInt16 uiParentTypeId);

  WHashTable<const WRTTI*, WWorldModuleTypeId> m_TypeToId;

  struct CreatorFuncContext
  {
    W_DECLARE_POD_TYPE();

    CreatorFunc m_Func;
    const WRTTI* m_pRtti;
  };

  WDynamicArray<CreatorFuncContext> m_CreatorFuncs;

  WHashTable<WString, WString> m_InterfaceImplementations;
};

/// Add this macro to the declaration of your module type.
#define W_DECLARE_WORLD_MODULE()                      \
public:                                                \
  static W_ALWAYS_INLINE WWorldModuleTypeId TypeId() \
  {                                                    \
    return s_TypeId;                                   \
  }                                                    \
                                                       \
private:                                               \
  static WWorldModuleTypeId s_TypeId;

/// Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define W_IMPLEMENT_WORLD_MODULE(moduleType) \
  WWorldModuleTypeId moduleType::s_TypeId = WWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// Helper macro to create an update function description with proper name
#define W_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) WWorldModule::UpdateFunctionDesc(WWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
