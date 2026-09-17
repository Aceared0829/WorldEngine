
bool WUuid::operator==(const WUuid& other) const
{
  return m_uiHigh == other.m_uiHigh && m_uiLow == other.m_uiLow;
}

bool WUuid::operator!=(const WUuid& other) const
{
  return m_uiHigh != other.m_uiHigh || m_uiLow != other.m_uiLow;
}

bool WUuid::operator<(const WUuid& other) const
{
  if (m_uiHigh < other.m_uiHigh)
    return true;
  if (m_uiHigh > other.m_uiHigh)
    return false;

  return m_uiLow < other.m_uiLow;
}

bool WUuid::IsValid() const
{
  return m_uiHigh != 0 || m_uiLow != 0;
}

void WUuid::CombineWithSeed(const WUuid& seed)
{
  m_uiHigh += seed.m_uiHigh;
  m_uiLow += seed.m_uiLow;
}

void WUuid::RevertCombinationWithSeed(const WUuid& seed)
{
  m_uiHigh -= seed.m_uiHigh;
  m_uiLow -= seed.m_uiLow;
}

void WUuid::HashCombine(const WUuid& guid)
{
  m_uiHigh = WHashingUtils::xxHash64(&guid.m_uiHigh, sizeof(WUInt64), m_uiHigh);
  m_uiLow = WHashingUtils::xxHash64(&guid.m_uiLow, sizeof(WUInt64), m_uiLow);
}

template <>
struct WHashHelper<WUuid>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WUuid& value) { return WHashingUtils::xxHash32(&value, sizeof(WUuid)); }

  W_ALWAYS_INLINE static bool Equal(const WUuid& a, const WUuid& b) { return a == b; }
};
