#pragma once

#include <Foundation/Tracing/Tracing.h>

// BEGIN-DOCS-CODE-SNIPPET: tracing-provider-header
W_DECLARE_TRACE_PROVIDER(g_WTrace_FoundationTest);

/// All W_TRACE_* macros in FoundationTest source files use this provider.
#define W_TRACE_PROVIDER g_WTrace_FoundationTest
// END-DOCS-CODE-SNIPPET
