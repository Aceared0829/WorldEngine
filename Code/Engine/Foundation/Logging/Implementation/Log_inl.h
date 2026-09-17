#pragma once

#if W_DISABLED(W_COMPILE_FOR_DEVELOPMENT)

inline void WLog::Dev(WLogInterface* /*pInterface*/, const WFormatString& /*string*/) {}

#endif

#if W_DISABLED(W_COMPILE_FOR_DEBUG)

inline void WLog::Debug(WLogInterface* /*pInterface*/, const WFormatString& /*string*/)
{
}

#endif
