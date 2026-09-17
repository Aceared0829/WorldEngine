
inline WColorLinear16f::WColorLinear16f() = default;

inline WColorLinear16f::WColorLinear16f(WFloat16 r, WFloat16 g, WFloat16 b, WFloat16 a)
  : r(r)
  , g(g)
  , b(b)
  , a(a)
{
}

inline WColorLinear16f::WColorLinear16f(const WColor& color)
  : r(color.r)
  , g(color.g)
  , b(color.b)
  , a(color.a)
{
}

inline WColor WColorLinear16f::ToLinearFloat() const
{
  return WColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
}
