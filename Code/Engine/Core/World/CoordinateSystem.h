#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/RefCounted.h>

/// Defines a coordinate system through three orthogonal direction vectors.
///
/// Represents a 3D coordinate system using forward, right, and up direction vectors.
/// Used for transforming between different coordinate conventions in the engine.
struct W_CORE_DLL WCoordinateSystem
{
  W_DECLARE_POD_TYPE();

  WVec3 m_vForwardDir;
  WVec3 m_vRightDir;
  WVec3 m_vUpDir;
};

/// Abstract base class for providing coordinate systems at specific world positions.
///
/// Allows defining position-dependent coordinate systems within a world. Derived classes
/// implement GetCoordinateSystem to provide custom coordinate transformations based on
/// world position, enabling features like gravity-aligned coordinate systems or curved spaces.
class W_CORE_DLL WCoordinateSystemProvider : public WRefCounted
{
public:
  WCoordinateSystemProvider(const WWorld* pOwnerWorld)
    : m_pOwnerWorld(pOwnerWorld)
  {
  }

  virtual ~WCoordinateSystemProvider() = default;

  /// Returns the coordinate system at the given global position.
  virtual void GetCoordinateSystem(const WVec3& vGlobalPosition, WCoordinateSystem& out_coordinateSystem) const = 0;

protected:
  friend class WWorld;

  const WWorld* m_pOwnerWorld;
};

/// Helper class to convert between two WCoordinateSystem spaces.
///
/// All functions will do an identity transform until SetConversion is called to set up
/// the conversion. Afterwards the convert functions can be used to convert between
/// the two systems in both directions.
/// Currently, only uniformly scaled orthogonal coordinate systems are supported.
/// They can however be right handed or left handed.
class W_CORE_DLL WCoordinateSystemConversion
{
public:
  /// Creates a new conversion that until set up, does identity conversions.
  WCoordinateSystemConversion(); // [tested]

  /// Set up the source and target coordinate systems.
  void SetConversion(const WCoordinateSystem& source, const WCoordinateSystem& target); // [tested]
  /// Returns the equivalent point in the target coordinate system.
  WVec3 ConvertSourcePosition(const WVec3& vPos) const; // [tested]
  /// Returns the equivalent rotation in the target coordinate system.
  WQuat ConvertSourceRotation(const WQuat& qOrientation) const; // [tested]
  /// Returns the equivalent length in the target coordinate system.
  float ConvertSourceLength(float fLength) const; // [tested]

  /// Returns the equivalent point in the source coordinate system.
  WVec3 ConvertTargetPosition(const WVec3& vPos) const; // [tested]
  /// Returns the equivalent rotation in the source coordinate system.
  WQuat ConvertTargetRotation(const WQuat& qOrientation) const; // [tested]
  /// Returns the equivalent length in the source coordinate system.
  float ConvertTargetLength(float fLength) const; // [tested]

private:
  WMat3 m_mSourceToTarget;
  WMat3 m_mTargetToSource;
  float m_fWindingSwap = 1.0f;
  float m_fSourceToTargetScale = 1.0f;
  float m_fTargetToSourceScale = 1.0f;
};
