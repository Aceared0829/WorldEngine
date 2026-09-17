#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class WVisualScriptInstance;
class WVisualScriptExecutionContext;

struct W_VISUALSCRIPTPLUGIN_DLL WVisualScriptNodeDescription
{
  /// Native node types for visual script graphs.
  /// Editor only types are not supported at runtime and will be replaced by the visual script compiler during asset transform.
  struct W_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      EntryCall_Coroutine,
      MessageHandler,
      MessageHandler_Coroutine,
      ReflectedFunction,
      GetReflectedProperty,
      SetReflectedProperty,
      InplaceCoroutine,
      GetScriptOwner,
      SendMessage,

      FirstBuiltin,

      Builtin_Constant,    // Editor only
      Builtin_GetVariable, // Editor only
      Builtin_SetVariable,
      Builtin_IncVariable,
      Builtin_DecVariable,
      Builtin_TempVariable,

      Builtin_Branch,
      Builtin_Switch,
      Builtin_WhileLoop,          // Editor only
      Builtin_ForLoop,            // Editor only
      Builtin_ForEachLoop,        // Editor only
      Builtin_ReverseForEachLoop, // Editor only
      Builtin_Break,              // Editor only
      Builtin_Jump,               // Editor only

      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_CompareExec, // Editor only
      Builtin_IsValid,
      Builtin_Select,

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,
      Builtin_Modulo,
      Builtin_Min,
      Builtin_Max,
      Builtin_Clamp,
      Builtin_Expression,

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_ToHashedString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_String_Format,
      Builtin_String_GetCharacterCount,
      Builtin_String_IsEmpty,

      Builtin_MakeArray,
      Builtin_Array_GetElement,
      Builtin_Array_SetElement,
      Builtin_Array_GetCount,
      Builtin_Array_IsEmpty,
      Builtin_Array_Clear,
      Builtin_Array_Contains,
      Builtin_Array_IndexOf,
      Builtin_Array_Insert,
      Builtin_Array_PushBack,
      Builtin_Array_PushBackRange,
      Builtin_Array_Remove,
      Builtin_Array_RemoveAt,

      Builtin_CreateComponent,
      Builtin_TryGetComponentOfBaseType,

      Builtin_StartCoroutine,
      Builtin_StopCoroutine,
      Builtin_StopAllCoroutines,
      Builtin_WaitForAll,
      Builtin_WaitForAny,
      Builtin_Yield,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    W_ALWAYS_INLINE static bool IsEntry(Enum type) { return type >= EntryCall && type <= MessageHandler_Coroutine; }
    W_ALWAYS_INLINE static bool IsLoop(Enum type) { return type >= Builtin_WhileLoop && type <= Builtin_ReverseForEachLoop; }

    W_ALWAYS_INLINE static bool MakesOuterCoroutine(Enum type) { return type == InplaceCoroutine || (type >= Builtin_WaitForAll && type <= Builtin_Yield); }

    W_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(WVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  using DataOffset = WVisualScriptDataDescription::DataOffset;

  WEnum<Type> m_Type;
  WEnum<WVisualScriptDataType> m_DeductedDataType;
  WSmallArray<WUInt16, 4> m_ExecutionIndices;
  WSmallArray<DataOffset, 4> m_InputDataOffsets;
  WSmallArray<DataOffset, 2> m_OutputDataOffsets;

  WHashedString m_sTargetTypeName;

  WVariant m_Value;

  void AppendUserDataName(WStringBuilder& out_sResult) const;
};

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptGraphDescription : public WRefCounted
{
  W_DISALLOW_COPY_AND_ASSIGN(WVisualScriptGraphDescription);

public:
  WVisualScriptGraphDescription();
  ~WVisualScriptGraphDescription();

  static WResult Serialize(WArrayPtr<const WVisualScriptNodeDescription> nodes, const WVisualScriptDataDescription& localDataDesc, WStreamWriter& inout_stream);
  WResult Deserialize(WStreamReader& inout_stream, const WVisualScriptDataDescription& instanceDataDesc, const WVisualScriptDataDescription& constantDataDesc);

  template <typename T, WUInt32 Size>
  struct EmbeddedArrayOrPointer
  {
    union
    {
      T m_Embedded[Size] = {};
      T* m_Ptr;
    };

    static void AddAdditionalDataSize(WArrayPtr<const T> a, WUInt32& inout_uiAdditionalDataSize);
    static void AddAdditionalDataSize(WUInt32 uiSize, WUInt32 uiAlignment, WUInt32& inout_uiAdditionalDataSize);

    T* Init(WUInt8 uiCount, WUInt32 uiAlignment, WUInt8*& inout_pAdditionalData);
    WResult ReadFromStream(WUInt8& out_uiCount, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData);
  };

  struct ExecResult
  {
    struct State
    {
      enum Enum
      {
        Completed = 0,
        ContinueLater = -1,

        Error = -100,
      };
    };

    static W_ALWAYS_INLINE ExecResult Completed() { return {0}; }
    static W_ALWAYS_INLINE ExecResult RunNext(int iExecSlot) { return {iExecSlot}; }
    static W_ALWAYS_INLINE ExecResult ContinueLater(WTime maxDelay) { return {State::ContinueLater, maxDelay}; }
    static W_ALWAYS_INLINE ExecResult Error() { return {State::Error}; }

    int m_NextExecAndState = 0;
    WTime m_MaxDelay = WTime::MakeZero();
  };

  struct Node;
  using ExecuteFunction = ExecResult (*)(WVisualScriptExecutionContext& inout_context, const Node& node);
  using DataOffset = WVisualScriptDataDescription::DataOffset;
  using ExecutionIndicesArray = EmbeddedArrayOrPointer<WUInt16, 4>;
  using InputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 4>;
  using OutputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 2>;
  using UserDataArray = EmbeddedArrayOrPointer<WUInt32, 4>;

  struct Node
  {
    ExecuteFunction m_Function = nullptr;
#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt32 m_uiPadding = 0;
#endif

    ExecutionIndicesArray m_ExecutionIndices;
    InputDataOffsetsArray m_InputDataOffsets;
    OutputDataOffsetsArray m_OutputDataOffsets;
    UserDataArray m_UserData;

    WEnum<WVisualScriptNodeDescription::Type> m_Type;
    WUInt8 m_NumExecutionIndices;
    WUInt8 m_NumInputDataOffsets;
    WUInt8 m_NumOutputDataOffsets;

    WUInt16 m_UserDataByteSize;
    WEnum<WVisualScriptDataType> m_DeductedDataType;
    WUInt8 m_Reserved = 0;

    WUInt32 GetExecutionIndex(WUInt32 uiSlot) const;
    DataOffset GetInputDataOffset(WUInt32 uiSlot) const;
    DataOffset GetOutputDataOffset(WUInt32 uiSlot) const;

    DataOffset* GetInputDataOffsets();
    DataOffset* GetOutputDataOffsets();

    template <typename T>
    static constexpr WUInt32 GetUserDataAlignment();

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    T& InitUserData(WUInt8*& inout_pAdditionalData, WUInt32 uiByteSize = sizeof(T), WUInt32 uiAlignment = GetUserDataAlignment<T>());
  };

  const Node* GetNode(WUInt32 uiIndex) const;

  bool IsCoroutine() const;
  WScriptMessageDesc GetMessageDesc() const;

  const WSharedPtr<const WVisualScriptDataDescription>& GetLocalDataDesc() const;

private:
  WArrayPtr<const Node> m_Nodes;
  WBlob m_Storage;

  WSharedPtr<const WVisualScriptDataDescription> m_pLocalDataDesc;
};


class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptExecutionContext
{
public:
  WVisualScriptExecutionContext(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc, WAllocator* pAllocator);
  ~WVisualScriptExecutionContext();

  void Initialize(WVisualScriptInstance& inout_instance, WArrayPtr<WVariant> arguments);
  void Deinitialize();

  using ExecResult = WVisualScriptGraphDescription::ExecResult;
  ExecResult Execute(WTime deltaTimeSinceLastExecution);

  WVisualScriptInstance& GetInstance() { return *m_pInstance; }

  using DataOffset = WVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  WTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const WRTTI* pType = nullptr);

  WVariant GetDataAsVariant(DataOffset dataOffset, const WRTTI* pExpectedType) const;
  void SetDataFromVariant(DataOffset dataOffset, const WVariant& value);

  WScriptCoroutine* GetCurrentCoroutine() { return m_pCurrentCoroutine; }
  void SetCurrentCoroutine(WScriptCoroutine* pCoroutine);

  WTime GetDeltaTimeSinceLastExecution();

private:
  WSharedPtr<const WVisualScriptGraphDescription> m_pDesc;
  WVisualScriptInstance* m_pInstance = nullptr;
  WUInt32 m_uiCurrentNode = 0;
  WUInt32 m_uiExecutionCounter = 0;
  WTime m_DeltaTimeSinceLastExecution;

  WVisualScriptDataStorage m_LocalDataStorage;
  WVisualScriptDataStorage* m_DataStorage[DataOffset::Source::Count] = {};

  WScriptCoroutine* m_pCurrentCoroutine = nullptr;
};

struct WVisualScriptSendMessageMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Direct,    ///< Directly send the message to the target game object
    Recursive, ///< Send the message to the target game object and its children
    Event,     ///< Send the message as event. \sa WGameObject::SendEventMessage()

    Default = Direct
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_VISUALSCRIPTPLUGIN_DLL, WVisualScriptSendMessageMode);

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
