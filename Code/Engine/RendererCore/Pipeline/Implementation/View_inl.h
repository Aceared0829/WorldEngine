
W_ALWAYS_INLINE WViewHandle WView::GetHandle() const
{
  return WViewHandle(m_InternalId);
}

W_ALWAYS_INLINE WStringView WView::GetName() const
{
  return m_Data.m_sName.GetView();
}

W_ALWAYS_INLINE WWorld* WView::GetWorld()
{
  return m_pWorld;
}

W_ALWAYS_INLINE const WWorld* WView::GetWorld() const
{
  return m_pWorld;
}

W_ALWAYS_INLINE WGALSwapChainHandle WView::GetSwapChain() const
{
  return m_Data.m_hSwapChain;
}

W_ALWAYS_INLINE const WGALRenderTargets& WView::GetRenderTargets() const
{
  return m_Data.m_RenderTargets;
}

W_ALWAYS_INLINE void WView::SetCamera(WCamera* pCamera)
{
  m_pCamera = pCamera;
}

W_ALWAYS_INLINE WCamera* WView::GetCamera()
{
  return m_pCamera;
}

W_ALWAYS_INLINE const WCamera* WView::GetCamera() const
{
  return m_pCamera;
}

W_ALWAYS_INLINE void WView::SetCullingCamera(const WCamera* pCamera)
{
  m_pCullingCamera = pCamera;
}

W_ALWAYS_INLINE const WCamera* WView::GetCullingCamera() const
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

W_ALWAYS_INLINE void WView::SetLodCamera(const WCamera* pCamera)
{
  m_pLodCamera = pCamera;
}

W_ALWAYS_INLINE const WCamera* WView::GetLodCamera() const
{
  return m_pLodCamera != nullptr ? m_pLodCamera : m_pCamera;
}

W_ALWAYS_INLINE WEnum<WCameraUsageHint> WView::GetCameraUsageHint() const
{
  return m_Data.m_CameraUsageHint;
}

W_ALWAYS_INLINE WEnum<WViewRenderMode> WView::GetViewRenderMode() const
{
  return m_Data.m_ViewRenderMode;
}

W_ALWAYS_INLINE const WRectFloat& WView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

W_ALWAYS_INLINE const WViewData& WView::GetData() const
{
  UpdateCachedMatrices();
  return m_Data;
}

W_FORCE_INLINE bool WView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

W_ALWAYS_INLINE const WSharedPtr<WTask>& WView::GetExtractTask()
{
  return m_pExtractTask;
}

W_FORCE_INLINE WResult WView::ComputePickingRay(float fScreenPosX, float fScreenPosY, WVec3& out_vRayStartPos, WVec3& out_vRayDir) const
{
  UpdateCachedMatrices();
  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_vRayStartPos, out_vRayDir);
}

W_FORCE_INLINE WResult WView::ComputeScreenSpacePos(const WVec3& vPoint, WVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

W_FORCE_INLINE WResult WView::ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, WVec3& out_vWorldPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeWorldSpacePos(fNormalizedScreenPosX, fNormalizedScreenPosY, out_vWorldPos);
}

W_FORCE_INLINE void WView::ConvertScreenPixelPosToNormalizedPos(WVec3& inout_vPixelPos)
{
  m_Data.ConvertScreenPixelPosToNormalizedPos(inout_vPixelPos);
}

W_FORCE_INLINE void WView::ConvertScreenNormalizedPosToPixelPos(WVec3& inout_vNormalizedPos)
{
  m_Data.ConvertScreenNormalizedPosToPixelPos(inout_vNormalizedPos);
}

W_ALWAYS_INLINE const WMat4& WView::GetProjectionMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ProjectionMatrix[static_cast<int>(eye)];
}

W_ALWAYS_INLINE const WMat4& WView::GetInverseProjectionMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<int>(eye)];
}

W_ALWAYS_INLINE const WMat4& WView::GetViewMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<int>(eye)];
}

W_ALWAYS_INLINE const WMat4& WView::GetInverseViewMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<int>(eye)];
}

W_ALWAYS_INLINE const WMat4& WView::GetViewProjectionMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<int>(eye)];
}

W_ALWAYS_INLINE const WMat4& WView::GetInverseViewProjectionMatrix(WCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<int>(eye)];
}
