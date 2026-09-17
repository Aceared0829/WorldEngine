#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

/// Compressed representation of ambient cube sky visibility data.
///
/// Packs 6 directional visibility values into a 32-bit integer using variable bit depths
/// (5 bits for the 4 horizontal directions, 6 bits for the 2 vertical directions).
using WCompressedSkyVisibility = WUInt32;

namespace WBakingUtils
{
  /// Generates a point on a unit sphere using Fibonacci spiral sampling.
  ///
  /// This produces evenly distributed points on a sphere, useful for hemisphere sampling
  /// during baking. The distribution minimizes clustering and gaps.
  W_RENDERERCORE_DLL WVec3 FibonacciSphere(WUInt32 uiSampleIndex, WUInt32 uiNumSamples);

  /// Compresses ambient cube sky visibility into a 32-bit value.
  ///
  /// Uses variable bit depths per direction (5 or 6 bits) to pack the data.
  /// Values are clamped to [0,1] before compression.
  W_RENDERERCORE_DLL WCompressedSkyVisibility CompressSkyVisibility(const WAmbientCube<float>& skyVisibility);

  /// Decompresses sky visibility from a 32-bit value back to ambient cube format.
  W_RENDERERCORE_DLL void DecompressSkyVisibility(WCompressedSkyVisibility compressedSkyVisibility, WAmbientCube<float>& out_skyVisibility);
} // namespace WBakingUtils
