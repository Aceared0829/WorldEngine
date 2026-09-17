#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcessingStreamProcessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WProcessingStreamProcessor::WProcessingStreamProcessor()

  = default;

WProcessingStreamProcessor::~WProcessingStreamProcessor()
{
  m_pStreamGroup = nullptr;
}



W_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamProcessor);
