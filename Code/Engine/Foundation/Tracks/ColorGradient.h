#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Threading/Mutex.h>

class WStreamWriter;
class WStreamReader;

/// Color control point. Stores red, green and blue in gamma space.
struct W_FOUNDATION_DLL WColorGradientColorCP
{
  W_DECLARE_POD_TYPE();

  WInt64 m_iTick; ///< Position in time. 4800 ticks per second.
  WUInt8 m_GammaRed;
  WUInt8 m_GammaGreen;
  WUInt8 m_GammaBlue;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  W_ALWAYS_INLINE bool operator<(const WColorGradientColorCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WColorGradientColorCP);

/// Alpha control point.
struct W_FOUNDATION_DLL WColorGradientAlphaCP
{
  W_DECLARE_POD_TYPE();

  WInt64 m_iTick;                  ///< Position in time. 4800 ticks per second.
  WUInt8 m_Alpha;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  W_ALWAYS_INLINE bool operator<(const WColorGradientAlphaCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WColorGradientAlphaCP);

/// Intensity control point. Used to scale rgb for high-dynamic range values.
struct W_FOUNDATION_DLL WColorGradientIntensityCP
{
  W_DECLARE_POD_TYPE();

  WInt64 m_iTick;                  ///< Position in time. 4800 ticks per second.
  float m_Intensity;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  W_ALWAYS_INLINE bool operator<(const WColorGradientIntensityCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WColorGradientIntensityCP);

/// A color curve for animating colors.
///
/// The gradient consists of a number of control points, for rgb, alpha and intensity.
/// One can evaluate the curve at any x coordinate.
class W_FOUNDATION_DLL WColorGradient
{
  W_ALLOW_PRIVATE_PROPERTIES(WColorGradient);

public:
  using ColorCP = WColorGradientColorCP;
  using AlphaCP = WColorGradientAlphaCP;
  using IntensityCP = WColorGradientIntensityCP;

public:
  WColorGradient();

  WColorGradient(const WColorGradient& rhs);
  WColorGradient(WColorGradient&& rhs) noexcept;

  void operator=(const WColorGradient& rhs);
  void operator=(WColorGradient&& rhs) noexcept;

  /// Removes all control points.
  void Clear();

  /// Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// Appends a color control point.
  void AddColorControlPoint(double x, const WColorGammaUB& rgb);

  /// Appends an alpha control point.
  void AddAlphaControlPoint(double x, WUInt8 uiAlpha);

  /// Appends an intensity control point.
  void AddIntensityControlPoint(double x, float fIntensity);

  /// Determines the min and max x-coordinate value across all control points.
  bool GetExtents(double& ref_fMinx, double& ref_fMaxx) const;

  /// Returns the number of control points of each type.
  void GetNumControlPoints(WUInt32& ref_uiRgb, WUInt32& ref_uiAlpha, WUInt32& ref_uiIntensity) const;

  /// Const access to a control point.
  const ColorCP& GetColorControlPoint(WUInt32 uiIdx) const { return m_ColorCPs[uiIdx]; }
  /// Const access to a control point.
  const AlphaCP& GetAlphaControlPoint(WUInt32 uiIdx) const { return m_AlphaCPs[uiIdx]; }
  /// Const access to a control point.
  const IntensityCP& GetIntensityControlPoint(WUInt32 uiIdx) const { return m_IntensityCPs[uiIdx]; }

  /// Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  ColorCP& ModifyColorControlPoint(WUInt32 uiIdx)
  {
    m_ColorOrder.Clear();
    return m_ColorCPs[uiIdx];
  }

  /// Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  AlphaCP& ModifyAlphaControlPoint(WUInt32 uiIdx)
  {
    m_AlphaOrder.Clear();
    return m_AlphaCPs[uiIdx];
  }

  /// Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  IntensityCP& ModifyIntensityControlPoint(WUInt32 uiIdx)
  {
    m_IntensityOrder.Clear();
    return m_IntensityCPs[uiIdx];
  }

  /// Evaluates the curve at the given x-coordinate and returns RGBA and intensity separately.
  void Evaluate(double x, WColorGammaUB& ref_rgba, float& ref_fIntensity) const;

  /// Evaluates the curve and returns RGBA and intensity in one combined WColor value.
  void Evaluate(double x, WColor& ref_hdr) const;

  /// Evaluates only the color curve.
  void EvaluateColor(double x, WColorGammaUB& ref_rgb) const;
  /// Evaluates only the color curve.
  void EvaluateColor(double x, WColor& ref_rgb) const;
  /// Evaluates only the alpha curve.
  void EvaluateAlpha(double x, WUInt8& ref_uiAlpha) const;
  /// Evaluates only the intensity curve.
  void EvaluateIntensity(double x, float& ref_fIntensity) const;

  /// How much heap memory the curve uses.
  WUInt64 GetHeapMemoryUsage() const;

  /// Stores the current state in a stream.
  void Save(WStreamWriter& inout_stream) const;

  /// Restores the state from a stream.
  void Load(WStreamReader& inout_stream);

  /// Converts a tick value to time (in seconds). 4800 ticks per second.
  static double TickToTime(WInt64 iTick) { return iTick / 4800.0; }

  /// Converts a time value (in seconds) to ticks. 4800 ticks per second.
  static WInt64 TimeToTick(double fTimeInSeconds) { return static_cast<WInt64>(fTimeInSeconds * 4800.0); }

  /// Converts a time value to ticks and snaps to the nearest frame boundary for the given FPS.
  static WInt64 SnapTimeToTick(double fTimeInSeconds, WUInt32 uiFramesPerSecond = 120);

  /// Snaps a tick value to the nearest frame boundary for the given FPS.
  static WInt64 SnapTickTo(WInt64 iTick, WUInt32 uiFramesPerSecond);

  /// Snaps a time value to the nearest frame boundary for the given FPS.
  static double SnapTimeTo(double fTimeInSeconds, WUInt32 uiFramesPerSecond = 120);

private:
  /// Caches the inverse distance between consecutive control points for faster interpolation.
  void PrecomputeLerpNormalizer() const;

  /// Builds the sort order arrays without modifying the original control point arrays.
  ///
  /// Control points are stored in insertion order to preserve array indices during editing.
  /// The sort order arrays are built lazily during evaluation to access points in temporal order.
  void UpdatePointOrder() const;

  WSmallArray<ColorCP, 8> m_ColorCPs;
  WSmallArray<AlphaCP, 8> m_AlphaCPs;
  WSmallArray<IntensityCP, 8> m_IntensityCPs;

  /// Mapping from sorted position to storage index. Cleared when control points are modified.
  mutable WSmallArray<WUInt8, 8> m_ColorOrder;
  mutable WSmallArray<WUInt8, 8> m_AlphaOrder;
  mutable WSmallArray<WUInt8, 8> m_IntensityOrder;

  /// Protects lazy initialization of sort order arrays and precomputed values during evaluation.
  mutable WMutex m_InitializationMutex;
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WColorGradient);
