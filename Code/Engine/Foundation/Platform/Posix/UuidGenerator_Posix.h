#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Types/Uuid.h>

#if __has_include(<uuid/uuid.h>)
#  include <uuid/uuid.h>
#  define HAS_UUID 1
#else
// #  error "uuid.h does not exist on this distro."
#  define HAS_UUID 0
#endif

#if HAS_UUID

static_assert(sizeof(WUInt64) * 2 == sizeof(uuid_t));

WUuid WUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  WUInt64* uiUuidData = reinterpret_cast<WUInt64*>(uuid);

  return WUuid(uiUuidData[1], uiUuidData[0]);
}

#else

WUuid WUuid::MakeUuid()
{
  W_REPORT_FAILURE("This distro doesn't have support for UUID generation.");
  return WUuid();
}

#endif
