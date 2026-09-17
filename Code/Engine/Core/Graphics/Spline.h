#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>

class WStreamReader;
class WStreamWriter;

/// The different modes that tangents may use in a spline control point.
struct WSplineTangentMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the control point is automatically computed to be smooth.
    Custom, ///< Custom tangents specified by the user.
    Linear, ///< There is no curvature through this control point/tangent. Creates sharp corners.

    Default = Auto
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WSplineTangentMode);

//////////////////////////////////////////////////////////////////////////

/// Describes a spline consisting of cubic Bezier curves segments. Each control point defines the position, rotation, and scale at that point.
/// The parameter fT to evaluate the spline is a combination of the control point index and the zero to one parameter in the fractional part to interpolate within that segment.
struct W_CORE_DLL WSpline
{
  struct ControlPoint
  {
    WSimdVec4f m_vPos = WSimdVec4f::MakeZero();
    WSimdVec4f m_vPosTangentIn = WSimdVec4f::MakeZero();  // Contains the tangent mode in w
    WSimdVec4f m_vPosTangentOut = WSimdVec4f::MakeZero(); // Contains the tangent mode in w

    WSimdVec4f m_vUpDirAndRoll = WSimdVec4f::MakeZero();  // Roll angle in w
    WSimdVec4f m_vUpDirTangentIn = WSimdVec4f::MakeZero();
    WSimdVec4f m_vUpDirTangentOut = WSimdVec4f::MakeZero();

    WSimdVec4f m_vScale = WSimdVec4f(1);
    WSimdVec4f m_vScaleTangentIn = WSimdVec4f::MakeZero();
    WSimdVec4f m_vScaleTangentOut = WSimdVec4f::MakeZero();

    WResult Serialize(WStreamWriter& ref_writer) const;
    WResult Deserialize(WStreamReader& ref_reader);

    void SetPosition(const WSimdVec4f& vPos);

    WSplineTangentMode::Enum GetTangentModeIn() const;
    void SetTangentModeIn(WSplineTangentMode::Enum mode);
    void SetTangentIn(const WSimdVec4f& vTangent, WSplineTangentMode::Enum mode = WSplineTangentMode::Custom);

    WSplineTangentMode::Enum GetTangentModeOut() const;
    void SetTangentModeOut(WSplineTangentMode::Enum mode);
    void SetTangentOut(const WSimdVec4f& vTangent, WSplineTangentMode::Enum mode = WSplineTangentMode::Custom);

    WAngle GetRoll() const;
    void SetRoll(WAngle roll);

    void SetScale(const WSimdVec4f& vScale);

    void SetAutoTangents(const WSimdVec4f& vDirIn, const WSimdVec4f& vDirOut);
  };

  WDynamicArray<ControlPoint, WAlignedAllocatorWrapper> m_ControlPoints;
  bool m_bClosed = false;
  WUInt32 m_uiChangeCounter = 0; //< Not incremented automatically, but can be incremented by the user to signal that the spline has changed.

  WResult Serialize(WStreamWriter& ref_writer) const;
  WResult Deserialize(WStreamReader& ref_reader);


  WUInt32 GetNumControlPoints() const;
  WUInt32 GetNumSegments() const;


  /// Calculates tangents for all control points with a tangent mode other than 'Custom'.
  void CalculateUpDirAndAutoTangents(const WSimdVec4f& vGlobalUpDir = WSimdVec4f(0, 0, 1), const WSimdVec4f& vGlobalForwardDir = WSimdVec4f(1, 0, 0));


  /// Returns the position of the spline at the given parameter fT.
  WSimdVec4f EvaluatePosition(float fT) const;
  WSimdVec4f EvaluatePosition(WUInt32 uiCp0, const WSimdFloat& fT) const;

  /// Returns the derivative, aka the tangent of the spline at the given parameter fT. This also equals to the unnormalized forward direction.
  WSimdVec4f EvaluateDerivative(float fT) const;
  WSimdVec4f EvaluateDerivative(WUInt32 uiCp0, const WSimdFloat& fT) const;

  /// Returns the up direction of the spline at the given parameter fT.
  WSimdVec4f EvaluateUpDirection(float fT) const;

  /// Returns the scale of the spline at the given parameter fT.
  WSimdVec4f EvaluateScale(float fT) const;

  /// Returns the full transform (consisting of position, scale, and orientation) of the spline at the given parameter fT.
  WSimdTransform EvaluateTransform(float fT) const;


  /// Calculates the bounding volume of a single segment of the spline.
  WResult CalculateSegmentBounds(WUInt32 uiSegmentIndex, WSimdBBoxSphere& out_bounds) const;

  /// Calculates the bounding volume of the entire spline.
  WResult CalculateBounds(WSimdBBoxSphere& out_bounds) const;


  /// Finds the closest point on a single segment of the spline to the given point. Returns the position on the spline, the parameter fT, and the squared distance.
  WSimdVec4f FindClosestPointOnSegment(WUInt32 uiSegmentIndex, const WSimdVec4f& vPoint, float& out_fT, float& out_fDistanceSquared, float fMaxError = 0.1f) const;

  /// Finds the closest point on the entire spline to the given point. Returns the position on the spline, the parameter fT, and the squared distance.
  WSimdVec4f FindClosestPoint(const WSimdVec4f& vPoint, float& out_fT, float& out_fDistanceSquared, float fMaxError = 0.1f) const;

private:
  float ClampAndSplitT(float fT, WUInt32& out_uiIndex) const;
  WUInt32 GetCp1Index(WUInt32 uiCp0) const;

  WSimdVec4f EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const;
  WSimdVec4f EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const;

  void EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT, WSimdVec4f& out_forwardDir, WSimdVec4f& out_rightDir, WSimdVec4f& out_upDir) const;

  WSimdVec4f EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const WSimdFloat& fT) const;
};

#include <Core/Graphics/Implementation/Spline_inl.h>
