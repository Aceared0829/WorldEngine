
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class WProcessingStreamGroup;

/// Base class for all stream processor implementations.
class W_FOUNDATION_DLL WProcessingStreamProcessor : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WProcessingStreamProcessor, WReflectedClass);

public:
  /// Base constructor
  WProcessingStreamProcessor();

  /// Base destructor.
  virtual ~WProcessingStreamProcessor();

  /// Used for sorting processors, to ensure a certain order. Lower priority == executed first.
  float m_fPriority = 0.0f;

protected:
  friend class WProcessingStreamGroup;

  /// Internal method which needs to be implemented, gets the concrete stream bindings.
  /// This is called every time the streams are resized. Implementations should check that their required streams exist and are of the correct data
  /// types.
  virtual WResult UpdateStreamBindings() = 0;

  /// This method needs to be implemented in order to initialize new elements to specific values.
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) = 0;

  /// The actual method which processes the data, will be called with the number of elements to process.
  virtual void Process(WUInt64 uiNumElements) = 0;

  /// Back pointer to the stream group - will be set to the owner stream group when adding the stream processor to the group.
  /// Can be used to get stream pointers in UpdateStreamBindings();
  WProcessingStreamGroup* m_pStreamGroup = nullptr;
};
