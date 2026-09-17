#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>

W_IMPLEMENT_SERIALIZATION_CONTEXT(WDeduplicationReadContext);

WDeduplicationReadContext::WDeduplicationReadContext() = default;
WDeduplicationReadContext::~WDeduplicationReadContext() = default;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_SERIALIZATION_CONTEXT(WDeduplicationWriteContext);

WDeduplicationWriteContext::WDeduplicationWriteContext() = default;
WDeduplicationWriteContext::~WDeduplicationWriteContext() = default;
