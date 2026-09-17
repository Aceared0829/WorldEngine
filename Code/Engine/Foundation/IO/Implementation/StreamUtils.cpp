#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void WStreamUtils::ReadAllAndAppend(WStreamReader& inout_stream, WDynamicArray<WUInt8>& ref_destination)
{
  WUInt8 temp[1024 * 4];

  while (true)
  {
    const WUInt32 uiRead = (WUInt32)inout_stream.ReadBytes(temp, W_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    ref_destination.PushBackRange(WArrayPtr<WUInt8>(temp, uiRead));
  }
}
