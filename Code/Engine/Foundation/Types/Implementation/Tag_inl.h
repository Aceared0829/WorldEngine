#pragma once

WTag::WTag()


  = default;

bool WTag::operator==(const WTag& rhs) const
{
  return m_sTagString == rhs.m_sTagString;
}

bool WTag::operator!=(const WTag& rhs) const
{
  return m_sTagString != rhs.m_sTagString;
}

bool WTag::operator<(const WTag& rhs) const
{
  return m_sTagString < rhs.m_sTagString;
}

const WString& WTag::GetTagString() const
{
  return m_sTagString.GetString();
}

bool WTag::IsValid() const
{
  return m_uiBlockIndex != 0xFFFFFFFEu;
}
