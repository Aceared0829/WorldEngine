#pragma once

#include <Core/CoreDLL.h>

#include <Core/Scripting/ScriptRTTI.h>

class WScriptWorldModule;

using WScriptCoroutineId = WGenericId<20, 12>;

/// A handle to a script coroutine which can be used to determine whether a coroutine is still running
/// even after the underlying coroutine object has already been deleted.
///
/// \sa WScriptWorldModule::CreateCoroutine, WScriptWorldModule::IsCoroutineFinished
struct WScriptCoroutineHandle
{
  W_DECLARE_HANDLE_TYPE(WScriptCoroutineHandle, WScriptCoroutineId);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutineHandle);
W_DECLARE_CUSTOM_VARIANT_TYPE(WScriptCoroutineHandle);

/// Base class of script coroutines.
///
/// A coroutine is a function that can be distributed over multiple frames and behaves similar to a mini state machine.
/// That is why coroutines are actually individual objects that keep track of their state rather than simple functions.
/// At first Start() is called with the arguments of the coroutine followed by one or multiple calls to Update().
/// The return value of the Update() function determines whether the Update() function should be called again next frame
/// or at latest after the specified delay. If the Update() function returns completed the Stop() function is called and the
/// coroutine object is destroyed.
/// The WScriptWorldModule is used to create and manage coroutine objects. The coroutine can then either be started and
/// scheduled automatically by calling WScriptWorldModule::StartCoroutine or the
/// Start/Stop/Update function is called manually if the coroutine is embedded as a subroutine in another coroutine.
class W_CORE_DLL WScriptCoroutine
{
public:
  WScriptCoroutine();
  virtual ~WScriptCoroutine();

  WScriptCoroutineHandle GetHandle() { return WScriptCoroutineHandle(m_Id); }

  WStringView GetName() const { return m_sName; }

  WScriptInstance* GetScriptInstance() { return m_pInstance; }
  const WScriptInstance* GetScriptInstance() const { return m_pInstance; }

  WScriptWorldModule* GetScriptWorldModule() { return m_pOwnerModule; }
  const WScriptWorldModule* GetScriptWorldModule() const { return m_pOwnerModule; }

  struct Result
  {
    struct State
    {
      using StorageType = WUInt8;

      enum Enum
      {
        Invalid,
        Running,
        Completed,
        Failed,

        Default = Invalid,
      };
    };

    static W_ALWAYS_INLINE Result Running(WTime maxDelay = WTime::MakeZero()) { return {State::Running, maxDelay}; }
    static W_ALWAYS_INLINE Result Completed() { return {State::Completed}; }
    static W_ALWAYS_INLINE Result Failed() { return {State::Failed}; }

    WEnum<State> m_State;
    WTime m_MaxDelay = WTime::MakeZero();
  };

  virtual void StartWithVarargs(WArrayPtr<WVariant> arguments) = 0;
  virtual void Stop() {}
  virtual Result Update(WTime deltaTimeSinceLastUpdate) = 0;

  void UpdateAndSchedule(WTime deltaTimeSinceLastUpdate = WTime::MakeZero());

private:
  friend class WScriptWorldModule;
  void Initialize(WScriptCoroutineId id, WStringView sName, WScriptInstance& inout_instance, WScriptWorldModule& inout_ownerModule);
  void Deinitialize();

  static const WAbstractFunctionProperty* GetUpdateFunctionProperty();

  WScriptCoroutineId m_Id;
  WHashedString m_sName;
  WScriptInstance* m_pInstance = nullptr;
  WScriptWorldModule* m_pOwnerModule = nullptr;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutine);

/// Base class of coroutines which are implemented in C++ to allow automatic unpacking of the arguments from variants
template <typename Derived, class... Args>
class WTypedScriptCoroutine : public WScriptCoroutine
{
private:
  template <std::size_t... I>
  W_ALWAYS_INLINE void StartImpl(WArrayPtr<WVariant> arguments, std::index_sequence<I...>)
  {
    static_cast<Derived*>(this)->Start(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void StartWithVarargs(WArrayPtr<WVariant> arguments) override
  {
    StartImpl(arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};

/// Mode that decides what should happen if a new coroutine is created while there is already another coroutine running with the same name
/// on a given instance.
///
/// \sa WScriptWorldModule::CreateCoroutine
struct WScriptCoroutineCreationMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    StopOther,     ///< Stop the other coroutine before creating a new one with the same name
    DontCreateNew, ///< Don't create a new coroutine if there is already one running with the same name
    AllowOverlap,  ///< Allow multiple overlapping coroutines with the same name

    Default = StopOther
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptCoroutineCreationMode);

/// A coroutine type that stores a custom allocator.
///
/// The custom allocator allows to pass more data to the created coroutine object than the default allocator.
/// E.g. this is used to pass the visual script graph to a visual script coroutine without the user needing to know
/// that the coroutine is actually implemented in visual script.
class W_CORE_DLL WScriptCoroutineRTTI : public WRTTI, public WRefCountingImpl
{
public:
  WScriptCoroutineRTTI(WStringView sName, WUniquePtr<WRTTIAllocator>&& pAllocator);
  ~WScriptCoroutineRTTI();

private:
  WString m_sTypeNameStorage;
  WUniquePtr<WRTTIAllocator> m_pAllocatorStorage;
};

/// A function property that creates an instance of the given coroutine type and starts it immediately.
class W_CORE_DLL WScriptCoroutineFunctionProperty : public WScriptFunctionProperty
{
public:
  WScriptCoroutineFunctionProperty(WStringView sName, const WSharedPtr<WScriptCoroutineRTTI>& pType, WScriptCoroutineCreationMode::Enum creationMode);
  ~WScriptCoroutineFunctionProperty();

  virtual WFunctionType::Enum GetFunctionType() const override { return WFunctionType::Member; }
  virtual const WRTTI* GetReturnType() const override { return nullptr; }
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const override { return WPropertyFlags::Void; }
  virtual WUInt32 GetArgumentCount() const override { return 0; }

  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const override
  {
    W_IGNORE_UNUSED(uiParamIndex);
    return nullptr;
  }

  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const override
  {
    W_IGNORE_UNUSED(uiParamIndex);
    return WPropertyFlags::Void;
  }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override;

protected:
  WSharedPtr<WScriptCoroutineRTTI> m_pType;
  WEnum<WScriptCoroutineCreationMode> m_CreationMode;
};

/// A message handler that creates an instance of the given coroutine type and starts it immediately.
class W_CORE_DLL WScriptCoroutineMessageHandler : public WScriptMessageHandler
{
public:
  WScriptCoroutineMessageHandler(WStringView sName, const WScriptMessageDesc& desc, const WSharedPtr<WScriptCoroutineRTTI>& pType, WScriptCoroutineCreationMode::Enum creationMode);
  ~WScriptCoroutineMessageHandler();

  static void Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg);

protected:
  WHashedString m_sName;
  WSharedPtr<WScriptCoroutineRTTI> m_pType;
  WEnum<WScriptCoroutineCreationMode> m_CreationMode;
};

/// HashHelper implementation so coroutine handles can be used as key in a hash table. Also needed to store in a variant.
template <>
struct WHashHelper<WScriptCoroutineHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WScriptCoroutineHandle value) { return WHashHelper<WUInt32>::Hash(value.GetInternalID().m_Data); }

  W_ALWAYS_INLINE static bool Equal(WScriptCoroutineHandle a, WScriptCoroutineHandle b) { return a == b; }
};

/// Currently not implemented as it is not needed for coroutine handles.
W_ALWAYS_INLINE void operator<<(WStreamWriter& inout_stream, const WScriptCoroutineHandle& hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(hValue);
  W_ASSERT_NOT_IMPLEMENTED;
}

W_ALWAYS_INLINE void operator>>(WStreamReader& inout_stream, WScriptCoroutineHandle& ref_hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(ref_hValue);
  W_ASSERT_NOT_IMPLEMENTED;
}
