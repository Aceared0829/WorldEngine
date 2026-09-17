#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: message-impl
W_IMPLEMENT_MESSAGE_TYPE(WMsgSetText);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetText, 1, WRTTIDefaultAllocator<WMsgSetText>)
W_END_DYNAMIC_REFLECTED_TYPE;
// END-DOCS-CODE-SNIPPET
// clang-format on
