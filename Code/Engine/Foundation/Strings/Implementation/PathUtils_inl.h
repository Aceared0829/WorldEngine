#pragma once

W_ALWAYS_INLINE bool WPathUtils::IsPathSeparator(WUInt32 c)
{
  return (c == '/' || c == '\\');
}
