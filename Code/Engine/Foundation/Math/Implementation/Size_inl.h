#pragma once

template <typename Type>
W_ALWAYS_INLINE WSizeTemplate<Type>::WSizeTemplate() = default;

template <typename Type>
W_ALWAYS_INLINE WSizeTemplate<Type>::WSizeTemplate(Type width, Type height)
  : width(width)
  , height(height)
{
}

template <typename Type>
W_ALWAYS_INLINE bool WSizeTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WSizeTemplate<Type>& v1, const WSizeTemplate<Type>& v2)
{
  return v1.height == v2.height && v1.width == v2.width;
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WSizeTemplate<Type>& v1, const WSizeTemplate<Type>& v2)
{
  return v1.height != v2.height || v1.width != v2.width;
}
