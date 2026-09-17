#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Tracks/ColorGradient.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WColorGradientColorCP, WNoBase, 1, WRTTIDefaultAllocator<WColorGradientColorCP>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Tick", m_iTick),
    W_MEMBER_PROPERTY("Red", m_GammaRed)->AddAttributes(new WDefaultValueAttribute(255)),
    W_MEMBER_PROPERTY("Green", m_GammaGreen)->AddAttributes(new WDefaultValueAttribute(255)),
    W_MEMBER_PROPERTY("Blue", m_GammaBlue)->AddAttributes(new WDefaultValueAttribute(255)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorGradientAlphaCP, WNoBase, 1, WRTTIDefaultAllocator<WColorGradientAlphaCP>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Tick", m_iTick),
    W_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new WDefaultValueAttribute(255)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorGradientIntensityCP, WNoBase, 1, WRTTIDefaultAllocator<WColorGradientIntensityCP>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Tick", m_iTick),
    W_MEMBER_PROPERTY("Intensity", m_Intensity)->AddAttributes(new WDefaultValueAttribute(1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WColorGradient, WNoBase, 1, WRTTIDefaultAllocator<WColorGradient>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("ColorCPs", m_ColorCPs),
    W_ARRAY_MEMBER_PROPERTY("AlphaCPs", m_AlphaCPs),
    W_ARRAY_MEMBER_PROPERTY("IntensityCPs", m_IntensityCPs),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WColorGradient::WColorGradient()
{
  Clear();
}

WColorGradient::WColorGradient(const WColorGradient& rhs)
{
  m_ColorCPs = rhs.m_ColorCPs;
  m_AlphaCPs = rhs.m_AlphaCPs;
  m_IntensityCPs = rhs.m_IntensityCPs;
  // Deliberately not copying m_ColorOrder, m_AlphaOrder, m_IntensityOrder, m_InitializationMutex
  // These will be rebuilt on first evaluation in the new instance
}

WColorGradient::WColorGradient(WColorGradient&& rhs) noexcept
{
  m_ColorCPs = std::move(rhs.m_ColorCPs);
  m_AlphaCPs = std::move(rhs.m_AlphaCPs);
  m_IntensityCPs = std::move(rhs.m_IntensityCPs);
  // Deliberately not moving m_ColorOrder, m_AlphaOrder, m_IntensityOrder, m_InitializationMutex
  // These will be rebuilt on first evaluation in the new instance
}

void WColorGradient::operator=(const WColorGradient& rhs)
{
  if (this == &rhs)
    return;

  m_ColorCPs = rhs.m_ColorCPs;
  m_AlphaCPs = rhs.m_AlphaCPs;
  m_IntensityCPs = rhs.m_IntensityCPs;
  m_ColorOrder.Clear();
  m_AlphaOrder.Clear();
  m_IntensityOrder.Clear();
  // m_InitializationMutex is not copied
}

void WColorGradient::operator=(WColorGradient&& rhs) noexcept
{
  if (this == &rhs)
    return;

  m_ColorCPs = std::move(rhs.m_ColorCPs);
  m_AlphaCPs = std::move(rhs.m_AlphaCPs);
  m_IntensityCPs = std::move(rhs.m_IntensityCPs);
  m_ColorOrder.Clear();
  m_AlphaOrder.Clear();
  m_IntensityOrder.Clear();
  // m_InitializationMutex is not moved
}

void WColorGradient::Clear()
{
  m_ColorCPs.Clear();
  m_AlphaCPs.Clear();
  m_IntensityCPs.Clear();
}


bool WColorGradient::IsEmpty() const
{
  return m_ColorCPs.IsEmpty() && m_AlphaCPs.IsEmpty() && m_IntensityCPs.IsEmpty();
}

void WColorGradient::AddColorControlPoint(double x, const WColorGammaUB& rgb)
{
  auto& cp = m_ColorCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_GammaRed = rgb.r;
  cp.m_GammaGreen = rgb.g;
  cp.m_GammaBlue = rgb.b;
}

void WColorGradient::AddAlphaControlPoint(double x, WUInt8 uiAlpha)
{
  auto& cp = m_AlphaCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_Alpha = uiAlpha;
}

void WColorGradient::AddIntensityControlPoint(double x, float fIntensity)
{
  auto& cp = m_IntensityCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_Intensity = fIntensity;
}

bool WColorGradient::GetExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  WInt64 minTick = WMath::MaxValue<WInt64>();
  WInt64 maxTick = WMath::MinValue<WInt64>();

  for (const auto& cp : m_ColorCPs)
  {
    minTick = WMath::Min(minTick, cp.m_iTick);
    maxTick = WMath::Max(maxTick, cp.m_iTick);
  }

  for (const auto& cp : m_AlphaCPs)
  {
    minTick = WMath::Min(minTick, cp.m_iTick);
    maxTick = WMath::Max(maxTick, cp.m_iTick);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    minTick = WMath::Min(minTick, cp.m_iTick);
    maxTick = WMath::Max(maxTick, cp.m_iTick);
  }

  if (minTick <= maxTick)
  {
    ref_fMinx = TickToTime(minTick);
    ref_fMaxx = TickToTime(maxTick);
    return true;
  }

  return false;
}

void WColorGradient::GetNumControlPoints(WUInt32& ref_uiRgb, WUInt32& ref_uiAlpha, WUInt32& ref_uiIntensity) const
{
  ref_uiRgb = m_ColorCPs.GetCount();
  ref_uiAlpha = m_AlphaCPs.GetCount();
  ref_uiIntensity = m_IntensityCPs.GetCount();
}


void WColorGradient::UpdatePointOrder() const
{
  // Create remapping arrays instead of sorting the actual arrays.
  // This preserves indices so editing operations in the UI don't break when control points are moved.

  m_ColorOrder.SetCount((WUInt16)m_ColorCPs.GetCount());
  for (WUInt8 i = 0; i < m_ColorCPs.GetCount(); ++i)
  {
    m_ColorOrder[i] = static_cast<WUInt8>(i);
  }
  m_ColorOrder.Sort([this](WUInt32 a, WUInt32 b)
    { return m_ColorCPs[a] < m_ColorCPs[b]; });

  // Alpha CPs
  m_AlphaOrder.SetCount((WUInt16)m_AlphaCPs.GetCount());
  for (WUInt8 i = 0; i < m_AlphaCPs.GetCount(); ++i)
  {
    m_AlphaOrder[i] = static_cast<WUInt8>(i);
  }
  m_AlphaOrder.Sort([this](WUInt32 a, WUInt32 b)
    { return m_AlphaCPs[a] < m_AlphaCPs[b]; });

  // Intensity CPs
  m_IntensityOrder.SetCount((WUInt16)m_IntensityCPs.GetCount());
  for (WUInt8 i = 0; i < m_IntensityCPs.GetCount(); ++i)
  {
    m_IntensityOrder[i] = static_cast<WUInt8>(i);
  }
  m_IntensityOrder.Sort([this](WUInt32 a, WUInt32 b)
    { return m_IntensityCPs[a] < m_IntensityCPs[b]; });

  PrecomputeLerpNormalizer();
}

void WColorGradient::PrecomputeLerpNormalizer() const
{
  for (WUInt32 i = 1; i < m_ColorOrder.GetCount(); ++i)
  {
    const WUInt32 idx0 = m_ColorOrder[i - 1];
    const WUInt32 idx1 = m_ColorOrder[i];

    const WInt64 tick0 = m_ColorCPs[idx0].m_iTick;
    const WInt64 tick1 = m_ColorCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_ColorCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }

  for (WUInt32 i = 1; i < m_AlphaOrder.GetCount(); ++i)
  {
    const WUInt32 idx0 = m_AlphaOrder[i - 1];
    const WUInt32 idx1 = m_AlphaOrder[i];

    const WInt64 tick0 = m_AlphaCPs[idx0].m_iTick;
    const WInt64 tick1 = m_AlphaCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_AlphaCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }

  for (WUInt32 i = 1; i < m_IntensityOrder.GetCount(); ++i)
  {
    const WUInt32 idx0 = m_IntensityOrder[i - 1];
    const WUInt32 idx1 = m_IntensityOrder[i];

    const WInt64 tick0 = m_IntensityCPs[idx0].m_iTick;
    const WInt64 tick1 = m_IntensityCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_IntensityCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }
}

void WColorGradient::Evaluate(double x, WColorGammaUB& ref_rgba, float& ref_fIntensity) const
{
  ref_rgba.r = 255;
  ref_rgba.g = 255;
  ref_rgba.b = 255;
  ref_rgba.a = 255;
  ref_fIntensity = 1.0f;

  EvaluateColor(x, ref_rgba);
  EvaluateAlpha(x, ref_rgba.a);
  EvaluateIntensity(x, ref_fIntensity);
}


void WColorGradient::Evaluate(double x, WColor& ref_hdr) const
{
  float intensity = 1.0f;
  WUInt8 alpha = 255;

  EvaluateColor(x, ref_hdr);
  EvaluateAlpha(x, alpha);
  EvaluateIntensity(x, intensity);

  ref_hdr.ScaleRGB(intensity);
  ref_hdr.a = WMath::ColorByteToFloat(alpha);
}

void WColorGradient::EvaluateColor(double x, WColorGammaUB& ref_rgb) const
{
  WColor hdr;
  EvaluateColor(x, hdr);

  ref_rgb = hdr;
  ref_rgb.a = 255;
}

void WColorGradient::EvaluateColor(double x, WColor& ref_rgb) const
{
  if (m_ColorCPs.GetCount() != m_ColorOrder.GetCount())
  {
    W_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_ColorCPs.GetCount() != m_ColorOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_rgb.r = 1.0f;
  ref_rgb.g = 1.0f;
  ref_rgb.b = 1.0f;
  ref_rgb.a = 1.0f;

  const WUInt32 numCPs = m_ColorCPs.GetCount();

  if (numCPs >= 2)
  {
    const WInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping to access first CP in sorted order
    const WUInt32 firstIdx = m_ColorOrder[0];
    if (m_ColorCPs[firstIdx].m_iTick >= xTick)
    {
      const ColorCP& cp = m_ColorCPs[firstIdx];
      ref_rgb = WColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

    WUInt32 uiControlPoint;

    for (WUInt32 i = 1; i < numCPs; ++i)
    {
      const WUInt32 idx = m_ColorOrder[i];
      if (m_ColorCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const WUInt32 lastIdx = m_ColorOrder[numCPs - 1];
      const ColorCP& cp = m_ColorCPs[lastIdx];
      ref_rgb = WColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

  found:
  {
    const WUInt32 idxL = m_ColorOrder[uiControlPoint];
    const WUInt32 idxR = m_ColorOrder[uiControlPoint + 1];

    const ColorCP& cpl = m_ColorCPs[idxL];
    const ColorCP& cpr = m_ColorCPs[idxR];

    const WColor lhs(WColorGammaUB(cpl.m_GammaRed, cpl.m_GammaGreen, cpl.m_GammaBlue, 255));
    const WColor rhs(WColorGammaUB(cpr.m_GammaRed, cpr.m_GammaGreen, cpr.m_GammaBlue, 255));

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = WMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_rgb = WMath::Lerp(lhs, rhs, lerpX);
  }
  }
  else if (m_ColorCPs.GetCount() == 1)
  {
    ref_rgb = WColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
  }
}

void WColorGradient::EvaluateAlpha(double x, WUInt8& ref_uiAlpha) const
{
  if (m_AlphaCPs.GetCount() != m_AlphaOrder.GetCount())
  {
    W_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_AlphaCPs.GetCount() != m_AlphaOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_uiAlpha = 255;

  const WUInt32 numCPs = m_AlphaCPs.GetCount();
  if (numCPs >= 2)
  {
    const WInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping
    const WUInt32 firstIdx = m_AlphaOrder[0];
    if (m_AlphaCPs[firstIdx].m_iTick >= xTick)
    {
      ref_uiAlpha = m_AlphaCPs[firstIdx].m_Alpha;
      return;
    }

    WUInt32 uiControlPoint;

    for (WUInt32 i = 1; i < numCPs; ++i)
    {
      const WUInt32 idx = m_AlphaOrder[i];
      if (m_AlphaCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const WUInt32 lastIdx = m_AlphaOrder[numCPs - 1];
      ref_uiAlpha = m_AlphaCPs[lastIdx].m_Alpha;
      return;
    }

  found:
  {
    /// \todo Use a midpoint interpolation

    const WUInt32 idxL = m_AlphaOrder[uiControlPoint];
    const WUInt32 idxR = m_AlphaOrder[uiControlPoint + 1];

    const AlphaCP& cpl = m_AlphaCPs[idxL];
    const AlphaCP& cpr = m_AlphaCPs[idxR];

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = WMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_uiAlpha = WMath::Lerp(cpl.m_Alpha, cpr.m_Alpha, lerpX);
  }
  }
  else if (m_AlphaCPs.GetCount() == 1)
  {
    ref_uiAlpha = m_AlphaCPs[0].m_Alpha;
  }
}

void WColorGradient::EvaluateIntensity(double x, float& ref_fIntensity) const
{
  if (m_IntensityCPs.GetCount() != m_IntensityOrder.GetCount())
  {
    W_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_IntensityCPs.GetCount() != m_IntensityOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_fIntensity = 1.0f;

  const WUInt32 numCPs = m_IntensityCPs.GetCount();
  if (m_IntensityCPs.GetCount() >= 2)
  {
    const WInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping
    const WUInt32 firstIdx = m_IntensityOrder[0];
    if (m_IntensityCPs[firstIdx].m_iTick >= xTick)
    {
      ref_fIntensity = m_IntensityCPs[firstIdx].m_Intensity;
      return;
    }

    WUInt32 uiControlPoint = 0;

    for (WUInt32 i = 1; i < numCPs; ++i)
    {
      const WUInt32 idx = m_IntensityOrder[i];
      if (m_IntensityCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const WUInt32 lastIdx = m_IntensityOrder[numCPs - 1];
      ref_fIntensity = m_IntensityCPs[lastIdx].m_Intensity;
      return;
    }

  found:
  {
    const WUInt32 idxL = m_IntensityOrder[uiControlPoint];
    const WUInt32 idxR = m_IntensityOrder[uiControlPoint + 1];

    const IntensityCP& cpl = m_IntensityCPs[idxL];
    const IntensityCP& cpr = m_IntensityCPs[idxR];

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = WMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_fIntensity = WMath::Lerp(cpl.m_Intensity, cpr.m_Intensity, lerpX);
  }
  }
  else if (m_IntensityCPs.GetCount() == 1)
  {
    ref_fIntensity = m_IntensityCPs[0].m_Intensity;
  }
}

WUInt64 WColorGradient::GetHeapMemoryUsage() const
{
  return m_ColorCPs.GetHeapMemoryUsage() + m_AlphaCPs.GetHeapMemoryUsage() + m_IntensityCPs.GetHeapMemoryUsage();
}

void WColorGradient::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = 3;

  inout_stream << uiVersion;

  const WUInt32 numColor = m_ColorCPs.GetCount();
  const WUInt32 numAlpha = m_AlphaCPs.GetCount();
  const WUInt32 numIntensity = m_IntensityCPs.GetCount();

  inout_stream << numColor;
  inout_stream << numAlpha;
  inout_stream << numIntensity;

  for (const auto& cp : m_ColorCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_GammaRed;
    inout_stream << cp.m_GammaGreen;
    inout_stream << cp.m_GammaBlue;
  }

  for (const auto& cp : m_AlphaCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_Alpha;
  }

  for (const auto& cp : m_IntensityCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_Intensity;
  }
}

void WColorGradient::Load(WStreamReader& inout_stream)
{
  WUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  W_ASSERT_DEV(uiVersion <= 3, "Incorrect version '{0}' for WColorGradient", uiVersion);

  WUInt32 numColor = 0;
  WUInt32 numAlpha = 0;
  WUInt32 numIntensity = 0;

  inout_stream >> numColor;
  inout_stream >> numAlpha;
  inout_stream >> numIntensity;

  m_ColorCPs.SetCountUninitialized((WUInt16)numColor);
  m_AlphaCPs.SetCountUninitialized((WUInt16)numAlpha);
  m_IntensityCPs.SetCountUninitialized((WUInt16)numIntensity);

  if (uiVersion == 1)
  {
    // Version 1: float positions
    float x;
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Intensity;
    }
  }
  else if (uiVersion == 2)
  {
    // Version 2: double positions
    double x;
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Intensity;
    }
  }
  else // version 3
  {
    // Version 3: tick positions
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_Intensity;
    }
  }
}


WInt64 WColorGradient::SnapTimeToTick(double fTimeInSeconds, WUInt32 uiFramesPerSecond)
{
  return SnapTickTo(TimeToTick(fTimeInSeconds), uiFramesPerSecond);
}

WInt64 WColorGradient::SnapTickTo(WInt64 iTick, WUInt32 uiFramesPerSecond)
{
  const WUInt32 uiTicksPerStep = 4800 / uiFramesPerSecond;
  return static_cast<WInt64>(WMath::RoundToMultiple(static_cast<double>(iTick), static_cast<double>(uiTicksPerStep)));
}

double WColorGradient::SnapTimeTo(double fTimeInSeconds, WUInt32 uiFramesPerSecond)
{
  return TickToTime(SnapTimeToTick(fTimeInSeconds, uiFramesPerSecond));
}


W_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_ColorGradient);
