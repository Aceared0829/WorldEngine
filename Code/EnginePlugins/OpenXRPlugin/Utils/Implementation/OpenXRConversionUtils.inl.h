
W_ALWAYS_INLINE XrPosef WOpenXRConversionUtils::ConvertTransform(const WTransform& tr)
{
  XrPosef pose;
  pose.orientation = ConvertOrientation(tr.m_qRotation);
  pose.position = ConvertPosition(tr.m_vPosition);
  return pose;
}

W_ALWAYS_INLINE XrQuaternionf WOpenXRConversionUtils::ConvertOrientation(const WQuat& q)
{
  return {q.y, q.z, -q.x, -q.w};
}

W_ALWAYS_INLINE XrVector3f WOpenXRConversionUtils::ConvertPosition(const WVec3& vPos)
{
  return {vPos.y, vPos.z, -vPos.x};
}

W_ALWAYS_INLINE WQuat WOpenXRConversionUtils::ConvertOrientation(const XrQuaternionf& q)
{
  return {-q.z, q.x, q.y, -q.w};
}

W_ALWAYS_INLINE WVec3 WOpenXRConversionUtils::ConvertPosition(const XrVector3f& pos)
{
  return {-pos.z, pos.x, pos.y};
}

W_ALWAYS_INLINE WMat4 WOpenXRConversionUtils::ConvertPoseToMatrix(const XrPosef& pose)
{
  WMat4 m;
  WMat3 rot = ConvertOrientation(pose.orientation).GetAsMat3();
  WVec3 pos = ConvertPosition(pose.position);
  m.SetTransformationMatrix(rot, pos);
  return m;
}
