#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include StringIterator.h directly, but instead include Foundation/Basics.h"
#endif

/// STL forward iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the first character of the string and ends at the address beyond the last character of the string.
struct WStringIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = WUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = WUInt32;

  W_DECLARE_POD_TYPE();

  /// Constructs an invalid iterator.
  W_ALWAYS_INLINE WStringIterator() = default; // [tested]

  /// Constructs either a begin or end iterator for the given string.
  W_FORCE_INLINE explicit WStringIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr)
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;
  }

  /// Checks whether this iterator points to a valid element. Invalid iterators either point to m_pEndPtr or were never initialized.
  W_ALWAYS_INLINE bool IsValid() const { return m_pCurPtr != nullptr && m_pCurPtr != m_pEndPtr; } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  W_ALWAYS_INLINE WUInt32 GetCharacter() const { return IsValid() ? WUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : WUInt32(0); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  W_ALWAYS_INLINE WUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// Returns the address the iterator currently points to.
  W_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// Checks whether the two iterators point to the same element.
  W_ALWAYS_INLINE bool operator==(const WStringIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WStringIterator&);

  /// Advances the iterated to the next character, same as operator++, but returns how many bytes were consumed in the source string.
  W_ALWAYS_INLINE WUInt32 Advance()
  {
    const char* pPrevElement = m_pCurPtr;

    if (m_pCurPtr < m_pEndPtr)
    {
      WUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return static_cast<WUInt32>(m_pCurPtr - pPrevElement);
  }

  /// Move to the next Utf8 character
  W_ALWAYS_INLINE WStringIterator& operator++() // [tested]
  {
    if (m_pCurPtr < m_pEndPtr)
    {
      WUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return *this;
  }

  /// Move to the previous Utf8 character
  W_ALWAYS_INLINE WStringIterator& operator--() // [tested]
  {
    if (m_pStartPtr < m_pCurPtr)
    {
      WUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }

    return *this;
  }

  /// Move to the next Utf8 character
  W_ALWAYS_INLINE WStringIterator operator++(int) // [tested]
  {
    WStringIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// Move to the previous Utf8 character
  W_ALWAYS_INLINE WStringIterator operator--(int) // [tested]
  {
    WStringIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  W_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  W_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// Returns an iterator that is advanced forwards by d characters.
  W_ALWAYS_INLINE WStringIterator operator+(difference_type d) const // [tested]
  {
    WStringIterator it = *this;
    it += d;
    return it;
  }

  /// Returns an iterator that is advanced backwards by d characters.
  W_ALWAYS_INLINE WStringIterator operator-(difference_type d) const // [tested]
  {
    WStringIterator it = *this;
    it -= d;
    return it;
  }

  /// Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos)
  {
    W_ASSERT_DEV((szCurPos >= m_pStartPtr) && (szCurPos <= m_pEndPtr), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};


/// STL reverse iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the last character of the string and ends at the address before the first character of the string.
struct WStringReverseIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = WUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = WUInt32;

  W_DECLARE_POD_TYPE();

  /// Constructs an invalid iterator.
  W_ALWAYS_INLINE WStringReverseIterator() = default; // [tested]

  /// Constructs either a rbegin or rend iterator for the given string.
  W_FORCE_INLINE explicit WStringReverseIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr) // [tested]
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;

    if (m_pStartPtr >= m_pEndPtr)
    {
      m_pCurPtr = nullptr;
    }
    else if (m_pCurPtr == m_pEndPtr)
    {
      WUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }
  }

  /// Checks whether this iterator points to a valid element.
  W_ALWAYS_INLINE bool IsValid() const { return (m_pCurPtr != nullptr); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  W_ALWAYS_INLINE WUInt32 GetCharacter() const { return IsValid() ? WUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : WUInt32(0); } // [tested]

  /// Returns the currently pointed to character in Utf32 encoding.
  W_ALWAYS_INLINE WUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// Returns the address the iterator currently points to.
  W_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// Checks whether the two iterators point to the same element.
  W_ALWAYS_INLINE bool operator==(const WStringReverseIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WStringReverseIterator&);

  /// Move to the next Utf8 character
  W_FORCE_INLINE WStringReverseIterator& operator++() // [tested]
  {
    if (m_pCurPtr != nullptr && m_pStartPtr < m_pCurPtr)
      WUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    else
      m_pCurPtr = nullptr;

    return *this;
  }

  /// Move to the previous Utf8 character
  W_FORCE_INLINE WStringReverseIterator& operator--() // [tested]
  {
    if (m_pCurPtr != nullptr)
    {
      const char* szOldPos = m_pCurPtr;
      WUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();

      if (m_pCurPtr == m_pEndPtr)
        m_pCurPtr = szOldPos;
    }
    else
    {
      // Set back to the first character.
      m_pCurPtr = m_pStartPtr;
    }
    return *this;
  }

  /// Move to the next Utf8 character
  W_ALWAYS_INLINE WStringReverseIterator operator++(int) // [tested]
  {
    WStringReverseIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// Move to the previous Utf8 character
  W_ALWAYS_INLINE WStringReverseIterator operator--(int) // [tested]
  {
    WStringReverseIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  W_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  W_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// Returns an iterator that is advanced forwards by d characters.
  W_ALWAYS_INLINE WStringReverseIterator operator+(difference_type d) const // [tested]
  {
    WStringReverseIterator it = *this;
    it += d;
    return it;
  }

  /// Returns an iterator that is advanced backwards by d characters.
  W_ALWAYS_INLINE WStringReverseIterator operator-(difference_type d) const // [tested]
  {
    WStringReverseIterator it = *this;
    it -= d;
    return it;
  }

  /// Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  W_FORCE_INLINE void SetCurrentPosition(const char* szCurPos)
  {
    W_ASSERT_DEV((szCurPos == nullptr) || ((szCurPos >= m_pStartPtr) && (szCurPos < m_pEndPtr)), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};
