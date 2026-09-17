#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>

class WCurve1D;

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WCurveTangentMode);

template <typename T>
void FindNearestControlPoints(WArrayPtr<T> cps, WInt64 iTick, T*& ref_pLlhs, T*& lhs, T*& rhs, T*& ref_pRrhs)
{
  ref_pLlhs = nullptr;
  lhs = nullptr;
  rhs = nullptr;
  ref_pRrhs = nullptr;
  WInt64 lhsTick = WMath::MinValue<WInt64>();
  WInt64 llhsTick = WMath::MinValue<WInt64>();
  WInt64 rhsTick = WMath::MaxValue<WInt64>();
  WInt64 rrhsTick = WMath::MaxValue<WInt64>();

  for (decltype(auto) cp : cps)
  {
    if (cp.m_iTick <= iTick)
    {
      if (cp.m_iTick > lhsTick)
      {
        ref_pLlhs = lhs;
        llhsTick = lhsTick;

        lhs = &cp;
        lhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick > llhsTick)
      {
        ref_pLlhs = &cp;
        llhsTick = cp.m_iTick;
      }
    }

    if (cp.m_iTick > iTick)
    {
      if (cp.m_iTick < rhsTick)
      {
        ref_pRrhs = rhs;
        rrhsTick = rhsTick;

        rhs = &cp;
        rhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick < rrhsTick)
      {
        ref_pRrhs = &cp;
        rrhsTick = cp.m_iTick;
      }
    }
  }
}

class W_FOUNDATION_DLL WCurveControlPointData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCurveControlPointData, WReflectedClass);

public:
  WTime GetTickAsTime() const { return WTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(WTime time, WInt64 iFps);

  WInt64 m_iTick; // 4800 ticks per second
  double m_fValue;
  WVec2 m_LeftTangent = WVec2(-0.1f, 0.0f);
  WVec2 m_RightTangent = WVec2(+0.1f, 0.0f);
  bool m_bTangentsLinked = true;
  WEnum<WCurveTangentMode> m_LeftTangentMode;
  WEnum<WCurveTangentMode> m_RightTangentMode;
};

class W_FOUNDATION_DLL WSingleCurveData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSingleCurveData, WReflectedClass);

public:
  WColorGammaUB m_CurveColor = WColor::White;
  WDynamicArray<WCurveControlPointData> m_ControlPoints;

  void ConvertToRuntimeData(WCurve1D& out_result) const;
  double Evaluate(WInt64 iTick) const;
};

class W_FOUNDATION_DLL WCurveExtentsAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCurveExtentsAttribute, WPropertyAttribute);

public:
  WCurveExtentsAttribute() = default;
  WCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed);

  double m_fLowerExtent = 0.0;
  double m_fUpperExtent = 1.0;
  bool m_bLowerExtentFixed = false;
  bool m_bUpperExtentFixed = false;
};


class W_FOUNDATION_DLL WCurveGroupData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCurveGroupData, WReflectedClass);

public:
  WCurveGroupData() = default;
  WCurveGroupData(const WCurveGroupData& rhs) = delete;
  ~WCurveGroupData();
  WCurveGroupData& operator=(const WCurveGroupData& rhs) = delete;

  /// Makes a deep copy of rhs.
  void CloneFrom(const WCurveGroupData& rhs);

  /// Clears the curve and deallocates the curve data, if it is owned (e.g. if it was created through CloneFrom())
  void Clear();

  /// Can be set to false for cases where the instance is only supposed to act like a container for passing curve pointers around
  bool m_bOwnsData = true;
  WDynamicArray<WSingleCurveData*> m_Curves;
  WUInt16 m_uiFramesPerSecond = 60;

  WInt64 TickFromTime(WTime time) const;

  void ConvertToRuntimeData(WUInt32 uiCurveIdx, WCurve1D& out_result) const;
};

struct W_FOUNDATION_DLL WSelectedCurveCP
{
  W_DECLARE_POD_TYPE();

  WUInt16 m_uiCurve;
  WUInt16 m_uiPoint;
};
