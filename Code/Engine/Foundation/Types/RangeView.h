#pragma once

#include <Foundation/Types/Delegate.h>

/// This class uses delegates to define a range of values that can be enumerated using a forward iterator.
///
/// Can be used to create a contiguous view to elements of a certain type without the need for them to actually
/// exist in the same space or format. Think of IEnumerable in c# using composition via WDelegate instead of derivation.
/// ValueType defines the value type we are iterating over and IteratorType is the internal key to identify an element.
/// An example that creates a RangeView of strings that are stored in a linear array of structs.
/// \code{.cpp}
/// auto range = WRangeView<const char*, WUInt32>(
///   [this]()-> WUInt32 { return 0; },
///   [this]()-> WUInt32 { return array.GetCount(); },
///   [this](WUInt32& it) { ++it; },
///   [this](const WUInt32& it)-> const char* { return array[it].m_String; });
///
/// for (const char* szValue : range)
/// {
/// }
/// \endcode
template <typename ValueType, typename IteratorType>
class WRangeView
{
public:
  using BeginCallback = WDelegate<IteratorType()>;
  using EndCallback = WDelegate<IteratorType()>;
  using NextCallback = WDelegate<void(IteratorType&)>;
  using ValueCallback = WDelegate<ValueType(const IteratorType&)>;

  /// Initializes the WRangeView with the delegates used to enumerate the range.
  W_ALWAYS_INLINE WRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value);

  /// Const iterator, don't use directly, use ranged based for loops or call begin() end().
  struct ConstIterator
  {
    W_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = ConstIterator;
    using pointer = ConstIterator*;
    using reference = ConstIterator&;

    W_ALWAYS_INLINE ConstIterator(const ConstIterator& rhs) = default;
    W_FORCE_INLINE void Next();
    W_FORCE_INLINE ValueType Value() const;
    W_ALWAYS_INLINE ValueType operator*() const { return Value(); }
    W_ALWAYS_INLINE void operator++() { Next(); }
    W_FORCE_INLINE bool operator==(const typename WRangeView<ValueType, IteratorType>::ConstIterator& it2) const;
    W_FORCE_INLINE bool operator!=(const typename WRangeView<ValueType, IteratorType>::ConstIterator& it2) const;

  protected:
    W_FORCE_INLINE explicit ConstIterator(const WRangeView<ValueType, IteratorType>* view, IteratorType pos);

    friend class WRangeView<ValueType, IteratorType>;
    const WRangeView<ValueType, IteratorType>* m_pView = nullptr;
    IteratorType m_Pos;
  };

  /// Iterator, don't use directly, use ranged based for loops or call begin() end().
  struct Iterator : public ConstIterator
  {
    W_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = Iterator;
    using pointer = Iterator*;
    using reference = Iterator&;

    using ConstIterator::Value;
    W_ALWAYS_INLINE Iterator(const Iterator& rhs) = default;
    W_FORCE_INLINE ValueType Value();
    W_ALWAYS_INLINE ValueType operator*() { return Value(); }

  protected:
    W_FORCE_INLINE explicit Iterator(const WRangeView<ValueType, IteratorType>* view, IteratorType pos);
  };

  Iterator begin() { return Iterator(this, m_Begin()); }
  Iterator end() { return Iterator(this, m_End()); }
  ConstIterator begin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator end() const { return ConstIterator(this, m_End()); }
  ConstIterator cbegin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator cend() const { return ConstIterator(this, m_End()); }

private:
  friend struct Iterator;
  friend struct ConstIterator;

  BeginCallback m_Begin;
  EndCallback m_End;
  NextCallback m_Next;
  ValueCallback m_Value;
};

template <typename V, typename I>
typename WRangeView<V, I>::Iterator begin(WRangeView<V, I>& in_container)
{
  return in_container.begin();
}

template <typename V, typename I>
typename WRangeView<V, I>::ConstIterator begin(const WRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename WRangeView<V, I>::ConstIterator cbegin(const WRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename WRangeView<V, I>::Iterator end(WRangeView<V, I>& in_container)
{
  return in_container.end();
}

template <typename V, typename I>
typename WRangeView<V, I>::ConstIterator end(const WRangeView<V, I>& container)
{
  return container.cend();
}

template <typename V, typename I>
typename WRangeView<V, I>::ConstIterator cend(const WRangeView<V, I>& container)
{
  return container.cend();
}

#include <Foundation/Types/Implementation/RangeView_inl.h>
