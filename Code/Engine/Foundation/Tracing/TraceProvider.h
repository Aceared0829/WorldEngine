#pragma once

#include <Foundation/Tracing/Tracing.h>

W_DECLARE_TRACE_PROVIDER(g_WTrace_Foundation);

/// All W_TRACE_* macros in Foundation source files use this provider.
#define W_TRACE_PROVIDER g_WTrace_Foundation
