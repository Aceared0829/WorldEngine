#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

#ifndef W_WORLD_INDEX_BITS
#  define W_WORLD_INDEX_BITS 8
#endif

#define W_MAX_WORLDS (1 << W_WORLD_INDEX_BITS)

class WWorld;
class WSpatialSystem;
class WCoordinateSystemProvider;

namespace WInternal
{
  class WorldData;

  enum
  {
    DEFAULT_BLOCK_SIZE = 1024 * 16
  };

  using WorldLargeBlockAllocator = WLargeBlockAllocator<DEFAULT_BLOCK_SIZE>;
} // namespace WInternal

class WGameObject;
struct WGameObjectDesc;

class WComponentManagerBase;
class WComponent;

struct WMsgDeleteGameObject;

/// Internal world id used by WWorldHandle.
using WWorldId = WGenericId<8, 8>;

/// A handle to a world.
struct WWorldHandle
{
  W_DECLARE_HANDLE_TYPE(WWorldHandle, WWorldId);

  friend class WWorld;
};

/// Internal game object id used by WGameObjectHandle.
struct WGameObjectId
{
  using StorageType = WUInt64;

  W_DECLARE_ID_TYPE(WGameObjectId, 32, 8);

  static_assert(W_WORLD_INDEX_BITS > 0 && W_WORLD_INDEX_BITS <= 24);

  W_FORCE_INLINE WGameObjectId(StorageType instanceIndex, WUInt8 uiGeneration, WUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<WUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : W_WORLD_INDEX_BITS;
    };
  };
};

/// A handle to a game object.
///
/// Never store a direct pointer to a game object. Always store a handle instead. A pointer to a game object can
/// be received by calling WWorld::TryGetObject with the handle.
/// Note that the object might have been deleted so always check the return value of TryGetObject.
struct WGameObjectHandle
{
  W_DECLARE_HANDLE_TYPE(WGameObjectHandle, WGameObjectId);

  friend class WWorld;
  friend class WGameObject;
};

/// HashHelper implementation so game object handles can be used as key in a hash table.
template <>
struct WHashHelper<WGameObjectHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WGameObjectHandle value)
  {
    const WUInt64 data = value.GetInternalID().m_Data;
    return WHashingUtils::xxHash32(&data, sizeof(data));
  }

  W_ALWAYS_INLINE static bool Equal(WGameObjectHandle a, WGameObjectHandle b) { return a == b; }
};

/// Currently not implemented as it is not needed for game object handles.
W_CORE_DLL void operator<<(WStreamWriter& inout_stream, const WGameObjectHandle& hValue);
W_CORE_DLL void operator>>(WStreamReader& inout_stream, WGameObjectHandle& ref_hValue);

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WGameObjectHandle);
W_DECLARE_CUSTOM_VARIANT_TYPE(WGameObjectHandle);
#define W_COMPONENT_TYPE_INDEX_BITS (24 - W_WORLD_INDEX_BITS)
#define W_MAX_COMPONENT_TYPES (1 << W_COMPONENT_TYPE_INDEX_BITS)

/// Internal component id used by WComponentHandle.
struct WComponentId
{
  using StorageType = WUInt64;

  W_DECLARE_ID_TYPE(WComponentId, 32, 8);

  static_assert(W_COMPONENT_TYPE_INDEX_BITS > 0 && W_COMPONENT_TYPE_INDEX_BITS <= 16);

  W_ALWAYS_INLINE WComponentId(StorageType instanceIndex, WUInt8 uiGeneration, WUInt16 uiTypeId = 0, WUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<WUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_TypeId = uiTypeId;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : W_WORLD_INDEX_BITS;
      StorageType m_TypeId : W_COMPONENT_TYPE_INDEX_BITS;
    };
  };
};

/// A handle to a component.
///
/// Never store a direct pointer to a component. Always store a handle instead. A pointer to a component can
/// be received by calling WWorld::TryGetComponent or TryGetComponent on the corresponding component manager.
/// Note that the component might have been deleted so always check the return value of TryGetComponent.
struct WComponentHandle
{
  W_DECLARE_HANDLE_TYPE(WComponentHandle, WComponentId);

  friend class WWorld;
  friend class WComponentManagerBase;
  friend class WComponent;
};

/// A typed handle to a component.
///
/// This should be preferred if the component type to be stored inside the handle is known, as it provides
/// compile time checks against wrong usages (e.g. assigning unrelated types) and more clearly conveys intent.
///
/// See struct \see WComponentHandle for more information about general component handle usage.
template <typename TYPE>
struct WTypedComponentHandle : public WComponentHandle
{
  WTypedComponentHandle() = default;
  explicit WTypedComponentHandle(const WComponentHandle& hUntyped)
  {
    m_InternalId = hUntyped.GetInternalID();
  }

  template <typename T, std::enable_if_t<std::is_convertible_v<T*, TYPE*>, bool> = true>
  explicit WTypedComponentHandle(const WTypedComponentHandle<T>& other)
    : WTypedComponentHandle(static_cast<const WComponentHandle&>(other))
  {
  }

  template <typename T, std::enable_if_t<std::is_convertible_v<T*, TYPE*>, bool> = true>
  W_ALWAYS_INLINE void operator=(const WTypedComponentHandle<T>& other)
  {
    WComponentHandle::operator=(other);
  }
};

/// HashHelper implementation so component handles can be used as key in a hashtable.
template <>
struct WHashHelper<WComponentHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WComponentHandle value)
  {
    const WUInt64 data = value.GetInternalID().m_Data;
    return WHashingUtils::xxHash32(&data, sizeof(data));
  }

  W_ALWAYS_INLINE static bool Equal(WComponentHandle a, WComponentHandle b) { return a == b; }
};

/// Currently not implemented as it is not needed for component handles.
W_CORE_DLL void operator<<(WStreamWriter& inout_stream, const WComponentHandle& hValue);
W_CORE_DLL void operator>>(WStreamReader& inout_stream, WComponentHandle& ref_hValue);

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WComponentHandle);
W_DECLARE_CUSTOM_VARIANT_TYPE(WComponentHandle);

/// Internal flags of game objects or components.
struct WObjectFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,
    Dynamic = W_BIT(0),                              ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = W_BIT(1),                         ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                                      ///< objects, and the automatic detection cannot know that.
    ActiveFlag = W_BIT(2),                           ///< The object/component has the 'active flag' set
    ActiveState = W_BIT(3),                          ///< The object/component and all its parents have the active flag
    Initialized = W_BIT(4),                          ///< The object/component has been initialized
    Initializing = W_BIT(5),                         ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted = W_BIT(6),                    ///< OnSimulationStarted() has been called on the component
    SimulationStarting = W_BIT(7),                   ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = W_BIT(8),              ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications = W_BIT(9),            ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications = W_BIT(10),       ///< The object should send a notification message when components are added or removed.
    StaticTransformChangesNotifications = W_BIT(11), ///< The object should send a notification message if it is static and its transform changes.
    ParentChangesNotifications = W_BIT(12),          ///< The object should send a notification message when the parent is changes.

    CreatedByPrefab = W_BIT(13),                     ///< Such flagged objects and components are ignored during scene export (see WWorldWriter) and will be removed when a prefab needs to be re-instantiated.
    HideShapeIcon = W_BIT(14),                       ///< Hide the shape icon of the object in the editor.

    UserFlag0 = W_BIT(24),
    UserFlag1 = W_BIT(25),
    UserFlag2 = W_BIT(26),
    UserFlag3 = W_BIT(27),
    UserFlag4 = W_BIT(28),
    UserFlag5 = W_BIT(29),
    UserFlag6 = W_BIT(30),
    UserFlag7 = W_BIT(31),

    Default = None
  };

  struct Bits
  {
    StorageType Dynamic : 1;                             //< 0
    StorageType ForceDynamic : 1;                        //< 1
    StorageType ActiveFlag : 1;                          //< 2
    StorageType ActiveState : 1;                         //< 3
    StorageType Initialized : 1;                         //< 4
    StorageType Initializing : 1;                        //< 5
    StorageType SimulationStarted : 1;                   //< 6
    StorageType SimulationStarting : 1;                  //< 7
    StorageType UnhandledMessageHandler : 1;             //< 8
    StorageType ChildChangesNotifications : 1;           //< 9
    StorageType ComponentChangesNotifications : 1;       //< 10
    StorageType StaticTransformChangesNotifications : 1; //< 11
    StorageType ParentChangesNotifications : 1;          //< 12

    StorageType CreatedByPrefab : 1;                     //< 13
    StorageType HideShapeIcon : 1;                       //< 14

    StorageType Padding : 9;                             // 15 - 23

    StorageType UserFlag0 : 1;                           //< 24
    StorageType UserFlag1 : 1;                           //< 25
    StorageType UserFlag2 : 1;                           //< 26
    StorageType UserFlag3 : 1;                           //< 27
    StorageType UserFlag4 : 1;                           //< 28
    StorageType UserFlag5 : 1;                           //< 29
    StorageType UserFlag6 : 1;                           //< 30
    StorageType UserFlag7 : 1;                           //< 31
  };
};

W_DECLARE_FLAGS_OPERATORS(WObjectFlags);

/// Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa WObjectFlags
struct WObjectMode
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Automatic,
    ForceDynamic,

    Default = Automatic
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WObjectMode);

/// Specifies the mode of a component. Dynamic components may change an object's transform, static components must not.
///
/// \sa WObjectFlags
struct WComponentMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WComponentMode);

/// Specifies at which phase the queued message should be processed.
struct WObjectMsgQueueType
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    PostAsync,        ///< Process the message in the PostAsync phase.
    PostTransform,    ///< Process the message in the PostTransform phase.
    NextFrame,        ///< Process the message in the PreAsync phase of the next frame.
    AfterInitialized, ///< Process the message after new components have been initialized.
    COUNT,

    Default = NextFrame
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WObjectMsgQueueType);

/// Certain components may delete themselves or their owner when they are finished with their main purpose
struct W_CORE_DLL WOnComponentFinishedAction
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.

    Default = None
  };

  /// Call this when a component is 'finished' with its work.
  ///
  /// Pass in the desired action (usually configured by the user) and the 'this' pointer of the component.
  /// The helper function will delete this component and maybe also attempt to delete the entire object.
  /// For that it will coordinate with other components, and delay the object deletion, if necessary,
  /// until the last component has finished it's work.
  static void HandleFinishedAction(WComponent* pComponent, WOnComponentFinishedAction::Enum action);

  /// Call this function in a message handler for WMsgDeleteGameObject messages.
  ///
  /// This is needed to coordinate object deletion across multiple components that use the
  /// WOnComponentFinishedAction mechanism.
  /// Depending on the state of this component, the function will either execute the object deletion,
  /// or delay it, until its own work is done.
  static void HandleDeleteObjectMsg(WMsgDeleteGameObject& ref_msg, WEnum<WOnComponentFinishedAction>& ref_action);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WOnComponentFinishedAction);

/// Same as WOnComponentFinishedAction, but additionally includes 'Restart'
struct W_CORE_DLL WOnComponentFinishedAction2
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.
    Restart,          ///< When finished, restart from the beginning.

    Default = None
  };

  /// See WOnComponentFinishedAction::HandleFinishedAction()
  static void HandleFinishedAction(WComponent* pComponent, WOnComponentFinishedAction2::Enum action);

  /// See WOnComponentFinishedAction::HandleDeleteObjectMsg()
  static void HandleDeleteObjectMsg(WMsgDeleteGameObject& ref_msg, WEnum<WOnComponentFinishedAction2>& ref_action);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WOnComponentFinishedAction2);

/// Used as return value of visitor functions to define whether calling function should stop or continue visiting.
struct WVisitorExecution
{
  enum Enum
  {
    Continue, ///< Continue regular iteration
    Skip,     ///< In a depth-first iteration mode this will skip the entire sub-tree below the current object
    Stop      ///< Stop the entire iteration
  };
};

using WSpatialDataId = WGenericId<24, 8>;
class WSpatialDataHandle
{
  W_DECLARE_HANDLE_TYPE(WSpatialDataHandle, WSpatialDataId);
};

#define W_MAX_WORLD_MODULE_TYPES W_MAX_COMPONENT_TYPES
using WWorldModuleTypeId = WUInt16;
static_assert(WMath::MaxValue<WWorldModuleTypeId>() >= W_MAX_WORLD_MODULE_TYPES - 1);

using WComponentInitBatchId = WGenericId<24, 8>;
class WComponentInitBatchHandle
{
  W_DECLARE_HANDLE_TYPE(WComponentInitBatchHandle, WComponentInitBatchId);
};

#include <Core/World/WorldLogLink.h>
