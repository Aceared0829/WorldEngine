#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGizmo, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WGizmo::WGizmo()
{
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
}

void WGizmo::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void WGizmo::SetTransformation(const WTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}

void WGizmo::GetInverseViewProjectionMatrix(WMat4& out_mInvViewProj) const
{
  if (m_pCamera == nullptr || m_vViewport.x == 0.0f || m_vViewport.y == 0.0f)
  {
    out_mInvViewProj.SetIdentity();
    return;
  }

  WMat4 mView = m_pCamera->GetViewMatrix();
  WMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  WMat4 mViewProj = mProj * mView;
  out_mInvViewProj = mViewProj.GetInverse();
}

WResult WGizmo::GetPointOnPlane(const WPlane& plane, const WVec2I32& vScreenPos, const WMat4& mInvViewProj, WVec3& out_Result) const
{
  out_Result = WVec3::MakeZero();

  WVec3 vPos, vRayDir;
  if (WGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, WVec3(vScreenPos.x, vScreenPos.y, 0), vPos, &vRayDir).Failed())
    return W_FAILURE;

  WVec3 vIntersection;
  if (!plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return W_FAILURE;

  out_Result = vIntersection;
  return W_SUCCESS;
}

WResult WGizmo::GetPointOnAxis(const WVec3& vStartPos, const WVec3& vAxis, const WVec2I32& vScreenPos, const WMat4& mInvViewProj, WVec3& out_Result, float* out_pProjectedLength /*= nullptr*/) const
{
  out_Result = vStartPos;

  WVec3 vPos, vRayDir;
  if (WGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, WVec3(vScreenPos.x, vScreenPos.y, 0), vPos, &vRayDir).Failed())
    return W_FAILURE;

  const WVec3 vPlaneTangent = vAxis.CrossRH(m_pCamera->GetDirForwards()).GetNormalized();
  const WVec3 vPlaneNormal = vAxis.CrossRH(vPlaneTangent);

  WPlane Plane;
  Plane = WPlane::MakeFromNormalAndPoint(vPlaneNormal, vStartPos);

  WVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return W_FAILURE;

  const WVec3 vDirAlongRay = vIntersection - vStartPos;
  const float fProjectedLength = vDirAlongRay.Dot(vAxis);
  if (out_pProjectedLength != nullptr)
  {
    *out_pProjectedLength = fProjectedLength;
  }

  out_Result = vStartPos + fProjectedLength * vAxis;
  return W_SUCCESS;
}
