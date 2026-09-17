#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Types/UniquePtr.h>

class W_FOUNDATION_DLL WExpressionVM
{
public:
  WExpressionVM();
  ~WExpressionVM();

  void RegisterFunction(const WExpressionFunction& func);
  void UnregisterFunction(const WExpressionFunction& func);

  struct Flags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      MapStreamsByName = W_BIT(0),
      ScalarizeStreams = W_BIT(1),

      UserFriendly = MapStreamsByName | ScalarizeStreams,
      BestPerformance = 0,

      Default = UserFriendly
    };

    struct Bits
    {
      StorageType MapStreamsByName : 1;
      StorageType ScalarizeStreams : 1;
    };
  };

  WResult Execute(const WExpressionByteCode& byteCode, WArrayPtr<const WProcessingStream> inputs, WArrayPtr<WProcessingStream> outputs, WUInt32 uiNumInstances, const WExpression::GlobalData& globalData = WExpression::GlobalData(), WBitflags<Flags> flags = Flags::Default);

private:
  void RegisterDefaultFunctions();

  static WResult ScalarizeStreams(WArrayPtr<const WProcessingStream> streams, WDynamicArray<WProcessingStream>& out_ScalarizedStreams);
  static WResult AreStreamsScalarized(WArrayPtr<const WProcessingStream> streams);
  static WResult ValidateStream(const WProcessingStream& stream, const WExpression::StreamDesc& streamDesc, WStringView sStreamType, WUInt32 uiNumInstances);

  template <typename T>
  static WResult MapStreams(WArrayPtr<const WExpression::StreamDesc> streamDescs, WArrayPtr<T> streams, WStringView sStreamType, WUInt32 uiNumInstances, WBitflags<Flags> flags, WDynamicArray<T*>& out_MappedStreams);
  WResult MapFunctions(WArrayPtr<const WExpression::FunctionDesc> functionDescs, const WExpression::GlobalData& globalData);

  WDynamicArray<WExpression::Register, WAlignedAllocatorWrapper> m_Registers;

  WDynamicArray<WProcessingStream> m_ScalarizedInputs;
  WDynamicArray<WProcessingStream> m_ScalarizedOutputs;

  WDynamicArray<const WProcessingStream*> m_MappedInputs;
  WDynamicArray<WProcessingStream*> m_MappedOutputs;
  WDynamicArray<const WExpressionFunction*> m_MappedFunctions;

  WDynamicArray<WExpressionFunction> m_Functions;
  WHashTable<WHashedString, WUInt32> m_FunctionNamesToIndex;
};
