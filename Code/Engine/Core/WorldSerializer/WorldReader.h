#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

class WStringDeduplicationReadContext;
class WProgress;
class WProgressRange;

struct WPrefabInstantiationOptions
{
  WGameObjectHandle m_hParent;

  WDynamicArray<WGameObject*>* m_pCreatedRootObjectsOut = nullptr;
  WDynamicArray<WGameObject*>* m_pCreatedChildObjectsOut = nullptr;
  const WUInt16* m_pOverrideTeamID = nullptr;

  bool m_bForceDynamic = false;

  /// If the prefab has a single root node with this non-empty name, rather than creating a new object, instead the m_hParent object is used.
  WTempHashedString m_ReplaceNamedRootWithParent;

  enum class RandomSeedMode
  {
    DeterministicFromParent, ///< WWorld::CreateObject() will either derive a deterministic value from the parent object, or assign a random value, if no parent exists
    CompletelyRandom,        ///< WWorld::CreateObject() will assign a random value to this object
    FixedFromSerialization,  ///< Keep deserialized random seed value
    CustomRootValue,         ///< Use the given seed root value to assign a deterministic (but different) value to each game object.
  };

  RandomSeedMode m_RandomSeedMode = RandomSeedMode::DeterministicFromParent;
  WUInt32 m_uiCustomRandomSeedRootValue = 0;

  WTime m_MaxStepTime = WTime::MakeZero();

  WProgress* m_pProgress = nullptr;
};

/// Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different WWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class W_CORE_DLL WWorldReader
{
public:
  /// A context object is returned from InstantiateWorld or InstantiatePrefab if a maxStepTime greater than zero is specified.
  ///
  /// Call the Step() function periodically to complete the instantiation.
  /// Each step will try to spend not more than the given maxStepTime.
  /// E.g. this is useful if the instantiation cost of large prefabs needs to be distributed over multiple frames.
  class InstantiationContextBase
  {
  public:
    enum class StepResult
    {
      Continue,          ///< The available time slice is used up. Call Step() again to continue the process.
      ContinueNextFrame, ///< The process has reached a point where you need to call WWorld::Update(). Otherwise no further progress can be made.
      Finished,          ///< The instantiation is finished and you can delete the context. Don't call 'Step()' on it again.
    };

    virtual ~InstantiationContextBase() = default;

    /// Advance the instantiation by one step
    /// \return Whether the operation is finished or needs to be repeated.
    virtual StepResult Step() = 0;

    /// Cancel the instantiation. This might lead to inconsistent states and must be used with care.
    virtual void Cancel() = 0;
  };

  WWorldReader();
  ~WWorldReader();

  /// Reads all information about the world from the given stream.
  ///
  /// Call this once to populate WWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into an WWorld.
  /// By default, the method will warn if it skips bytes in the stream that are of unknown
  /// types. The warnings can be suppressed by setting warningOnUnkownSkip to false.
  WResult ReadWorldDescription(WStreamReader& inout_stream, bool bWarningOnUnkownSkip = true);

  /// Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// This is identical to calling InstantiatePrefab() with identity values, however, it is a bit
  /// more efficient, as unnecessary computations are skipped.
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The WProgress object
  /// has to be valid as long as the instantiation is in progress.
  WUniquePtr<InstantiationContextBase> InstantiateWorld(WWorld& ref_world, const WUInt16* pOverrideTeamID = nullptr, WTime maxStepTime = WTime::MakeZero(), WProgress* pProgress = nullptr);

  /// Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// \param rootTransform is an additional transform that is applied to all root objects.
  /// \param hParent allows to attach the newly created objects immediately to a parent
  /// \param out_CreatedRootObjects If this is valid, all pointers the to created root objects are stored in this array
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The WProgress object
  /// has to be valid as long as the instantiation is in progress.
  WUniquePtr<InstantiationContextBase> InstantiatePrefab(WWorld& ref_world, const WTransform& rootTransform, const WPrefabInstantiationOptions& options);

  /// Gives access to the stream of data. Use this inside component deserialization functions to read data.
  WStreamReader& GetStream() const;

  /// Used during component deserialization to read a handle to a game object.
  WGameObjectHandle ReadGameObjectHandle();

  /// Used during component deserialization to read a handle to a component.
  void ReadComponentHandle(WComponentHandle& out_hComponent);

  /// Used during component deserialization to query the actual version number with which the
  /// given component type was written. The version number is given through the W_BEGIN_COMPONENT_TYPE
  /// macro. Whenever the serialization of a component changes, that number should be increased.
  WUInt32 GetComponentTypeVersion(const WRTTI* pRtti) const;

  /// Returns whether world contains a component of given type.
  bool HasComponentOfType(const WRTTI* pRtti) const;

  /// Clears all data.
  void ClearAndCompact();

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const;

  using FindComponentTypeCallback = WDelegate<const WRTTI*(WStringView sTypeName)>;

  /// An optional callback to redirect the lookup of a component type name to an WRTTI type.
  ///
  /// If specified, this is used by ALL world readers. The intention is to use this either for logging purposes,
  /// or to implement a whitelist or blacklist for specific component types.
  /// E.g. if the callback returns nullptr, the component type is 'unknown' and skipped by the world reader.
  /// Thus one can remove unwanted component types.
  /// Theoretically one could also redirect an old (or renamed) component type to a new one,
  /// given that their deserialization code is compatible.
  static FindComponentTypeCallback s_FindComponentTypeCallback;

  WUInt32 GetRootObjectCount() const;
  WUInt32 GetChildObjectCount() const;

  static void SetMaxStepTime(InstantiationContextBase* pContext, WTime maxStepTime);
  static WTime GetMaxStepTime(InstantiationContextBase* pContext);

private:
  struct GameObjectToCreate
  {
    WGameObjectDesc m_Desc;
    WString m_sGlobalKey;
    WUInt32 m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentTypeInfo(WUInt32 uiComponentTypeIdx);
  void ReadComponentDataToMemStream(bool warningOnUnknownSkip = true);

  WUniquePtr<InstantiationContextBase> Instantiate(WWorld& world, bool bUseTransform, const WTransform& rootTransform, const WPrefabInstantiationOptions& options);

  WStreamReader* m_pReadStream = nullptr;
  WUInt8 m_uiVersion = 0;

  WDynamicArray<GameObjectToCreate> m_RootObjectsToCreate;
  WDynamicArray<GameObjectToCreate> m_ChildObjectsToCreate;

  struct ComponentTypeInfo
  {
    const WRTTI* m_pRtti = nullptr;
    WUInt32 m_uiNumComponents = 0;
    WUInt32 m_uiComponentDataSize = 0;
  };

  WDynamicArray<ComponentTypeInfo> m_ComponentTypes;
  WHashTable<const WRTTI*, WUInt32> m_ComponentTypeVersions;
  WDefaultMemoryStreamStorage m_ComponentCreationStream;
  WDefaultMemoryStreamStorage m_ComponentDataStream;
  WUInt64 m_uiTotalNumComponents = 0;

  WUniquePtr<WStringDeduplicationReadContext> m_pStringDedupReadContext;

  class InstantiationContext : public InstantiationContextBase
  {
  public:
    InstantiationContext(WWorldReader& ref_worldReader, WWorld* pWorld, bool bUseTransform, const WTransform& rootTransform, const WPrefabInstantiationOptions& options, WAllocator* pAllocator);
    ~InstantiationContext();

    virtual StepResult Step() override;
    virtual void Cancel() override;

    template <bool UseTransform>
    bool CreateGameObjects(const WDynamicArray<GameObjectToCreate>& objects, WGameObjectHandle hParent, WDynamicArray<WGameObject*>* out_pCreatedObjects, WTime endTime);

    bool CreateComponents(WTime endTime);
    bool DeserializeComponents(WTime endTime);
    bool AddComponentsToBatch(WTime endTime);

    void SetMaxStepTime(WTime stepTime);
    WTime GetMaxStepTime() const;

  private:
    void BeginNextProgressStep(WStringView sName);
    void SetSubProgressCompletion(double fCompletion);

    friend class WWorldReader;
    WWorldReader& m_WorldReader;

    WWorld* m_pWorld = nullptr;

    bool m_bUseTransform = false;
    WTransform m_RootTransform;

    WPrefabInstantiationOptions m_Options;

    struct ComponentTypeState
    {
      ComponentTypeState(WAllocator* pAllocator)
        : m_ComponentIndexToHandle(pAllocator)
      {
      }

      WUInt64 m_uiDataReadOffset = 0;
      WDynamicArray<WComponentHandle> m_ComponentIndexToHandle;
    };

    WDynamicArray<WGameObjectHandle> m_IndexToGameObjectHandle;
    WDynamicArray<ComponentTypeState> m_ComponentTypeStates;

    WComponentInitBatchHandle m_hComponentInitBatch;

    // Current state
    struct Phase
    {
      enum Enum
      {
        Invalid = -1,
        CreateRootObjects,
        CreateChildObjects,
        CreateComponents,
        DeserializeComponents,
        AddComponentsToBatch,
        InitComponents,

        Count
      };
    };

    Phase::Enum m_Phase = Phase::Invalid;
    WUInt32 m_uiCurrentIndex = 0; // object or component
    WUInt32 m_uiCurrentComponentTypeIndex = 0;
    WUInt64 m_uiCurrentNumComponentsProcessed = 0;
    WMemoryStreamReader m_CurrentReader;

    WUniquePtr<WProgressRange> m_pOverallProgressRange;
    WUniquePtr<WProgressRange> m_pSubProgressRange;
  };
};
