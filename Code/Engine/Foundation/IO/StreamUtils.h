
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

namespace WStreamUtils
{
  /// Reads all the remaining data in \a stream and appends it to \a destination.
  W_FOUNDATION_DLL void ReadAllAndAppend(WStreamReader& inout_stream, WDynamicArray<WUInt8>& ref_destination);

} // namespace WStreamUtils
