#pragma once

template <typename Type>
W_ALWAYS_INLINE WRectTemplate<Type>::WRectTemplate() = default;

template <typename Type>
W_ALWAYS_INLINE WRectTemplate<Type>::WRectTemplate(Type x, Type y, Type width, Type height)
  : x(x)
  , y(y)
  , width(width)
  , height(height)
{
}

template <typename Type>
W_ALWAYS_INLINE WRectTemplate<Type>::WRectTemplate(Type width, Type height)
  : x(0)
  , y(0)
  , width(width)
  , height(height)
{
}

template <typename Type>
W_ALWAYS_INLINE WRectTemplate<Type>::WRectTemplate(const WVec2Template<Type>& vTopLeftPosition, const WVec2Template<Type>& vSize)
{
  x = vTopLeftPosition.x;
  y = vTopLeftPosition.y;
  width = vSize.x;
  height = vSize.y;
}

template <typename Type>
WRectTemplate<Type> WRectTemplate<Type>::MakeInvalid()
{
  /// \test This is new

  WRectTemplate<Type> res;

  const Type fLargeValue = WMath::MaxValue<Type>() / 2;
  res.x = fLargeValue;
  res.y = fLargeValue;
  res.width = -fLargeValue;
  res.height = -fLargeValue;

  return res;
}

template <typename Type>
WRectTemplate<Type> WRectTemplate<Type>::MakeZero()
{
  return WRectTemplate<Type>(0, 0, 0, 0);
}

template <typename Type>
WRectTemplate<Type> WRectTemplate<Type>::MakeIntersection(const WRectTemplate<Type>& r0, const WRectTemplate<Type>& r1)
{
  /// \test This is new

  WRectTemplate<Type> res;

  Type x1 = WMath::Max(r0.GetX1(), r1.GetX1());
  Type y1 = WMath::Max(r0.GetY1(), r1.GetY1());
  Type x2 = WMath::Min(r0.GetX2(), r1.GetX2());
  Type y2 = WMath::Min(r0.GetY2(), r1.GetY2());

  res.x = x1;
  res.y = y1;
  res.width = x2 - x1;
  res.height = y2 - y1;

  return res;
}

template <typename Type>
WRectTemplate<Type> WRectTemplate<Type>::MakeUnion(const WRectTemplate<Type>& r0, const WRectTemplate<Type>& r1)
{
  /// \test This is new

  WRectTemplate<Type> res;

  Type x1 = WMath::Min(r0.GetX1(), r1.GetX1());
  Type y1 = WMath::Min(r0.GetY1(), r1.GetY1());
  Type x2 = WMath::Max(r0.GetX2(), r1.GetX2());
  Type y2 = WMath::Max(r0.GetY2(), r1.GetY2());

  res.x = x1;
  res.y = y1;
  res.width = x2 - x1;
  res.height = y2 - y1;

  return res;
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::operator==(const WRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::operator!=(const WRectTemplate<Type>& rhs) const
{
  return !(*this == rhs);
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::Contains(const WVec2Template<Type>& vPoint) const
{
  if (vPoint.x >= x && vPoint.x <= Right())
  {
    if (vPoint.y >= y && vPoint.y <= Bottom())
      return true;
  }

  return false;
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::Contains(const WRectTemplate<Type>& r) const
{
  return r.x >= x && r.y >= y && r.Right() <= Right() && r.Bottom() <= Bottom();
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::Overlaps(const WRectTemplate<Type>& other) const
{
  if (x < other.Right() && Right() > other.x && y < other.Bottom() && Bottom() > other.y)
    return true;

  return false;
}

template <typename Type>
void WRectTemplate<Type>::ExpandToInclude(const WRectTemplate<Type>& other)
{
  Type thisRight = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.Right() > thisRight)
    width = other.Right() - x;
  else
    width = thisRight - x;

  if (other.Bottom() > thisBottom)
    height = other.Bottom() - y;
  else
    height = thisBottom - y;
}

template <typename Type>
void WRectTemplate<Type>::ExpandToInclude(const WVec2Template<Type>& other)
{
  Type thisRight = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.x > thisRight)
    width = other.x - x;
  else
    width = thisRight - x;

  if (other.y > thisBottom)
    height = other.y - y;
  else
    height = thisBottom - y;
}

template <typename Type>
void WRectTemplate<Type>::Grow(Type xy)
{
  x -= xy;
  y -= xy;
  width += xy * 2;
  height += xy * 2;
}

template <typename Type>
W_ALWAYS_INLINE void WRectTemplate<Type>::Clip(const WRectTemplate<Type>& clipRect)
{
  Type newLeft = WMath::Max<Type>(x, clipRect.x);
  Type newTop = WMath::Max<Type>(y, clipRect.y);

  Type newRight = WMath::Min<Type>(Right(), clipRect.Right());
  Type newBottom = WMath::Min<Type>(Bottom(), clipRect.Bottom());

  x = newLeft;
  y = newTop;
  width = newRight - newLeft;
  height = newBottom - newTop;
}

template <typename Type>
W_ALWAYS_INLINE bool WRectTemplate<Type>::IsValid() const
{
  /// \test This is new

  return width >= 0 && height >= 0;
}

template <typename Type>
W_ALWAYS_INLINE const WVec2Template<Type> WRectTemplate<Type>::GetClampedPoint(const WVec2Template<Type>& vPoint) const
{
  /// \test This is new

  return WVec2Template<Type>(WMath::Clamp(vPoint.x, Left(), Right()), WMath::Clamp(vPoint.y, Top(), Bottom()));
}

template <typename Type>
void WRectTemplate<Type>::SetCenter(Type tX, Type tY)
{
  /// \test This is new

  x = tX - width / 2;
  y = tY - height / 2;
}

template <typename Type>
void WRectTemplate<Type>::Translate(Type tX, Type tY)
{
  /// \test This is new

  x += tX;
  y += tY;
}

template <typename Type>
void WRectTemplate<Type>::Scale(Type sX, Type sY)
{
  /// \test This is new

  x *= sX;
  y *= sY;
  width *= sX;
  height *= sY;
}
