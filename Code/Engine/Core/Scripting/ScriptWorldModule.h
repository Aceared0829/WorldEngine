#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>

using WScriptClassResourceHandle = WTypedResourceHandle<class WScriptClassResource>;
class WScriptInstance;

/// World module responsible for script execution and coroutine management.
///
/// Handles the execution of script functions, manages script coroutines,
/// and provides scheduling for script update functions. This module ensures
/// scripts are properly integrated with the world update cycle.
class W_CORE_DLL WScriptWorldModule : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WScriptWorldModule, WWorldModule);
  W_DISALLOW_COPY_AND_ASSIGN(WScriptWorldModule);

public:
  WScriptWorldModule(WWorld* pWorld);
  ~WScriptWorldModule();

  virtual void Initialize() override;
  virtual void WorldClear() override;

  /// Schedules a script function to be called at regular intervals.
  void AddUpdateFunctionToSchedule(const WAbstractFunctionProperty* pFunction, void* pInstance, WTime updateInterval, bool bOnlyWhenSimulating);

  /// Removes a previously scheduled script function from the scheduler.
  void RemoveUpdateFunctionToSchedule(const WAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// Creates a new coroutine of the specified type with the given name.
  ///
  /// Returns an invalid handle if the creationMode prevents creating a new coroutine
  /// and there is already a coroutine running with the same name on the given instance.
  WScriptCoroutineHandle CreateCoroutine(const WRTTI* pCoroutineType, WStringView sName, WScriptInstance& inout_instance, WScriptCoroutineCreationMode::Enum creationMode, WScriptCoroutine*& out_pCoroutine);

  /// Starts the coroutine with the given arguments.
  ///
  /// Calls the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(WScriptCoroutineHandle hCoroutine, WArrayPtr<WVariant> arguments);

  /// Stops and deletes the coroutine.
  ///
  /// Calls the Stop() function and deletes the coroutine on the next update cycle.
  void StopAndDeleteCoroutine(WScriptCoroutineHandle hCoroutine);

  /// Stops and deletes all coroutines with the given name on the specified instance.
  void StopAndDeleteCoroutine(WStringView sName, WScriptInstance* pInstance);

  /// Stops and deletes all coroutines on the specified instance.
  void StopAndDeleteAllCoroutines(WScriptInstance* pInstance);

  /// Returns whether the coroutine has finished or been stopped.
  bool IsCoroutineFinished(WScriptCoroutineHandle hCoroutine) const;

  ///@}

  /// Returns a shared expression VM for custom script implementations.
  ///
  /// The VM is NOT thread safe - only execute one expression at a time.
  WExpressionVM& GetSharedExpressionVM() { return m_SharedExpressionVM; }

  /// Context information for scheduled script functions.
  struct FunctionContext
  {
    /// Flags controlling when the function should be executed.
    enum Flags : WUInt8
    {
      None,              ///< Execute always
      OnlyWhenSimulating ///< Execute only during simulation
    };

    WPointerWithFlags<const WAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags == other.m_pFunctionAndFlags && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const WWorldModule::UpdateContext& context);

  WIntervalScheduler<FunctionContext> m_Scheduler;

  WIdTable<WScriptCoroutineId, WUniquePtr<WScriptCoroutine>> m_RunningScriptCoroutines;
  WHashTable<WScriptInstance*, WSmallArray<WScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  WDynamicArray<WUniquePtr<WScriptCoroutine>> m_DeadScriptCoroutines;

  WExpressionVM m_SharedExpressionVM;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct WHashHelper<WScriptWorldModule::FunctionContext>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WScriptWorldModule::FunctionContext& value)
  {
    WUInt32 hash = WHashHelper<const void*>::Hash(value.m_pFunctionAndFlags);
    hash = WHashingUtils::CombineHashValues32(hash, WHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  W_ALWAYS_INLINE static bool Equal(const WScriptWorldModule::FunctionContext& a, const WScriptWorldModule::FunctionContext& b) { return a == b; }
};
