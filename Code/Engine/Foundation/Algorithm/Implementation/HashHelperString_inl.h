
WUInt32 WHashHelperString_NoCase::Hash(WStringView value)
{
  WHybridArray<char, 256> temp;
  temp.SetCountUninitialized(value.GetElementCount());
  WMemoryUtils::Copy(temp.GetData(), value.GetStartPointer(), value.GetElementCount());
  const WUInt32 uiElemCount = WStringUtils::ToLowerString(temp.GetData(), temp.GetData() + value.GetElementCount());

  return WHashingUtils::StringHashTo32(WHashingUtils::xxHash64((void*)temp.GetData(), uiElemCount));
}

bool WHashHelperString_NoCase::Equal(WStringView lhs, WStringView rhs)
{
  return lhs.IsEqual_NoCase(rhs);
}
