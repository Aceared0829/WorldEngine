#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererFoundation/Device/SwapChain.h>

/// Holds view data like the viewport, view and projection matrices
struct W_RENDERERCORE_DLL WViewData
{
  WViewData()
  {
    m_ViewPortRect = WRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = WViewRenderMode::None;

    for (int i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  WHashedString m_sName;
  WUInt32 m_uiSkyIrradianceIndex = 0;

  WGALRenderTargets m_RenderTargets;
  WGALSwapChainHandle m_hSwapChain;
  WRectFloat m_ViewPortRect;
  WEnum<WViewRenderMode> m_ViewRenderMode;
  WEnum<WCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  WMat4 m_ViewMatrix[2];
  WMat4 m_InverseViewMatrix[2];
  WMat4 m_ProjectionMatrix[2];
  WMat4 m_InverseProjectionMatrix[2];
  WMat4 m_ViewProjectionMatrix[2];
  WMat4 m_InverseViewProjectionMatrix[2];

  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, W_FAILURE is returned.
  W_ALWAYS_INLINE WResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, WVec3& out_vRayStartPos, WVec3& out_vRayDir, WCameraEye eye = WCameraEye::Left) const
  {
    WVec3 vScreenPos;
    vScreenPos.x = fNormalizedScreenPosX;
    vScreenPos.y = fNormalizedScreenPosY;
    vScreenPos.z = 0.0f;

    return WGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<int>(eye)], vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns W_FAILURE, if the point could not be projected into screen-space.
  W_ALWAYS_INLINE WResult ComputeScreenSpacePos(const WVec3& vWorldPos, WVec3& out_vScreenPosNormalized, WCameraEye eye = WCameraEye::Left) const
  {
    return WGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], vWorldPos, out_vScreenPosNormalized);
  }

  /// Calculates the world-space position that the given normalized screen-space coordinate maps to
  W_ALWAYS_INLINE WResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, WVec3& out_vWorldPos, WCameraEye eye = WCameraEye::Left) const
  {
    return WGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<int>(eye)], WVec3(fNormalizedScreenPosX, fNormalizedScreenPosY, 0.0f), out_vWorldPos);
  }

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  W_ALWAYS_INLINE void ConvertScreenPixelPosToNormalizedPos(WVec3& inout_vPixelPos) const
  {
    WUInt32 x = (WUInt32)m_ViewPortRect.x;
    WUInt32 y = (WUInt32)m_ViewPortRect.y;
    WUInt32 w = (WUInt32)m_ViewPortRect.width;
    WUInt32 h = (WUInt32)m_ViewPortRect.height;
    WGraphicsUtils::ConvertScreenPixelPosToNormalizedPos(x, y, w, h, inout_vPixelPos);
  }

  /// Returns the active render targets. If a swap chain is set, its render targets are returned, otherwise m_RenderTargets.
  const WGALRenderTargets& GetActiveRenderTargets() const;

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  W_ALWAYS_INLINE void ConvertScreenNormalizedPosToPixelPos(WVec3& inout_vNormalizedPos) const
  {
    {
      WUInt32 x = (WUInt32)m_ViewPortRect.x;
      WUInt32 y = (WUInt32)m_ViewPortRect.y;
      WUInt32 w = (WUInt32)m_ViewPortRect.width;
      WUInt32 h = (WUInt32)m_ViewPortRect.height;
      WGraphicsUtils::ConvertScreenNormalizedPosToPixelPos(x, y, w, h, inout_vNormalizedPos);
    }
  }
};
