#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/StackTracer.h>

#include <Foundation/Math/Math.h>
#if __has_include(<cxxabi.h>)
#  include <cxxabi.h>
#  define HAS_CXXABI 1
#endif
#if __has_include(<execinfo.h>)
#  include <execinfo.h>
#  define HAS_EXECINFO 1
#endif

#if __has_include(<dlfcn.h>)
#  include <dlfcn.h>
#  define HAS_DLFCN 1
#endif
void WStackTracer::OnPluginEvent(const WPluginEvent& e)
{
}

// static
WUInt32 WStackTracer::GetStackTrace(WArrayPtr<void*>& trace, void* pContext)
{
#if HAS_EXECINFO
  return backtrace(trace.GetPtr(), trace.GetCount());
#else
  return 0;
#endif
}

// static
void WStackTracer::ResolveStackTrace(const WArrayPtr<void*>& trace, PrintFunc printFunc)
{
#if HAS_EXECINFO
  char szBuffer[512] = {0};

  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());

  // Demangle if possible, otherwise fallback to backtrace_symbols output.
  if (ppSymbols != nullptr)
  {
    for (WUInt32 i = 0; i < trace.GetCount(); i++)
    {
#  if HAS_DLFCN && HAS_CXXABI
      Dl_info info{0};
      if (dladdr(trace[i], &info))
      {
        int iStatus = 0;
        char* szDemangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &iStatus);
        W_SCOPE_EXIT(free(szDemangled));
        if (szDemangled != nullptr)
        {
          WUInt32 uiOffset = static_cast<WUInt32>((char*)trace[i] - (char*)info.dli_saddr);
          WStringUtils::snprintf(szBuffer, W_ARRAY_SIZE(szBuffer), "%s(%s+0x%x) [0x%llx]\n", info.dli_fname, szDemangled, uiOffset, (WUInt64)trace[i]);
          printFunc(szBuffer);
          continue;
        }
      }
#  endif
      WStringUtils::snprintf(szBuffer, W_ARRAY_SIZE(szBuffer), "%s\n", ppSymbols[i]);
      printFunc(szBuffer);
    }
#  if HAS_DLFCN
    // Addr2line commands:
    printFunc("*** Run in terminal to resolve file and line callstack: ***");
    for (WUInt32 i = 0; i < trace.GetCount(); i++)
    {
      Dl_info info{0};
      if (dladdr(trace[i], &info))
      {
        ptrdiff_t offset = (char*)trace[i] - (char*)info.dli_fbase;
        WStringUtils::snprintf(szBuffer, W_ARRAY_SIZE(szBuffer), "addr2line -e %s -C -f -s -p 0x%llx\n", info.dli_fname, (WUInt64)offset);
        printFunc(szBuffer);
      }
    }
  }
#  endif
  free(ppSymbols);

#else
  printFunc("Could not record stack trace on this Linux system, because execinfo.h is not available.");
#endif
}
