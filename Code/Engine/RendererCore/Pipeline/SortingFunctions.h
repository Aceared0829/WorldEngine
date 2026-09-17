#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Provides sorting functions for render data.
///
/// These functions generate 64-bit sorting keys used to order render data for optimal rendering.
/// Different sorting strategies are used for different render passes (opaque vs transparent).
struct W_RENDERERCORE_DLL WRenderSortingFunctions
{
  using StorageType = WUInt8;

  enum Enum
  {
    ByRenderDataThenFrontToBack,
    BackToFrontThenByRenderData,
    ByDepthOffsetOnly,
    BySortingKeyOnly,

    Default = ByRenderDataThenFrontToBack
  };

  using Func = WUInt64 (*)(const WRenderData*, const WCamera&);

  /// Sorts by render data type first, then by render data sorting key, then by depth front-to-back.
  ///
  /// Used for opaque geometry to minimize state changes and benefit from early-z rejection.
  static WUInt64 ByRenderDataThenFrontToBackFunc(const WRenderData* pRenderData, const WCamera& camera);

  /// Sorts by depth back-to-front, then by render data type, then by render data sorting key.
  ///
  /// Used for transparent geometry to ensure correct blending order.
  static WUInt64 BackToFrontThenByRenderDataFunc(const WRenderData* pRenderData, const WCamera& camera);

  /// Sorts only by the render data's depth offset back-to-front, meaning render data with a higher depth offset is rendered first.
  ///
  /// This can be used for special cases like full-screen effects where the render order needs to be fully deterministic.
  static WUInt64 ByDepthOffsetOnlyFunc(const WRenderData* pRenderData, const WCamera& camera);

  /// Sorts only by the render data's sorting key.
  ///
  /// Used for special cases like lights where the sorting key is already carefully constructed to achieve the desired order, and distance-based sorting is not needed.
  static WUInt64 BySortingKeyOnlyFunc(const WRenderData* pRenderData, const WCamera& camera);

  /// Returns the sorting function corresponding to the given enum value.
  static Func GetFunction(Enum sortingFunction);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderSortingFunctions);
