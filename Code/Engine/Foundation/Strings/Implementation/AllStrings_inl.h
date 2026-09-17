#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(const WStringBuilder& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <WUInt16 Size>
WHybridStringBase<Size>::WHybridStringBase(WStringBuilder&& rhs, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = std::move(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(const WStringBuilder& rhs)
  : WHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE WHybridString<Size, A>::WHybridString(WStringBuilder&& rhs)
  : WHybridStringBase<Size>(std::move(rhs), A::GetAllocator())
{
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(const WStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
}

template <WUInt16 Size>
void WHybridStringBase<Size>::operator=(WStringBuilder&& rhs)
{
  m_Data = std::move(rhs.m_Data);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(const WStringBuilder& rhs)
{
  WHybridStringBase<Size>::operator=(rhs);
}

template <WUInt16 Size, typename A>
W_ALWAYS_INLINE void WHybridString<Size, A>::operator=(WStringBuilder&& rhs)
{
  WHybridStringBase<Size>::operator=(std::move(rhs));
}

template <WUInt16 Size>
void WHybridStringBase<Size>::ReadAll(WStreamReader& inout_stream)
{
  Clear();

  WHybridArray<WUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  WUInt8 Temp[1024];

  while (true)
  {
    const WUInt32 uiRead = (WUInt32)inout_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(WArrayPtr<WUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}
