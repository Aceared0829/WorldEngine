#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>

WUuid WUuid::MakeStableUuidFromString(WStringView sString)
{
  WUuid NewUuid;
  NewUuid.m_uiLow = WHashingUtils::xxHash64String(sString);
  NewUuid.m_uiHigh = WHashingUtils::xxHash64String(sString, 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}

WUuid WUuid::MakeStableUuidFromInt(WInt64 iInt)
{
  WUuid NewUuid;
  NewUuid.m_uiLow = WHashingUtils::xxHash64(&iInt, sizeof(WInt64));
  NewUuid.m_uiHigh = WHashingUtils::xxHash64(&iInt, sizeof(WInt64), 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}
