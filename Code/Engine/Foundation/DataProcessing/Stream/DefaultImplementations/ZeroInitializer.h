
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class WProcessingStream;

/// This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class W_FOUNDATION_DLL WProcessingStreamSpawnerZeroInitialized : public WProcessingStreamProcessor
{
  W_ADD_DYNAMIC_REFLECTION(WProcessingStreamSpawnerZeroInitialized, WProcessingStreamProcessor);

public:
  WProcessingStreamSpawnerZeroInitialized();

  /// Which stream to zero initialize
  void SetStreamName(WStringView sStreamName);

protected:
  virtual WResult UpdateStreamBindings() override;

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override { W_IGNORE_UNUSED(uiNumElements); }

  WHashedString m_sStreamName;

  WProcessingStream* m_pStream = nullptr;
};
