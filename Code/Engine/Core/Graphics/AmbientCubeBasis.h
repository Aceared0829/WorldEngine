#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

/// Defines the basis directions for ambient cube sampling.
///
/// Provides the six cardinal directions (positive/negative X, Y, Z) used for
/// ambient lighting calculations and directional sampling.
struct W_CORE_DLL WAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static WVec3 s_Dirs[NumDirs];
};

/// Template class for storing ambient lighting data in a cube format.
///
/// Stores lighting values for six directions (the cardinal axes) to approximate
/// ambient lighting. Values can be added via directional samples and evaluated
/// for any normal direction using trilinear interpolation.
template <typename T>
struct WAmbientCube
{
  W_DECLARE_POD_TYPE();

  WAmbientCube();

  template <typename U>
  WAmbientCube(const WAmbientCube<U>& other);

  template <typename U>
  void operator=(const WAmbientCube<U>& other);

  bool operator==(const WAmbientCube& other) const;
  bool operator!=(const WAmbientCube& other) const;

  void AddSample(const WVec3& vDir, const T& value);

  T Evaluate(const WVec3& vNormal) const;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  T m_Values[WAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
