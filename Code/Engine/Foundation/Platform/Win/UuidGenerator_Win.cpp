#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Types/Uuid.h>

#  include <combaseapi.h>
#  include <rpc.h>

static_assert(sizeof(WUInt64) * 2 == sizeof(UUID));

WUuid WUuid::MakeUuid()
{
  WUInt64 uiUuidData[2];

  // this works on desktop Windows
  // UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // this also works on UWP
  GUID* guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  HRESULT hr = CoCreateGuid(guid);
  W_IGNORE_UNUSED(hr);
  W_ASSERT_DEBUG(SUCCEEDED(hr), "CoCreateGuid failed, guid might be invalid!");

  return WUuid(uiUuidData[1], uiUuidData[0]);
}

#endif
