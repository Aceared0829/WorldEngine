#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec3.h>

/// Helper functions to convert between WorldEngine and OpenXR types.
class W_OPENXRPLUGIN_DLL WOpenXRConversionUtils
{
public:
  static XrPosef ConvertTransform(const WTransform& tr);
  static XrQuaternionf ConvertOrientation(const WQuat& q);
  static XrVector3f ConvertPosition(const WVec3& vPos);
  static WQuat ConvertOrientation(const XrQuaternionf& q);
  static WVec3 ConvertPosition(const XrVector3f& pos);
  static WMat4 ConvertPoseToMatrix(const XrPosef& pose);
};

#include <OpenXRPlugin/Utils/Implementation/OpenXRConversionUtils.inl.h>
