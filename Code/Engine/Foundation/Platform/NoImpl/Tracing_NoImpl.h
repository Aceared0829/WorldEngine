#pragma once

// Null tracing backend — all macros expand to nothing. Used when W_USE_TRACING is on but no platform backend is available.

#define W_TRACE_INTERNAL_VALUE(FieldName, Value)

#define W_TRACE_INTERNAL_EVENT(EventName, Level, ...) \
  do                                                   \
  {                                                    \
  } while (false)

#define W_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ...) \
  do                                                         \
  {                                                          \
  } while (false)

#define W_TRACE_INTERNAL_SCOPE_END(EventName) \
  do                                           \
  {                                            \
  } while (false)

#define W_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ...) \
  do                                                             \
  {                                                              \
  } while (false)

#define W_TRACE_INTERNAL_ASYNC_END(EventName, Id) \
  do                                               \
  {                                                \
  } while (false)

#ifndef W_DECLARE_TRACE_PROVIDER
#  define W_DECLARE_TRACE_PROVIDER(ProviderSymbol)
#endif

#ifndef W_IMPLEMENT_TRACE_PROVIDER
#  define W_IMPLEMENT_TRACE_PROVIDER(ProviderSymbol, ProviderName)
#endif
