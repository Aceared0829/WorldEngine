#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>

/// Base class for all component managers. Do not derive directly from this class, but derive from WComponentManager instead.
///
/// Every component type has its corresponding manager type. The manager stores the components in memory blocks to minimize overhead
/// on creation and deletion of components. Each manager can also register update functions to update its components during
/// the different update phases of WWorld.
/// Use WWorld::CreateComponentManager to create an instance of a component manager within a specific world.
class W_CORE_DLL WComponentManagerBase : public WWorldModule
{
  W_ADD_DYNAMIC_REFLECTION(WComponentManagerBase, WWorldModule);

protected:
  WComponentManagerBase(WWorld* pWorld);
  virtual ~WComponentManagerBase();

public:
  /// Checks whether the given handle references a valid component.
  bool IsValidComponent(const WComponentHandle& hComponent) const;

  /// Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const WComponentHandle& hComponent, WComponent*& out_pComponent);

  /// Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const WComponentHandle& hComponent, const WComponent*& out_pComponent) const;

  /// Returns the number of components managed by this manager.
  WUInt32 GetComponentCount() const;

  /// Create a new component instance and returns a handle to it.
  WComponentHandle CreateComponent(WGameObject* pOwnerObject);

  /// Create a new component instance and returns a handle to it.
  template <typename ComponentType>
  WTypedComponentHandle<ComponentType> CreateComponent(WGameObject* pOwnerObject, ComponentType*& out_pComponent);

  /// Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const WComponentHandle& hComponent);

  /// Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(WComponent* pComponent);

  /// Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a WComponentManagerBase pointer.
  virtual void CollectAllComponents(WDynamicArray<WComponentHandle>& out_allComponents, bool bOnlyActive) = 0;

  /// Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a WComponentManagerBase pointer.
  virtual void CollectAllComponents(WDynamicArray<WComponent*>& out_allComponents, bool bOnlyActive) = 0;

protected:
  /// \cond
  // internal methods
  friend class WWorld;
  friend class WInternal::WorldData;

  virtual void Deinitialize() override;

protected:
  friend class WWorldReader;

  WComponentHandle CreateComponentNoInit(WGameObject* pOwnerObject, WComponent*& out_pComponent);
  void InitializeComponent(WComponent* pComponent);
  void DeinitializeComponent(WComponent* pComponent);
  void PatchIdTable(WComponent* pComponent);

  virtual WComponent* CreateComponentStorage() = 0;
  virtual void DeleteComponentStorage(WComponent* pComponent, WComponent*& out_pMovedComponent) = 0;

  /// \endcond

  WIdTable<WComponentId, WComponent*> m_Components;
};

template <typename T, WBlockStorageType::Enum StorageType>
class WComponentManager : public WComponentManagerBase
{
public:
  using ComponentType = T;
  using SUPER = WComponentManagerBase;

  /// Although the constructor is public always use WWorld::CreateComponentManager to create an instance.
  WComponentManager(WWorld* pWorld);
  virtual ~WComponentManager();

  /// Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const WComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const WComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// Returns an iterator over all components.
  typename WBlockStorage<ComponentType, WInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator GetComponents(WUInt32 uiStartIndex = 0);

  /// Returns an iterator over all components.
  typename WBlockStorage<ComponentType, WInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator GetComponents(WUInt32 uiStartIndex = 0) const;

  /// Returns the type id corresponding to the component type managed by this manager.
  static WWorldModuleTypeId TypeId();

  virtual void CollectAllComponents(WDynamicArray<WComponentHandle>& out_allComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(WDynamicArray<WComponent*>& out_allComponents, bool bOnlyActive) override;

protected:
  friend ComponentType;
  friend class WComponentManagerFactory;

  virtual WComponent* CreateComponentStorage() override;
  virtual void DeleteComponentStorage(WComponent* pComponent, WComponent*& out_pMovedComponent) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  WBlockStorage<ComponentType, WInternal::DEFAULT_BLOCK_SIZE, StorageType> m_ComponentStorage;
};


//////////////////////////////////////////////////////////////////////////

struct WComponentUpdateType
{
  enum Enum
  {
    Always,
    WhenSimulating
  };
};

/// Simple component manager implementation that calls an update method on all components every frame.
template <typename ComponentType, WComponentUpdateType::Enum UpdateType, WBlockStorageType::Enum StorageType = WBlockStorageType::FreeList, WWorldUpdatePhase::Enum UpdatePhase = WWorldUpdatePhase::PreAsync>
class WComponentManagerSimple final : public WComponentManager<ComponentType, StorageType>
{
public:
  WComponentManagerSimple(WWorld* pWorld);

  virtual void Initialize() override;

  /// A simple update function that iterates over all components and calls Update() on every component
  void SimpleUpdate(const WWorldModule::UpdateContext& context);

private:
  static void SimpleUpdateName(WStringBuilder& out_sName);
};

//////////////////////////////////////////////////////////////////////////

#define W_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType)                                            \
public:                                                                                                                 \
  using ComponentManagerType = managerType;                                                                             \
  virtual WWorldModuleTypeId GetTypeId() const override                                                                \
  {                                                                                                                     \
    return s_TypeId;                                                                                                    \
  }                                                                                                                     \
  static W_ALWAYS_INLINE WWorldModuleTypeId TypeId()                                                                  \
  {                                                                                                                     \
    return s_TypeId;                                                                                                    \
  }                                                                                                                     \
  WTypedComponentHandle<componentType> GetHandle() const                                                               \
  {                                                                                                                     \
    return WTypedComponentHandle<componentType>(WComponent::GetHandle());                                             \
  }                                                                                                                     \
  virtual WComponentMode::Enum GetMode() const override;                                                               \
  static WTypedComponentHandle<componentType> CreateComponent(WGameObject* pOwnerObject, componentType*& pComponent); \
                                                                                                                        \
private:                                                                                                                \
  friend managerType;                                                                                                   \
  static WWorldModuleTypeId s_TypeId

#define W_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType) \
public:                                                                  \
  virtual WWorldModuleTypeId GetTypeId() const override                 \
  {                                                                      \
    return WWorldModuleTypeId(-1);                                      \
  }                                                                      \
  static W_ALWAYS_INLINE WWorldModuleTypeId TypeId()                   \
  {                                                                      \
    return WWorldModuleTypeId(-1);                                      \
  }

/// Add this macro to a custom component type inside the type declaration.
#define W_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  W_ADD_DYNAMIC_REFLECTION(componentType, baseType);                   \
  W_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType);

/// Add this macro to a custom abstract component type inside the type declaration.
#define W_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  W_ADD_DYNAMIC_REFLECTION(componentType, baseType);               \
  W_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType);


/// Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see W_BEGIN_DYNAMIC_REFLECTED_TYPE
#define W_BEGIN_COMPONENT_TYPE(componentType, version, mode)                                                                            \
  WWorldModuleTypeId componentType::s_TypeId =                                                                                          \
    WWorldModuleFactory::GetInstance()->RegisterWorldModule<typename componentType::ComponentManagerType, componentType>();             \
  WComponentMode::Enum componentType::GetMode() const                                                                                   \
  {                                                                                                                                      \
    return mode;                                                                                                                         \
  }                                                                                                                                      \
  WTypedComponentHandle<componentType> componentType::CreateComponent(WGameObject* pOwnerObject, componentType*& out_pComponent)       \
  {                                                                                                                                      \
    return pOwnerObject->GetWorld()->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pOwnerObject, out_pComponent); \
  }                                                                                                                                      \
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, WRTTINoAllocator)

/// Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see W_BEGIN_DYNAMIC_REFLECTED_TYPE
#define W_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version) W_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(componentType, version)

/// Ends the component implementation code block that was opened with W_BEGIN_COMPONENT_TYPE.
#define W_END_COMPONENT_TYPE W_END_DYNAMIC_REFLECTED_TYPE
#define W_END_ABSTRACT_COMPONENT_TYPE W_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE

#include <Core/World/Implementation/ComponentManager_inl.h>
