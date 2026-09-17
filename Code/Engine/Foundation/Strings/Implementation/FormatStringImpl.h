#pragma once

#include <array>
#include <tuple>
#include <utility>

template <typename... ARGS>
class WFormatStringImpl : public WFormatString
{
  // this is the size of the temp buffer that BuildString functions get for writing their result to.
  // The buffer is always available and allocated on the stack, so this prevents the need for memory allocations.
  // If a BuildString function requires no storage at all, it can return an WStringView to unrelated memory
  // (e.g. if the memory already exists).
  // If a BuildString function requires more storage, it may need to do some trickery.
  // For an example look at BuildString for WArgErrorCode, which uses an increased thread_local temp buffer.
  static constexpr WUInt32 TempStringLength = 64;

public:
  WFormatStringImpl(WStringView sFormat, ARGS&&... args)
    : m_Arguments(std::forward<ARGS>(args)...)
  {
    m_sString = sFormat;
  }

  WFormatStringImpl(const char* szFormat, ARGS&&... args)
    : m_Arguments(std::forward<ARGS>(args)...)
  {
    m_sString = szFormat;
  }

  /// Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an WStringBuilder as storage, ie. writes the formatted text into it. Additionally it returns a const char* to that
  /// string builder data for convenience.
  virtual WStringView GetText(WStringBuilder& ref_sStorage) const override
  {
    if (m_sString.IsEmpty())
    {
      return {};
    }

    WStringView param[MaxNumParameters];

    char tmp[MaxNumParameters][TempStringLength];
    ReplaceString<0>(tmp, param);

    return BuildFormattedText(ref_sStorage, param, MaxNumParameters);
  }

  virtual const char* GetTextCStr(WStringBuilder& out_sString) const override
  {
    WStringView param[MaxNumParameters];

    char tmp[MaxNumParameters][TempStringLength];
    ReplaceString<0>(tmp, param);

    return BuildFormattedText(out_sString, param, MaxNumParameters).GetStartPointer();
  }

private:
  template <WInt32 N>
  typename std::enable_if<sizeof...(ARGS) != N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], WStringView* pViews) const
  {
    static_assert(N < MaxNumParameters, "Maximum number of format arguments reached");

    // using a free function allows to overload with various different argument types
    pViews[N] = BuildString(tmp[N], TempStringLength - 1, std::get<N>(m_Arguments));

    // Recurse, chip off one argument
    ReplaceString<N + 1>(tmp, pViews);
  }

  // Recursion end if we reached the number of arguments.
  template <WInt32 N>
  typename std::enable_if<sizeof...(ARGS) == N>::type ReplaceString(char tmp[MaxNumParameters][TempStringLength], WStringView* pViews) const
  {
    W_IGNORE_UNUSED(tmp);
    W_IGNORE_UNUSED(pViews);
  }


  // stores the arguments
  std::tuple<ARGS...> m_Arguments;
};
