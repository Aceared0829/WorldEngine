#pragma once

///\todo optimize these methods if needed

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Exp(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_exp_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Exp(f.x()), WMath::Exp(f.y()), WMath::Exp(f.z()), WMath::Exp(f.w()));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Ln(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_log_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Ln(f.x()), WMath::Ln(f.y()), WMath::Ln(f.z()), WMath::Ln(f.w()));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Log2(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_log2_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Log2(f.x()), WMath::Log2(f.y()), WMath::Log2(f.z()), WMath::Log2(f.w()));
#endif
}

// static
W_FORCE_INLINE WSimdVec4i WSimdMath::Log2i(const WSimdVec4i& i)
{
  return WSimdVec4i(WMath::Log2i(i.x()), WMath::Log2i(i.y()), WMath::Log2i(i.z()), WMath::Log2i(i.w()));
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Log10(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_log10_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Log10(f.x()), WMath::Log10(f.y()), WMath::Log10(f.z()), WMath::Log10(f.w()));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Pow2(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_exp2_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Pow2(f.x()), WMath::Pow2(f.y()), WMath::Pow2(f.z()), WMath::Pow2(f.w()));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Sin(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_sin_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Sin(WAngle::MakeFromRadian(f.x())), WMath::Sin(WAngle::MakeFromRadian(f.y())), WMath::Sin(WAngle::MakeFromRadian(f.z())),
    WMath::Sin(WAngle::MakeFromRadian(f.w())));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Cos(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_cos_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Cos(WAngle::MakeFromRadian(f.x())), WMath::Cos(WAngle::MakeFromRadian(f.y())), WMath::Cos(WAngle::MakeFromRadian(f.z())),
    WMath::Cos(WAngle::MakeFromRadian(f.w())));
#endif
}

// static
W_FORCE_INLINE WSimdVec4f WSimdMath::Tan(const WSimdVec4f& f)
{
#if W_ENABLED(W_COMPILER_MSVC_PURE) && W_SIMD_IMPLEMENTATION == W_SIMD_IMPLEMENTATION_SSE
  return _mm_tan_ps(f.m_v);
#else
  return WSimdVec4f(WMath::Tan(WAngle::MakeFromRadian(f.x())), WMath::Tan(WAngle::MakeFromRadian(f.y())), WMath::Tan(WAngle::MakeFromRadian(f.z())),
    WMath::Tan(WAngle::MakeFromRadian(f.w())));
#endif
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdMath::ASin(const WSimdVec4f& f)
{
  return WSimdVec4f(WMath::Pi<float>() * 0.5f) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
W_FORCE_INLINE WSimdVec4f WSimdMath::ACos(const WSimdVec4f& f)
{
  WSimdVec4f x1 = f.Abs();
  WSimdVec4f x2 = x1.CompMul(x1);
  WSimdVec4f x3 = x2.CompMul(x1);

  WSimdVec4f s = x1 * -0.2121144f + WSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((WSimdVec4f(1.0f) - x1).GetSqrt());

  return WSimdVec4f::Select(f >= WSimdVec4f::MakeZero(), s, WSimdVec4f(WMath::Pi<float>()) - s);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
W_FORCE_INLINE WSimdVec4f WSimdMath::ATan(const WSimdVec4f& f)
{
  WSimdVec4f x = f.Abs();
  WSimdVec4f t0 = WSimdVec4f::Select(x < WSimdVec4f(1.0f), x, x.GetReciprocal());
  WSimdVec4f t1 = t0.CompMul(t0);
  WSimdVec4f poly = WSimdVec4f(0.0872929f);
  poly = WSimdVec4f(-0.301895f) + poly.CompMul(t1);
  poly = WSimdVec4f(1.0f) + poly.CompMul(t1);
  poly = poly.CompMul(t0);
  t0 = WSimdVec4f::Select(x < WSimdVec4f(1.0f), poly, WSimdVec4f(WMath::Pi<float>() * 0.5f) - poly);

  return WSimdVec4f::Select(f < WSimdVec4f::MakeZero(), -t0, t0);
}
